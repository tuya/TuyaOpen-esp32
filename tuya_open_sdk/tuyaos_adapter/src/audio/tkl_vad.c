/**
 * @file tkl_vad.c
 * @brief tkl_vad module is used to 
 * @version 0.1
 * @date 2025-04-15
 */

#include "tuya_cloud_types.h"

#include "tuya_ringbuf.h"

#include "tkl_vad.h"
#include "tkl_memory.h"
#include "tkl_thread.h"
#include "tkl_system.h"
#include "tkl_mutex.h"

#include "esp_err.h"
#include "esp_log.h"
#include <esp_afe_sr_models.h>
#include <freertos/FreeRTOS.h>
#include "freertos/queue.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TAG "tkl_vad"

#define FEED_RB_SIZE (8 * 1024)

typedef uint8_t VAD_TASK_STATUS_T;
#define VAD_TASK_STATUS_NONE 0
#define VAD_TASK_STATUS_WORKING 1
#define VAD_TASK_STATUS_STOP 2
#define VAD_TASK_STATUS_MAX 3

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    uint8_t is_init;
    TKL_VAD_STATUS_T vad_status;

    TKL_THREAD_HANDLE thread_hdl;
    TKL_MUTEX_HANDLE mutex_hdl;
    VAD_TASK_STATUS_T task_status;

    esp_afe_sr_iface_t* afe_iface;
    esp_afe_sr_data_t* afe_data;

    uint32_t process_uint_size;

    TUYA_RINGBUFF_T rb_hdl;

    uint32_t feed_size;
} TKL_VAD_DATA_T;

/***********************************************************
********************function declaration********************
***********************************************************/
#define TKL_VAD_TASK_STAT_CHANGE(new_stat)                                                                             \
    do {                                                                                                               \
        ESP_LOGI(TAG, "tkl vad stat changed: %d->%d", sg_vad_hdl.task_status, new_stat);                               \
        sg_vad_hdl.task_status = new_stat;                                                                             \
    } while (0)

/***********************************************************
***********************variable define**********************
***********************************************************/
static volatile TKL_VAD_DATA_T sg_vad_hdl = {
    .is_init = 0,
    .vad_status = TKL_VAD_STATUS_NONE,
    .task_status = VAD_TASK_STATUS_NONE,
    .afe_iface = NULL,
    .afe_data = NULL,
    .feed_size = 0,
};

/***********************************************************
***********************function define**********************
***********************************************************/
static void __tkl_vad_feed_task(void *arg)
{
    uint8_t *feed_data = tkl_system_malloc(sg_vad_hdl.process_uint_size);
    assert(feed_data);

    for (;;) {
        uint32_t used_size = tuya_ring_buff_used_size_get(sg_vad_hdl.rb_hdl);
        if (used_size < sg_vad_hdl.process_uint_size) {
            // wait for data
            // ESP_LOGI(TAG, "Waiting for data...");
            tkl_system_sleep(10);
            continue;
        }

        memset(feed_data, 0, sg_vad_hdl.process_uint_size);
        // Read data from ring buffer
        tuya_ring_buff_read(sg_vad_hdl.rb_hdl, feed_data, sg_vad_hdl.process_uint_size);
        // ESP_LOGI(TAG, "feed data size: %ld", sg_vad_hdl.process_uint_size);
        sg_vad_hdl.afe_iface->feed(sg_vad_hdl.afe_data, (const int16_t *)feed_data);
        tkl_system_sleep(10);
    }
}

static void __tkl_vad_detect_task(void *arg)
{
    static afe_fetch_result_t last_vad_state = {
        .vad_state = TKL_VAD_STATUS_NONE
    };
    for (;;) {
        afe_fetch_result_t* res = sg_vad_hdl.afe_iface->fetch_with_delay(sg_vad_hdl.afe_data, portMAX_DELAY);
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGE(TAG, "fetch error!");
            continue;
        }
        if(last_vad_state.vad_state != res->vad_state) {
            ESP_LOGI(TAG, "vad state: %d -> %d", last_vad_state.vad_state, res->vad_state);
            last_vad_state.vad_state = res->vad_state;
        }
        sg_vad_hdl.vad_status = (res->vad_state == VAD_SPEECH) ? TKL_VAD_STATUS_SPEECH : TKL_VAD_STATUS_NONE;
    }
}

static int __tkl_vad_init(void)
{
    // Initialize AFE (Audio Front End) settings here

    // Single microphone, non-real-time
    srmodel_list_t *models = esp_srmodel_init("model");
    char* ns_model_name = esp_srmodel_filter(models, ESP_NSNET_PREFIX, NULL);

    afe_config_t* afe_config = afe_config_init("M", NULL, AFE_TYPE_VC, AFE_MODE_HIGH_PERF);
    afe_config->aec_init = false;
    afe_config->ns_init = true;
    afe_config->ns_model_name = ns_model_name;
    afe_config->afe_ns_mode = AFE_NS_MODE_NET;
    afe_config->vad_init = true;
    afe_config->vad_mode = VAD_MODE_0;
    afe_config->vad_min_speech_ms = 128*5;
    afe_config->vad_min_noise_ms = 300;
    afe_config->vad_delay_ms = 128*5;
    afe_config->afe_perferred_core = 1;
    afe_config->afe_perferred_priority = 1;
    afe_config->agc_init = false;
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

    sg_vad_hdl.afe_iface = esp_afe_handle_from_config(afe_config);
    sg_vad_hdl.afe_data = sg_vad_hdl.afe_iface->create_from_config(afe_config);

    return 0;
}

