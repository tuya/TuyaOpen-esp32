/**
 * @file netif_esp.h
 * @brief netif 实现（esp_netif/esp_wifi 事件）对外接口——供 tuyaopen/tal_ops.c
 *        组装 TAL_NETIF_OPS_T 卡使用（esp32 平台无 tkl_wired，此卡非薄卡）。
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */
#ifndef __NETIF_ESP_H__
#define __NETIF_ESP_H__

#include "tal_netif_ops.h"

/* 取当前 IP（未就绪返回错误码） */
OPERATE_RET tal_netif_esp_get_ip(NW_IP_S *ip);

/* 注册链路状态回调（注册时先按当前状态立即回调一次） */
OPERATE_RET tal_netif_esp_set_link_cb(TAL_NETIF_LINK_CHANGE_CB cb);

#endif /* __NETIF_ESP_H__ */
