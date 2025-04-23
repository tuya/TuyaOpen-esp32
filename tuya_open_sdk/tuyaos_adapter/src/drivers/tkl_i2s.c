/**
 * @file tkl_i2s.c
 * @brief tkl_i2s module is used to 
 * @version 0.1
 * @date 2025-04-15
 */

#include "tkl_i2s.h"

#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2s_pdm.h"
#include "driver/i2s_std.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TAG "tkl_i2s"

/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef struct {
    gpio_num_t clk_pin;
    gpio_num_t ws_pin;
    gpio_num_t data_pin;
} TKL_I2S_GPIO_CFG_T;

typedef struct {
    uint8_t is_init;
    TUYA_I2S_BASE_CFG_T config;

    i2s_chan_handle_t tx_hdl;
    i2s_chan_handle_t rx_hdl;
} TKL_I2S_HANDLE_T;

/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
#if defined(CONFIG_IDF_TARGET_ESP32S3)
static TKL_I2S_GPIO_CFG_T sg_i2s_gpio_cfg[TUYA_I2S_NUM_MAX] = {
    {GPIO_NUM_5, GPIO_NUM_4, GPIO_NUM_6},   // I2S0
    {GPIO_NUM_15, GPIO_NUM_16, GPIO_NUM_7}, // I2S1
};
#endif

static TKL_I2S_HANDLE_T sg_i2s_hdl[TUYA_I2S_NUM_MAX] = {0};

/***********************************************************
***********************function define**********************
***********************************************************/
OPERATE_RET tkl_i2s_init(TUYA_I2S_NUM_E i2s_num, const TUYA_I2S_BASE_CFG_T *i2s_config)
{
    OPERATE_RET rt = OPRT_OK;

    i2s_chan_handle_t *i2s_tx_hdl = NULL;
    i2s_chan_handle_t *i2s_rx_hdl = NULL;

    if (i2s_num >= TUYA_I2S_NUM_MAX) {
        ESP_LOGE(TAG, "Invalid i2s_num: %d", i2s_num);
        return OPRT_INVALID_PARM;
    }

    if (i2s_config == NULL) {
        ESP_LOGE(TAG, "Invalid i2s_config");
        return OPRT_INVALID_PARM;
    }

    if (i2s_config->mode & TUYA_I2S_MODE_SLAVE) {
        ESP_LOGE(TAG, "I2S slave mode is not supported");
        return OPRT_COM_ERROR;
    }

    memset(&sg_i2s_hdl[i2s_num], 0, sizeof(TKL_I2S_HANDLE_T));
    memcpy(&sg_i2s_hdl[i2s_num].config, i2s_config, sizeof(TUYA_I2S_BASE_CFG_T));

    // i2s handle set
    if (i2s_config->mode & TUYA_I2S_MODE_TX) {
        i2s_tx_hdl = &sg_i2s_hdl[i2s_num].tx_hdl;
    }
    if (i2s_config->mode & TUYA_I2S_MODE_RX) {
        i2s_rx_hdl = &sg_i2s_hdl[i2s_num].rx_hdl;
    }

    // pin set
    gpio_num_t clk_pin = sg_i2s_gpio_cfg[i2s_num].clk_pin;
    gpio_num_t ws_pin = sg_i2s_gpio_cfg[i2s_num].ws_pin;
    gpio_num_t di_pin = ((i2s_config->mode & TUYA_I2S_MODE_RX) ? (sg_i2s_gpio_cfg[i2s_num].data_pin) : (I2S_GPIO_UNUSED));
    gpio_num_t do_pin = ((i2s_config->mode & TUYA_I2S_MODE_TX) ? (sg_i2s_gpio_cfg[i2s_num].data_pin) : (I2S_GPIO_UNUSED));

    ESP_LOGE(TAG, "I2S config:"
                    "\n\t\tchannel: %d"
                    "\n\t\tmode: %d"
                    "\n\t\tsample_rate: %lu"
                    "\n\t\tbits_per_sample: %d"
                    "\n\t\tclk_pin: %d"
                    "\n\t\tws_pin: %d"
                    "\n\t\tdo_pin: %d"
                    "\n\t\tdi_pin: %d"
                    "\n\t\ttx_hdl: %p"
                    "\n\t\trx_hdl: %p",
        i2s_num, i2s_config->mode, i2s_config->sample_rate, i2s_config->bits_per_sample,
        clk_pin, ws_pin, do_pin, di_pin, i2s_tx_hdl, i2s_rx_hdl);

    i2s_chan_config_t i2s_chan_cfg = {
        .id = (i2s_port_t)i2s_num,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = 6,
        .dma_frame_num = 240,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    esp_err_t esp_rt = i2s_new_channel(&i2s_chan_cfg, i2s_tx_hdl, i2s_rx_hdl);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "i2s new channel failed");
        return OPRT_COM_ERROR;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)i2s_config->sample_rate,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
            #ifdef   I2S_HW_VERSION_2
                .ext_clk_freq_hz = 0,
            #endif

        },
        .slot_cfg = {
            .data_bit_width = (i2s_data_bit_width_t)i2s_config->bits_per_sample,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
            .ws_pol = false,
            .bit_shift = true,
            #ifdef   I2S_HW_VERSION_2
                .left_align = true,
                .big_endian = false,
                .bit_order_lsb = false
            #endif
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = clk_pin,
            .ws = ws_pin,
            .dout = do_pin,
            .din = di_pin,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    if (i2s_tx_hdl) {
        esp_rt = i2s_channel_init_std_mode(*i2s_tx_hdl, &std_cfg);
        if (esp_rt != ESP_OK) {
            // PR_ERR("i2s new channel failed");
            ESP_LOGE(TAG, "i2s new channel failed");
            return OPRT_COM_ERROR;
        }

        esp_rt = i2s_channel_enable(*i2s_tx_hdl);
        if (esp_rt != ESP_OK) {
            ESP_LOGE(TAG, "i2s tx channel enable failed");
            return OPRT_COM_ERROR;
        }
    }

    if (i2s_rx_hdl) {
        esp_rt = i2s_channel_init_std_mode(*i2s_rx_hdl, &std_cfg);
        if (esp_rt != ESP_OK) {
            // PR_ERR("i2s new channel failed");
            ESP_LOGE(TAG, "i2s new channel failed");
            return OPRT_COM_ERROR;
        }

        esp_rt = i2s_channel_enable(*i2s_rx_hdl);
        if (esp_rt != ESP_OK) {
            ESP_LOGE(TAG, "i2s rx channel enable failed");
            return OPRT_COM_ERROR;
        }
    }

    sg_i2s_hdl[i2s_num].is_init = 1;
    ESP_LOGI(TAG, "I2S init success, num: %d", i2s_num);

    return rt;
}


