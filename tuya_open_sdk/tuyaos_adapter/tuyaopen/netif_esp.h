/**
 * @file netif_esp.h
 * @brief Netif implementation via esp_netif / esp_wifi events, used by
 *        tuyaopen-core_ops.c to assemble CORE_NETIF_OPS_T.
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */
#ifndef __NETIF_ESP_H__
#define __NETIF_ESP_H__

#include "core_netif_ops.h"

/* get the current IP; returns an error when no IP is available */
OPERATE_RET core_netif_esp_get_ip(NW_IP_S *ip);

/* register the link status callback; reports the current state immediately */
OPERATE_RET core_netif_esp_set_link_cb(CORE_NETIF_LINK_CHANGE_CB cb);

#endif /* __NETIF_ESP_H__ */