OPERATE_RET tkl_vad_init(TKL_VAD_CONFIG_T *config)
{
    OPERATE_RET rt = OPRT_OK;

    if (NULL == config) {
        ESP_LOGE(TAG, "Invalid config");
        return OPRT_INVALID_PARM;
    }

    // Initialize ring buffer
    rt = tuya_ring_buff_create(FEED_RB_SIZE, OVERFLOW_STOP_TYPE, &sg_vad_hdl.rb_hdl);
    if (OPRT_OK != rt) {
        ESP_LOGE(TAG, "Failed to create ring buffer");
        return rt;
    }

    __tkl_vad_init();

    // get required data size
    int chunksize = sg_vad_hdl.afe_iface->get_feed_chunksize(sg_vad_hdl.afe_data);
    // TODO: support multi-channel
    int feed_channel = 1;
    sg_vad_hdl.process_uint_size = chunksize * sizeof(int16_t) * feed_channel;
    ESP_LOGI(TAG, "Process unit size: %ld", sg_vad_hdl.process_uint_size);

    xTaskCreatePinnedToCore(&__tkl_vad_feed_task, "tkl_vad_feed", 8 * 1024, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(&__tkl_vad_detect_task, "tkl_vad_detect", 4 * 1024, NULL, 4, NULL, 1);

    ESP_LOGD(TAG, "VAD task created");

    rt = tkl_mutex_create_init(&sg_vad_hdl.mutex_hdl);
    if (OPRT_OK != rt) {
        ESP_LOGE(TAG, "Failed to create VAD mutex");
        return rt;
    }
    ESP_LOGD(TAG, "VAD mutex created");
    
    ESP_LOGI(TAG, "VAD initialized successfully");

    sg_vad_hdl.is_init = 1;

    return rt;
}

OPERATE_RET tkl_vad_feed(uint8_t *data, uint32_t len)
{
    OPERATE_RET rt = OPRT_OK;

    if (0 == sg_vad_hdl.is_init) {
        return OPRT_COM_ERROR;
    }

    if (NULL == sg_vad_hdl.afe_data) {
        ESP_LOGE(TAG, "AFE data is NULL");
        return OPRT_COM_ERROR;
    }

    if (NULL == data || len == 0) {
        ESP_LOGE(TAG, "Data is NULL or length is 0");
        return OPRT_INVALID_PARM;
    }

    if (NULL == sg_vad_hdl.rb_hdl) {
        ESP_LOGE(TAG, "Ring buffer is NULL");
        return OPRT_COM_ERROR;
    }

    uint32_t free_size = tuya_ring_buff_free_size_get(sg_vad_hdl.rb_hdl);
    if (free_size < len) {
        // ESP_LOGW(TAG, "Ring buffer is full, discarding %ld bytes", len);
        tuya_ring_buff_discard(sg_vad_hdl.rb_hdl, len);
    }
    // ESP_LOGI(TAG, "Writing %ld bytes to ring buffer", len);
    tuya_ring_buff_write(sg_vad_hdl.rb_hdl, data, len);

    return OPRT_COM_ERROR;
}

TKL_VAD_STATUS_T tkl_vad_get_status(void)
{
    return sg_vad_hdl.vad_status;
}

OPERATE_RET tkl_vad_start(void)
{
    OPERATE_RET rt = OPRT_OK;

    if (NULL == sg_vad_hdl.mutex_hdl) {
        ESP_LOGE(TAG, "VAD mutex is NULL");
        return OPRT_COM_ERROR;
    }

    if (sg_vad_hdl.task_status == VAD_TASK_STATUS_WORKING) {
        ESP_LOGW(TAG, "VAD task is already working");
        return OPRT_OK;
    }

    tkl_mutex_lock(sg_vad_hdl.mutex_hdl);
    TKL_VAD_TASK_STAT_CHANGE(VAD_TASK_STATUS_WORKING);
    tkl_mutex_unlock(sg_vad_hdl.mutex_hdl);

    return rt;
}

OPERATE_RET tkl_vad_stop(void)
{
    OPERATE_RET rt = OPRT_OK;

    if (NULL == sg_vad_hdl.mutex_hdl) {
        ESP_LOGE(TAG, "VAD mutex is NULL");
        return OPRT_COM_ERROR;
    }

    if (sg_vad_hdl.task_status == VAD_TASK_STATUS_STOP) {
        ESP_LOGW(TAG, "VAD task is already stopped");
        return OPRT_OK;
    }

    tkl_mutex_lock(sg_vad_hdl.mutex_hdl);
    TKL_VAD_TASK_STAT_CHANGE(VAD_TASK_STATUS_STOP);
    tkl_mutex_unlock(sg_vad_hdl.mutex_hdl);

    return rt;
}

OPERATE_RET tkl_vad_deinit(void)
{
    OPERATE_RET rt = OPRT_OK;

    return rt;
}
