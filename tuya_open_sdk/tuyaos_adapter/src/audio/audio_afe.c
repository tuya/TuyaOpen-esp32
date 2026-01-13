/**
 * @file audio_afe.c
 * @version 0.1
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include "tuya_cloud_types.h"
#include "tuya_ringbuf.h"

#include "tkl_memory.h"
#include "tkl_system.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"

#include "esp_err.h"
#include "esp_log.h"
#include <esp_afe_sr_models.h>
#include <freertos/FreeRTOS.h>
#include "freertos/queue.h"

#include "audio_afe.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TAG                       "audio_afe"
#define FEED_RB_SIZE              (8 * 1024)
/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    bool                is_init;
    bool                is_start;
    TKL_THREAD_HANDLE   thread;
    TUYA_RINGBUFF_T     rb;
    TKL_MUTEX_HANDLE    rb_mutex;
    uint32_t            uint_size;

    esp_afe_sr_iface_t *afe_iface;
    esp_afe_sr_data_t  *afe_data;
}AUDIO_AFE_PROCE_T;

/***********************************************************
***********************variable define**********************
***********************************************************/
static AUDIO_AFE_PROCE_T sg_afe_proce;
static AUDIO_AFE_WAKEUP_CB sg_afe_wakeup_cb = NULL;
static AUDIO_AFE_VAD_CB sg_afe_vad_cb = NULL;

/***********************************************************
***********************function define**********************
***********************************************************/
static void __afe_feed_task(void *arg)
{
    uint8_t *feed_data = tkl_system_malloc(sg_afe_proce.uint_size);
    assert(feed_data);

    for (;;) {
        tkl_mutex_lock(sg_afe_proce.rb_mutex);
        uint32_t used_size = tuya_ring_buff_used_size_get(sg_afe_proce.rb);
        tkl_mutex_unlock(sg_afe_proce.rb_mutex);
        if (used_size < sg_afe_proce.uint_size) {
            // wait for data
            // ESP_LOGI(TAG, "Waiting for data...");
            tkl_system_sleep(10);
            continue;
        }

        memset(feed_data, 0, sg_afe_proce.uint_size);
        // Read data from ring buffer
        tkl_mutex_lock(sg_afe_proce.rb_mutex);
        tuya_ring_buff_read(sg_afe_proce.rb, feed_data, sg_afe_proce.uint_size);
        tkl_mutex_unlock(sg_afe_proce.rb_mutex);

        sg_afe_proce.afe_iface->feed(sg_afe_proce.afe_data,\
                                     (const int16_t *)feed_data);

        tkl_system_sleep(10);
    }
}

static void __afe_detect_task(void *arg)
{
    for (;;) {
        afe_fetch_result_t* res = sg_afe_proce.afe_iface->fetch_with_delay(sg_afe_proce.afe_data,\
                                                                            portMAX_DELAY);
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGE(TAG, "fetch error!");
            continue;
        }

        if(false == sg_afe_proce.is_start) {
            continue;
        }

        // ESP_LOGI(TAG, "vad state: %d", res->vad_state);
        if(sg_afe_vad_cb) {
            sg_afe_vad_cb(res->vad_state);
        }

        // Set wake word status
        if (res->wakeup_state == WAKENET_DETECTED) {
            ESP_LOGI(TAG, "wakeword detected");
            ESP_LOGI(TAG, "model index:%d, word index:%d", res->wakenet_model_index, res->wake_word_index);
            ESP_LOGI(TAG, "-----------LISTENING-----------");

            if (1 == res->wakenet_model_index) {
                //wakeup callback
                if(sg_afe_wakeup_cb) {
                    sg_afe_wakeup_cb();
                }
            } 
        }
    }
}

