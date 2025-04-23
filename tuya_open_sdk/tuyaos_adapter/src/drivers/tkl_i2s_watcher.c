#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2s_std.h"
#include "esp_system.h"
#include "esp_check.h"

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_codec_dev.h"
#include "esp_codec_dev_defaults.h"

#include "tuya_cloud_types.h"
#include "tkl_i2s_watcher.h"

static const char *TAG = "tkl_i2s_es8311";

static i2s_chan_handle_t tx_handle_ = NULL;
static i2s_chan_handle_t rx_handle_ = NULL;
static int input_sample_rate_ = EXAMPLE_SAMPLE_RATE;
static int output_sample_rate_ = EXAMPLE_SAMPLE_RATE;
static gpio_num_t pa_pin_ = GPIO_OUTPUT_PA;
static i2c_master_bus_handle_t codec_i2c_bus_ = NULL;
static audio_codec_data_if_t* data_if_ = NULL;
static audio_codec_ctrl_if_t* ctrl_if_ = NULL;
static audio_codec_ctrl_if_t* in_ctrl_if_ = NULL;
static audio_codec_gpio_if_t* gpio_if_ = NULL;
static audio_codec_if_t* codec_if_ = NULL;
static audio_codec_if_t* in_codec_if_ = NULL;
static esp_codec_dev_handle_t output_dev_ = NULL;
static esp_codec_dev_handle_t input_dev_ = NULL;

static void InitializeCodecI2c() {
    // Initialize I2C peripheral
    i2c_master_bus_config_t i2c_bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_IO,
        .scl_io_num = I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = 1,
        },
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_cfg, &codec_i2c_bus_));
}

static void EnableInput(bool enable) {
    if (enable) {
        esp_codec_dev_sample_info_t fs = {
            .bits_per_sample = 16,
            // .channel = 1,
            .channel = 2,  // debug
            .channel_mask = ESP_CODEC_DEV_MAKE_CHANNEL_MASK(1),
            .sample_rate = (uint32_t)input_sample_rate_,
            .mclk_multiple = 0,
        };
        ESP_ERROR_CHECK(esp_codec_dev_open(input_dev_, &fs));
        ESP_ERROR_CHECK(esp_codec_dev_set_in_gain(input_dev_, 27.0));
    } else {
        ESP_ERROR_CHECK(esp_codec_dev_close(input_dev_));
    }
}

static void EnableOutput(bool enable)
{
    if (enable) {
        // Play 16bit 1 channel
        esp_codec_dev_sample_info_t fs = {
            .bits_per_sample = 16,
            .channel = 1,
            .channel_mask = 0,
            .sample_rate = (uint32_t)output_sample_rate_,
            .mclk_multiple = 0,
        };
        ESP_ERROR_CHECK(esp_codec_dev_open(output_dev_, &fs));
        ESP_ERROR_CHECK(esp_codec_dev_set_out_vol(output_dev_, EXAMPLE_VOICE_VOLUME));
        if (pa_pin_ != GPIO_NUM_NC) {
            gpio_set_level(pa_pin_, 1);
        }
    } else {
        ESP_ERROR_CHECK(esp_codec_dev_close(output_dev_));
        if (pa_pin_ != GPIO_NUM_NC) {
            gpio_set_level(pa_pin_, 0);
        }
    }
}

static void CreateDuplexChannels(
    gpio_num_t mclk, gpio_num_t bclk, gpio_num_t ws,
    gpio_num_t dout, gpio_num_t din)
{
    i2s_chan_config_t chan_cfg = {
        .id = I2S_NUM_0,
        .role = I2S_ROLE_MASTER,
        .dma_desc_num = AUDIO_CODEC_DMA_DESC_NUM,
        .dma_frame_num = AUDIO_CODEC_DMA_FRAME_NUM,
        .auto_clear_after_cb = true,
        .auto_clear_before_cb = false,
        .intr_priority = 0,
    };
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_handle_, &rx_handle_));

    i2s_std_config_t std_cfg = {
        .clk_cfg = {
            .sample_rate_hz = (uint32_t)EXAMPLE_SAMPLE_RATE,
            .clk_src = I2S_CLK_SRC_DEFAULT,
            .mclk_multiple = I2S_MCLK_MULTIPLE_256,
			#ifdef   I2S_HW_VERSION_2
				.ext_clk_freq_hz = 0,
			#endif
        },
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_BOTH,
            .ws_width = I2S_DATA_BIT_WIDTH_16BIT,
            .ws_pol = false,
            .bit_shift = true,
            #ifdef   I2S_HW_VERSION_2
                .left_align = true,
                .big_endian = false,
                .bit_order_lsb = false
            #endif
        },
        .gpio_cfg = {
            .mclk = mclk,
            .bclk = bclk,
            .ws = ws,
            .dout = dout,
            .din = din,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false
            }
        }
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_handle_, &std_cfg));
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;
    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_handle_, &std_cfg));
    ESP_LOGI(TAG, "Duplex channels created");
}

