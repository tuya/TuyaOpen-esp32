/**
 * @file tkl_asr.c
 * @brief tkl_asr module is used to 
 * @version 0.1
 * @date 2025-04-15
 */
#include "tuya_cloud_types.h"

#include "audio_afe.h"

#include "esp_err.h"
#include "esp_log.h"

#include "tkl_asr.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TAG "tkl_asr"

/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
static TKL_ASR_WAKEUP_CB sg_wakeup_cb = NULL;
static bool sg_is_asr_enable = false;

/***********************************************************
***********************function define**********************
***********************************************************/
static void __afe_wakeup_callback(void)
{
    if(true == sg_is_asr_enable && sg_wakeup_cb) {
        sg_wakeup_cb(TKL_ASR_WAKEUP_NIHAO_XIAOZHI);
    }
}

OPERATE_RET tkl_asr_init(void)
{
    auio_afe_register_wakeup_cb(__afe_wakeup_callback);

    return OPRT_OK;
}

OPERATE_RET tkl_asr_reg_wakeup_cb(TKL_ASR_WAKEUP_CB wakeup_cb)
{
    sg_wakeup_cb = wakeup_cb;

    return OPRT_OK;
}

OPERATE_RET tkl_asr_enable(void)
{
    sg_is_asr_enable = true;

    return OPRT_OK;
}

OPERATE_RET tkl_asr_disable(void)
{
    sg_is_asr_enable = false;

    return OPRT_OK;
}

OPERATE_RET tkl_asr_deinit(void)
{
    return OPRT_OK;
}





