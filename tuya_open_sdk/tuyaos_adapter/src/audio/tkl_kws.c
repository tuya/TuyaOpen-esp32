/**
 * @file tkl_kws.c
 * @brief tkl_kws module is used to 
 * @version 0.1
 * @date 2025-04-15
 */
#include "tuya_cloud_types.h"

#include "audio_afe.h"

#include "esp_err.h"
#include "esp_log.h"

#include "tkl_kws.h"

/***********************************************************
************************macro define************************
***********************************************************/
#define TAG "tkl_kws"

/***********************************************************
***********************typedef define***********************
***********************************************************/


/***********************************************************
********************function declaration********************
***********************************************************/


/***********************************************************
***********************variable define**********************
***********************************************************/
static TKL_KWS_WAKEUP_CB sg_wakeup_cb = NULL;
static bool sg_is_kws_enable = false;

/***********************************************************
***********************function define**********************
***********************************************************/
static void __afe_wakeup_callback(void)
{
    if(true == sg_is_kws_enable && sg_wakeup_cb) {
        sg_wakeup_cb(TKL_KWS_WAKEUP_NIHAO_XIAOZHI);
    }
}

OPERATE_RET tkl_kws_init(void)
{
    auio_afe_register_wakeup_cb(__afe_wakeup_callback);

    return OPRT_OK;
}

OPERATE_RET tkl_kws_reg_wakeup_cb(TKL_KWS_WAKEUP_CB wakeup_cb)
{
    sg_wakeup_cb = wakeup_cb;

    return OPRT_OK;
}

OPERATE_RET tkl_kws_enable(void)
{
    sg_is_kws_enable = true;

    return OPRT_OK;
}

OPERATE_RET tkl_kws_disable(void)
{
    sg_is_kws_enable = false;

    return OPRT_OK;
}

OPERATE_RET tkl_kws_deinit(void)
{
    return OPRT_OK;
}