OPERATE_RET tkl_i2s_watcher_init(TUYA_I2S_NUM_E i2s_num, const TUYA_I2S_BASE_CFG_T *i2s_config)
{
    OPERATE_RET rt = OPRT_OK;
    void* i2c_master_handle = NULL;
    i2c_port_t i2c_port = I2C_NUM_0;
    uint8_t es8311_addr = AUDIO_CODEC_ES8311_ADDR;
    bool use_mclk = true;

    ESP_LOGI(TAG, ">>>>>>>>mclk=%d, bclk=%d, ws=%d, dout=%d, din=%d",
             I2S_MCK_IO, I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO);
    ESP_LOGI(TAG, ">>>>>>>>i2c_port=%d, es8311_addr=%d, pa_pin_=%d",
             i2c_port, es8311_addr, pa_pin_);

    InitializeCodecI2c();
    i2c_master_handle = (void*)codec_i2c_bus_;
    CreateDuplexChannels(I2S_MCK_IO, I2S_BCK_IO, I2S_WS_IO,
        I2S_DO_IO, I2S_DI_IO);

    // Do initialize of related interface: data_if, ctrl_if and gpio_if
    audio_codec_i2s_cfg_t i2s_cfg = {
        .port = I2S_NUM_0,
        .rx_handle = rx_handle_,
        .tx_handle = tx_handle_,
    };
    data_if_ = audio_codec_new_i2s_data(&i2s_cfg);
    assert(data_if_ != NULL);

    // Output
    audio_codec_i2c_cfg_t i2c_cfg = {
        .port = i2c_port,
        .addr = es8311_addr,
        .bus_handle = i2c_master_handle,
    };
    ctrl_if_ = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(ctrl_if_ != NULL);

    gpio_if_ = audio_codec_new_gpio();
    assert(gpio_if_ != NULL);

    es8311_codec_cfg_t es8311_cfg = {};
    es8311_cfg.ctrl_if = ctrl_if_;
    es8311_cfg.gpio_if = gpio_if_;
    es8311_cfg.codec_mode = ESP_CODEC_DEV_WORK_MODE_DAC;
    es8311_cfg.pa_pin = pa_pin_;
    es8311_cfg.use_mclk = use_mclk;
    es8311_cfg.hw_gain.pa_voltage = 5.0;
    es8311_cfg.hw_gain.codec_dac_voltage = 3.3;
    codec_if_ = es8311_codec_new(&es8311_cfg);
    assert(codec_if_ != NULL);

    esp_codec_dev_cfg_t dev_cfg = {
        .dev_type = ESP_CODEC_DEV_TYPE_OUT,
        .codec_if = codec_if_,
        .data_if = data_if_,
    };
    output_dev_ = esp_codec_dev_new(&dev_cfg);
    assert(output_dev_ != NULL);

    // Input
    i2c_cfg.addr = ((uint8_t)AUDIO_CODEC_ES7243E_ADDR) << 1;
    in_ctrl_if_ = audio_codec_new_i2c_ctrl(&i2c_cfg);
    assert(in_ctrl_if_ != NULL);

    es7243e_codec_cfg_t es7243e_cfg = {};
    es7243e_cfg.ctrl_if = in_ctrl_if_;
    in_codec_if_ = es7243e_codec_new(&es7243e_cfg);
    assert(in_codec_if_ != NULL);

    dev_cfg.dev_type = ESP_CODEC_DEV_TYPE_IN;
    dev_cfg.codec_if = in_codec_if_;
    input_dev_ = esp_codec_dev_new(&dev_cfg);
    assert(input_dev_ != NULL);

    esp_codec_set_disable_when_closed(output_dev_, false);
    esp_codec_set_disable_when_closed(input_dev_, false);
    ESP_LOGI(TAG, "Es8311AudioCodec initialized");
    EnableInput(true);
    EnableOutput(true);

    return rt;
}

OPERATE_RET tkl_i2s_watcher_send(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len)
{
    // len -> data len
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_write(output_dev_, (void*)buff, len * sizeof(int16_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s write failed. %d", ret);
        return OPRT_COM_ERROR;
    }

    return OPRT_OK;
}

int tkl_i2s_watcher_recv(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len)
{
    // len -> data len
    esp_err_t ret = ESP_OK;
    ret = esp_codec_dev_read(input_dev_, (void*)buff, len * sizeof(int16_t));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s read failed. %d", ret);
        return 0;
    }

    return (int)len;
}