OPERATE_RET tkl_i2s_send(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len)
{
    OPERATE_RET rt = OPRT_OK;

    if (!sg_i2s_hdl[i2s_num].is_init) {
        ESP_LOGE(TAG, "I2S not initialized");
        return OPRT_COM_ERROR;
    }

    size_t bytes_written;
    esp_err_t esp_rt = i2s_channel_write(sg_i2s_hdl[i2s_num].tx_hdl, buff, len, &bytes_written, portMAX_DELAY);

    if (esp_rt != ESP_OK || bytes_written != len) {
        ESP_LOGE(TAG, "I2S write failed, esp_rt: %d, bytes_written: %zu", esp_rt, bytes_written);
        return OPRT_COM_ERROR;
    }

    return rt;
}

int tkl_i2s_recv(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len)
{
    if (!sg_i2s_hdl[i2s_num].is_init || !sg_i2s_hdl[i2s_num].rx_hdl) {
        ESP_LOGE(TAG, "I2S not initialized or rx handle is NULL");
        return 0;
    }

    size_t bytes_read = 0;
    esp_err_t esp_rt = i2s_channel_read(sg_i2s_hdl[i2s_num].rx_hdl, buff, len, &bytes_read, portMAX_DELAY);
    if (esp_rt != ESP_OK) {
        ESP_LOGE(TAG, "I2S read failed, esp_rt: %d", esp_rt);
        return 0;
    }

    return (int)bytes_read;
}

OPERATE_RET tkl_i2s_send_stop(TUYA_I2S_NUM_E i2s_num)
{
    OPERATE_RET rt = OPRT_OK;

    return rt;
}

OPERATE_RET tkl_i2s_recv_stop(TUYA_I2S_NUM_E i2s_num)
{
    OPERATE_RET rt = OPRT_OK;

    return rt;
}

OPERATE_RET tkl_i2s_deinit(TUYA_I2S_NUM_E i2s_num)
{
    OPERATE_RET rt = OPRT_OK;

    return rt;
}
