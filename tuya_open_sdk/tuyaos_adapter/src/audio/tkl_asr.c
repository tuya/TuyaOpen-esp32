/**
 * @file tkl_asr.c
 * @brief tkl_asr module is used to 
 * @version 0.1
 * @date 2025-04-15
 */

#include "tkl_asr.h"
#include "tkl_thread.h"
#include "tkl_queue.h"
#include "tkl_memory.h"
#include "tkl_system.h"

#include "tuya_ringbuf.h"

#include "esp_err.h"
#include "esp_log.h"
#include <esp_afe_sr_models.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TAG "tkl_asr"

#define FEED_RB_SIZE (8 * 1024)

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    TKL_ASR_WAKEUP_WORD_E wakeup_word[TKL_ASR_WAKEUP_WORD_MAX];
    uint8_t arr_cnt;

    esp_afe_sr_iface_t* afe_iface;
    esp_afe_sr_data_t* afe_data;

    uint32_t process_uint_size;

    TUYA_RINGBUFF_T rb_hdl;

    TKL_QUEUE_HANDLE queue_hdl;
} TKL_ASR_DATA_T;

static TKL_ASR_WAKEUP_WORD_E sg_cur_wakeup_word = TKL_ASR_WAKEUP_WORD_UNKNOWN;

/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
static TKL_ASR_DATA_T sg_asr_hdl = {
    .wakeup_word = {TKL_ASR_WAKEUP_WORD_UNKNOWN},
    .arr_cnt = 0,
    .afe_iface = NULL,
    .afe_data = NULL,
    .process_uint_size = 0,
};

/***********************************************************
***********************function define**********************
***********************************************************/
static void __tkl_asr_feed_task(void *arg)
{
    uint8_t *feed_data = tkl_system_malloc(sg_asr_hdl.process_uint_size);
    assert(feed_data);

    for (;;) {
        uint32_t used_size = tuya_ring_buff_used_size_get(sg_asr_hdl.rb_hdl);
        if (used_size < sg_asr_hdl.process_uint_size) {
            // wait for data
            // ESP_LOGI(TAG, "Waiting for data...");
            tkl_system_sleep(10);
            continue;
        }

        memset(feed_data, 0, sg_asr_hdl.process_uint_size);
        // Read data from ring buffer
        tuya_ring_buff_read(sg_asr_hdl.rb_hdl, feed_data, sg_asr_hdl.process_uint_size);
        // ESP_LOGI(TAG, "feed data size: %ld", sg_asr_hdl.process_uint_size);
        sg_asr_hdl.afe_iface->feed(sg_asr_hdl.afe_data, (const int16_t *)feed_data);
        tkl_system_sleep(10);
    }
}

static void __tkl_asr_detect_task(void *arg)
{
    for (;;) {
        afe_fetch_result_t* res = sg_asr_hdl.afe_iface->fetch_with_delay(sg_asr_hdl.afe_data, portMAX_DELAY);
        if (!res || res->ret_value == ESP_FAIL) {
            ESP_LOGE(TAG, "fetch error!");
            sg_cur_wakeup_word = TKL_ASR_WAKEUP_WORD_UNKNOWN;
            continue;
        }
        // ESP_LOGI(TAG, "vad state: %d", res->vad_state);

        // Set wake word status
        if (res->wakeup_state == WAKENET_DETECTED) {
            ESP_LOGI(TAG, "wakeword detected");
            ESP_LOGI(TAG, "model index:%d, word index:%d", res->wakenet_model_index, res->wake_word_index);
            ESP_LOGI(TAG, "-----------LISTENING-----------");

            if (1 == res->wakenet_model_index) {
                sg_cur_wakeup_word = TKL_ASR_WAKEUP_NIHAO_XIAOZHI;
            } else {
                sg_cur_wakeup_word = TKL_ASR_WAKEUP_WORD_UNKNOWN;
            }
        }
    }
}

static OPERATE_RET __tkl_asr_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    // Initialize AFE (Audio Front End) settings here

    // Single microphone, non-real-time
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

    afe_config_t* afe_config = afe_config_init("M", models, AFE_TYPE_SR, AFE_MODE_HIGH_PERF);
    afe_config->aec_init = false;
    afe_config->afe_perferred_core = 1;
    afe_config->afe_perferred_priority = 1;
    afe_config->memory_alloc_mode = AFE_MEMORY_ALLOC_MORE_PSRAM;

    sg_asr_hdl.afe_iface = esp_afe_handle_from_config(afe_config);
    if (NULL == sg_asr_hdl.afe_iface) {
        ESP_LOGE(TAG, "Failed to get afe handle");
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

    sg_asr_hdl.afe_data = sg_asr_hdl.afe_iface->create_from_config(afe_config);
    if (NULL == sg_asr_hdl.afe_data) {
        ESP_LOGE(TAG, "Failed to create afe data");
        rt = OPRT_COM_ERROR;
        goto __EXIT;
    }

__EXIT:
    if (NULL == afe_config) {
        afe_config_free(afe_config);
        afe_config = NULL;
    }

    return 0;
}

