/**
* @file tkl_init_wired.c
* @brief Common process - tkl init wired description
* @version 0.1
* @date 2021-08-06
*
* @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
*
*/
//有线
#include "tkl_init_wired.h"

// 指向有线网络描述符
const TKL_WIRED_DESC_T c_wired_desc = {
    .get_status         = tkl_wired_get_status,
    .set_status_cb      = tkl_wired_set_status_cb,
    .get_ip             = tkl_wired_get_ip,
    .get_mac            = tkl_wired_get_mac,
    .set_mac            = tkl_wired_set_mac,
};

/**
 * @brief register wired description to tuya object manage
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
/**
 * @brief 获取有线网络描述符
 * 
 * 该函数返回指向有线网络描述符的指针，描述符包含有线网络的相关配置信息。
 * 此函数为弱定义函数，允许用户在其他文件中重新定义实现。
 * 
 * @return TKL_WIRED_DESC_T* 返回有线网络描述符指针
 */
TUYA_WEAK_ATTRIBUTE TKL_WIRED_DESC_T* tkl_wired_desc_get()
{
    return (TKL_WIRED_DESC_T*)&c_wired_desc;
}

