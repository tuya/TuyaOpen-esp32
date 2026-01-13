/**
 * @file tkl_vad.c
 * @brief tkl_vad module is used to 
 * @version 0.1
 * @date 2025-04-15
 */

#include "tuya_cloud_types.h"

#include "audio_afe.h"

#include "esp_err.h"
#include "esp_log.h"

#include "tkl_vad.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TAG "tkl_vad"


/***********************************************************
***********************typedef define***********************
***********************************************************/

/***********************************************************
********************function declaration********************
***********************************************************/

/***********************************************************
***********************variable define**********************
***********************************************************/
static TKL_VAD_STATUS_T sg_vad_state = TKL_VAD_STATUS_NONE;

/***********************************************************
***********************function define**********************
***********************************************************/
void __get_vad_state_cb(vad_state_t state)
{
    if(state == VAD_SPEECH) {
        sg_vad_state = TKL_VAD_STATUS_SPEECH;
    }else {
        sg_vad_state = TKL_VAD_STATUS_NONE;
    }
}

OPERATE_RET tkl_vad_init(TKL_VAD_CONFIG_T *config)
{
    OPERATE_RET rt = OPRT_OK;

    auio_afe_register_vad_cb(__get_vad_state_cb);

    return rt;
}

TKL_VAD_STATUS_T tkl_vad_get_status(void)
{
    return sg_vad_state;
}

OPERATE_RET tkl_vad_start(void)
{
    OPERATE_RET rt = OPRT_OK;

    auio_afe_processor_start();

    return rt;
}

OPERATE_RET tkl_vad_stop(void)
{
    OPERATE_RET rt = OPRT_OK;

    auio_afe_processor_end();

    sg_vad_state = TKL_VAD_STATUS_NONE;

    return rt;
}

OPERATE_RET tkl_vad_deinit(void)
{
    OPERATE_RET rt = OPRT_OK;

    return rt;
}

OPERATE_RET tkl_vad_set_threshold(TKL_AUDIO_VAD_THRESHOLD_E level)
{
    return OPRT_OK;
}