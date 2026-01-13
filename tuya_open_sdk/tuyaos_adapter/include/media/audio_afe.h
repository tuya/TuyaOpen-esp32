/**
 * @file audio_afe.h
 * @version 0.1
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#ifndef __AUDIO_AFE_H__
#define __AUDIO_AFE_H__

#include "tuya_cloud_types.h"
#include "esp_vad.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
************************macro define************************
***********************************************************/


/***********************************************************
***********************typedef define***********************
***********************************************************/
typedef void (*AUDIO_AFE_WAKEUP_CB)(void);
typedef void (*AUDIO_AFE_VAD_CB)(vad_state_t state);

/***********************************************************
********************function declaration********************
***********************************************************/
OPERATE_RET audio_afe_processor_init(void);

void auio_afe_processor_feed(uint8_t *data, int len);

void auio_afe_processor_start(void);

void auio_afe_processor_end(void);

void auio_afe_register_wakeup_cb(AUDIO_AFE_WAKEUP_CB cb);

void auio_afe_register_vad_cb(AUDIO_AFE_VAD_CB cb);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_AFE_H__ */