OPERATE_RET tkl_asr_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    // Initialize ring buffer
    rt = tuya_ring_buff_create(FEED_RB_SIZE, OVERFLOW_STOP_TYPE, &sg_asr_hdl.rb_hdl);
    if (OPRT_OK != rt) {
        ESP_LOGE(TAG, "Failed to create ring buffer");
        return rt;
    }

    __tkl_asr_init();

    tkl_queue_create_init(&sg_asr_hdl.queue_hdl, sizeof(TKL_ASR_WAKEUP_WORD_E), 3);

    // get required data size
    int chunksize = sg_asr_hdl.afe_iface->get_feed_chunksize(sg_asr_hdl.afe_data);
    // TODO: support multi-channel
    int feed_channel = 1;
    sg_asr_hdl.process_uint_size = chunksize * sizeof(int16_t) * feed_channel;
    ESP_LOGI(TAG, "Process unit size: %ld", sg_asr_hdl.process_uint_size);

    xTaskCreatePinnedToCore(&__tkl_asr_feed_task, "tkl_asr_feed", 2 * 1024, NULL, 4, NULL, 0);
    xTaskCreatePinnedToCore(&__tkl_asr_detect_task, "tkl_asr_detect", 4 * 1024, NULL, 4, NULL, 1);

    ESP_LOGI(TAG, "ASR initialized successfully");

    return OPRT_OK;
}

OPERATE_RET tkl_asr_wakeup_word_config(TKL_ASR_WAKEUP_WORD_E *wakeup_word_arr, uint8_t arr_cnt)
{
    OPERATE_RET rt = OPRT_OK;

    if (NULL == wakeup_word_arr || arr_cnt == 0) {
        ESP_LOGE(TAG, "Invalid wakeup word array");
        return OPRT_INVALID_PARM;
    }

    if (arr_cnt > TKL_ASR_WAKEUP_WORD_MAX) {
        ESP_LOGE(TAG, "Wakeup word array count exceeds max");
        return OPRT_INVALID_PARM;
    }

    memset(sg_asr_hdl.wakeup_word, TKL_ASR_WAKEUP_WORD_UNKNOWN, sizeof(TKL_ASR_WAKEUP_WORD_E) * TKL_ASR_WAKEUP_WORD_MAX);
    sg_asr_hdl.arr_cnt = arr_cnt;
    for (uint8_t i = 0; i < arr_cnt; i++) {
        sg_asr_hdl.wakeup_word[i] = wakeup_word_arr[i];
        ESP_LOGD(TAG, "wakeup word[%d]: %d", i, sg_asr_hdl.wakeup_word[i]);
    }

    return rt;
}

uint32_t tkl_asr_get_process_uint_size(void)
{
    return sg_asr_hdl.process_uint_size;
}

TKL_ASR_WAKEUP_WORD_E tkl_asr_recognize_wakeup_word(uint8_t *data, uint32_t len)
{
    OPERATE_RET rt = OPRT_OK;
    TKL_ASR_WAKEUP_WORD_E rt_wakeup_word = TKL_ASR_WAKEUP_WORD_UNKNOWN;

    if (NULL == data || len == 0) {
        ESP_LOGE(TAG, "Invalid data");
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    if (NULL == sg_asr_hdl.rb_hdl) {
        ESP_LOGE(TAG, "Ring buffer not initialized");
        return TKL_ASR_WAKEUP_WORD_UNKNOWN;
    }

    uint32_t free_size = tuya_ring_buff_free_size_get(sg_asr_hdl.rb_hdl);
    if (free_size < len) {
        // ESP_LOGW(TAG, "Ring buffer is full, discarding %ld bytes", len);
        tuya_ring_buff_discard(sg_asr_hdl.rb_hdl, len);
    }
    // ESP_LOGI(TAG, "Writing %ld bytes to ring buffer", len);
    tuya_ring_buff_write(sg_asr_hdl.rb_hdl, data, len);

    rt_wakeup_word = sg_cur_wakeup_word;
    sg_cur_wakeup_word = TKL_ASR_WAKEUP_WORD_UNKNOWN;

    return rt_wakeup_word;
}

OPERATE_RET tkl_asr_deinit(void)
{
    OPERATE_RET rt = OPRT_OK;

    return rt;
}