static OPERATE_RET __esp_afe_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    srmodel_list_t *models = esp_srmodel_init("model");
    if (NULL == models) {
        ESP_LOGE(TAG, "Failed to initialize SR models");
        return OPRT_COM_ERROR;
    }

    for (int i=0; i<models->num; i++) {
        if (strstr(models->model_name[i], ESP_WN_PREFIX) != NULL) {
            ESP_LOGI(TAG, "wakenet model in flash: %s", models->model_name[i]);
        }
    }

    char* ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL);

    afe_config_t* afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    afe_config->aec_init = false;
    afe_config->ns_init = true;
    afe_config->ns_model_name = ns_model_name;
    afe_config->afe_ns_mode = AFE_NS_MODE_NET;
    afe_config->vad_init = true;
    afe_config->vad_mode = VAD_MODE_0;
    afe_config->vad_min_speech_ms = 128*4;
    afe_config->vad_min_noise_ms = 500;
    afe_config->vad_delay_ms = 128*2;
    afe_config->afe_perferred_core = 1;
    afe_config->afe_perferred_priority = 1;
    afe_config->agc_init = false;
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

    sg_afe_proce.afe_iface = (esp_afe_sr_iface_t *)esp_afe_handle_from_config(afe_config);
    if (NULL == sg_afe_proce.afe_iface) {
        ESP_LOGE(TAG, "Failed to get afe handle");
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

    sg_afe_proce.afe_data = (esp_afe_sr_data_t *)sg_afe_proce.afe_iface->create_from_config(afe_config);
    if (NULL ==  sg_afe_proce.afe_data) {
        ESP_LOGE(TAG, "Failed to create afe data");
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

    // get required data size
    int chunksize = sg_afe_proce.afe_iface->get_feed_chunksize(sg_afe_proce.afe_data);
    // TODO: support multi-channel
    int feed_channel = 1;
    sg_afe_proce.uint_size = chunksize * sizeof(int16_t) * feed_channel;
    ESP_LOGI(TAG, "Process unit size: %ld", sg_afe_proce.uint_size);

__EXIT:
    if (NULL == afe_config) {
        afe_config_free(afe_config);
        afe_config = NULL;
    }

    return rt;
}

OPERATE_RET audio_afe_processor_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    rt = __esp_afe_init();
    if(rt != OPRT_OK) {
        ESP_LOGE(TAG, "init failed %d", rt);
    }

    rt = tkl_mutex_create_init(&sg_afe_proce.rb_mutex);
    if (OPRT_OK != rt) {
        ESP_LOGE(TAG, "Failed to create rb mutex");
        return rt;
    }

    // Initialize ring buffer
    rt = tuya_ring_buff_create(FEED_RB_SIZE, OVERFLOW_STOP_TYPE, &sg_afe_proce.rb);
    if (OPRT_OK != rt) {
        ESP_LOGE(TAG, "Failed to create ring buffer");
        return rt;
    }

    xTaskCreatePinnedToCore(&__afe_feed_task, "afe_feed", 8 * 1024, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(&__afe_detect_task, "afe_detect", 4 * 1024, NULL, 4, NULL, 1);

    ESP_LOGI(TAG, "AFE initialized successfully");

    sg_afe_proce.is_init = true;

    return OPRT_OK;
}
void auio_afe_processor_feed(uint8_t *data, int len)
{
    if(NULL == data || 0 == len) {
        ESP_LOGE(TAG, "param is err");    
        return;
    }

    if(false == sg_afe_proce.is_init) {
        ESP_LOGE(TAG, "afe is not init");    
        return;  
    }

    if(false == sg_afe_proce.is_start) {
        return;
    }

    tkl_mutex_lock(sg_afe_proce.rb_mutex);

    uint32_t free_size = tuya_ring_buff_free_size_get(sg_afe_proce.rb);
    if (free_size < len) {
        tuya_ring_buff_discard(sg_afe_proce.rb, len);
    }
    tuya_ring_buff_write(sg_afe_proce.rb, data, len);

    tkl_mutex_unlock(sg_afe_proce.rb_mutex);
}

void auio_afe_processor_start(void)
{
    if(true == sg_afe_proce.is_start) {
        return;
    }

    sg_afe_proce.is_start = true;
}

void auio_afe_processor_end(void)
{
    if(false == sg_afe_proce.is_start) {
        return;
    }
    
    sg_afe_proce.is_start = false;

    tkl_mutex_lock(sg_afe_proce.rb_mutex);
    tuya_ring_buff_reset(sg_afe_proce.rb);
    tkl_mutex_unlock(sg_afe_proce.rb_mutex);
}

void auio_afe_register_wakeup_cb(AUDIO_AFE_WAKEUP_CB cb)
{
    sg_afe_wakeup_cb = cb;
}

void auio_afe_register_vad_cb(AUDIO_AFE_VAD_CB cb)
{
    sg_afe_vad_cb = cb;
}
