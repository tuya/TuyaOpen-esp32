/**
* @file tkl_asr.h
* @brief Common process - Automatic Speech Recognition
* @version 0.1
* @date 2021-08-18
*
* @copyright Copyright 2021-2030 Tuya Inc. All Rights Reserved.
*
*/
#ifndef __TKL_ASR_H__
#define __TKL_ASR_H__


#include "tuya_cloud_types.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TKL_ASR_WAKEUP_WORD_UNKNOWN,
    TKL_ASR_WAKEUP_NIHAO_TUYA,
    TKL_ASR_WAKEUP_NIHAO_XIAOZHI,
    TKL_ASR_WAKEUP_HEY_TUYA,
    TKL_ASR_WAKEUP_SMARTLIFE, 
    TKL_ASR_WAKEUP_ZHINENGGUANJIA,
    TKL_ASR_WAKEUP_XIAOZHI_TONGXUE,
    TKL_ASR_WAKEUP_XIAOZHI_GUANJIA,
    TKL_ASR_WAKEUP_XIAOAI_XIAOAI,
    TKL_ASR_WAKEUP_XIAODU_XIAODU,
    TKL_ASR_WAKEUP_WORD_MAX,
} TKL_ASR_WAKEUP_WORD_E;

typedef void (*TKL_ASR_WAKEUP_CB)(TKL_ASR_WAKEUP_WORD_E wakeup_word);

OPERATE_RET tkl_asr_init(void);

OPERATE_RET tkl_asr_reg_wakeup_cb(TKL_ASR_WAKEUP_CB wakeup_cb);

OPERATE_RET tkl_asr_enable(void);

OPERATE_RET tkl_asr_disable(void);

OPERATE_RET tkl_asr_deinit(void);


#ifdef __cplusplus
}
#endif

#endif
