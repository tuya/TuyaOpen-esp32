/*
 * SPDX-FileCopyrightText: 2021-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "sdkconfig.h"

/* Example configurations */
#define EXAMPLE_RECV_BUF_SIZE   (2400)
#define EXAMPLE_SAMPLE_RATE     (16000)
#define EXAMPLE_VOICE_VOLUME    (80)

/* I2C port and GPIOs */
#define I2C_NUM         (0)
#define I2C_SCL_IO      (GPIO_NUM_48)
#define I2C_SDA_IO      (GPIO_NUM_47)

/* I2S port and GPIOs */
#define I2S_NUM         (0)
#define I2S_MCK_IO      (GPIO_NUM_10)
#define I2S_BCK_IO      (GPIO_NUM_11)
#define I2S_WS_IO       (GPIO_NUM_12)

#define I2S_DO_IO       (GPIO_NUM_16)
#define I2S_DI_IO       (GPIO_NUM_15)

#define GPIO_OUTPUT_PA  (GPIO_NUM_NC)

#define AUDIO_CODEC_DMA_DESC_NUM 6
#define AUDIO_CODEC_DMA_FRAME_NUM 240
#define AUDIO_CODEC_ES8311_ADDR ES8311_CODEC_DEFAULT_ADDR
#define AUDIO_CODEC_ES7243E_ADDR  (0x14)

OPERATE_RET tkl_i2s_watcher_init(TUYA_I2S_NUM_E i2s_num, const TUYA_I2S_BASE_CFG_T *i2s_config);
OPERATE_RET tkl_i2s_watcher_send(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len);
int tkl_i2s_watcher_recv(TUYA_I2S_NUM_E i2s_num, void *buff, uint32_t len);
