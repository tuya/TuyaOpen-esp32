/**
 * @file tal_netif_ops.h
 * @brief 契约层（interface）：tal_netif 平台 ops 卡类型 + 链路状态类型。
 *
 * 分层定位与依赖方向见 tal_api_ops.h 的文件头。本头只含类型：
 *  - `TAL_NETIF_LINK_STAT_E` / `TAL_NETIF_LINK_CHANGE_CB`（原在 core 的 tal_netif.h，
 *    随契约一并迁到本层；core 的 tal_netif.h 反向 include 本头以保持其 API 面不变）
 *  - `TAL_NETIF_OPS_T`（卡类型）
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */
#ifndef __TAL_NETIF_OPS_H__
#define __TAL_NETIF_OPS_H__

#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 网络接口链路状态 */
typedef enum {
    TAL_NETIF_LINK_DOWN = 0, ///< 链路 down（无非回环接口 up）
    TAL_NETIF_LINK_UP,       ///< 链路 up（至少一个非回环接口 up）
} TAL_NETIF_LINK_STAT_E;

/**
 * @brief 链路状态变化回调
 *
 * @param[out] status 当前链路状态
 */
typedef void (*TAL_NETIF_LINK_CHANGE_CB)(TAL_NETIF_LINK_STAT_E status);

/**
 * @brief tal_netif 平台 ops：成员与 port 的 netif 实现一一对应
 */
typedef struct {
    /* 取本机网络接口 IP；见 tal_netif.h 的 tal_netif_get_ip */
    OPERATE_RET (*get_ip)(NW_IP_S *ip);
    /* 注册链路状态变化回调；见 tal_netif.h 的 tal_netif_set_link_cb */
    OPERATE_RET (*set_link_cb)(TAL_NETIF_LINK_CHANGE_CB cb);
} TAL_NETIF_OPS_T;

#ifdef __cplusplus
}
#endif

#endif /* __TAL_NETIF_OPS_H__ */
