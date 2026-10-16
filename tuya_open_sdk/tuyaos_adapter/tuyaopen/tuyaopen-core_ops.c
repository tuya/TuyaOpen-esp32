/**
 * @file tal_ops.c
 * @brief tuyaopen-core 的 ops 卡注册：TAL_API_OPS_T / TAL_NETWORK_OPS_T /
 *        TAL_BLUETOOTH_OPS_T / TAL_NETIF_OPS_T。
 *
 * 分层（C2）：core 的 tal_api.c / tal_network.c / tal_netif.c / tal_bluetooth.c 经
 * extern 引用这些卡实例完成分发。前两张是**薄卡**——成员直接绑定本平台
 * （tuya_open_sdk/tuyaos_adapter）的 tkl_* 实现；netif 卡不是薄卡：esp32 平台无
 * tkl_wired，取 IP/链路回调由本层 netif_esp.c（esp_netif/esp_wifi 事件）实现。
 *
 * 单独立文件（不写进平台实现文件）：平台侧 tkl_*.c 保持原样零改动。
 * 注：netif 卡与 WiFi 相关，须先完成 WiFi 初始化再注册回调（见 netif_esp.c 头注释）。
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 */

#include "tal_api_ops.h"     /* TAL_API_OPS_T */
#include "tal_network_ops.h" /* TAL_NETWORK_OPS_T */
#include "tal_bluetooth_ops.h" /* TAL_BLUETOOTH_OPS_T */
#include "tal_netif_ops.h"   /* TAL_NETIF_OPS_T */

#include "netif_esp.h" /* netif 卡实现（esp_netif 事件） */

#include "tkl_output.h"
#include "tkl_memory.h"
#include "tkl_system.h"
#include "tkl_semaphore.h"
#include "tkl_mutex.h"
#include "tkl_thread.h"
#include "tkl_queue.h"
#include "tkl_network.h"

/* ---------------------------------------------------------------------------
 * TAL_API_OPS_T：log / 内存 / system / 信号量 / 互斥 / 线程 / 队列
 * --------------------------------------------------------------------------- */
TAL_API_OPS_T tal_api_ops_platform = {
    //! log output
    .log_output = tkl_log_output,
    //! memory
    .malloc = tkl_system_malloc,
    .free   = tkl_system_free,
    //! system
    .sleep           = tkl_system_sleep,
    .reset           = tkl_system_reset,
    .get_millisecond = tkl_system_get_millisecond,
    .get_random      = tkl_system_get_random,
    //! semaphore
    .semaphore_create_init = tkl_semaphore_create_init,
    .semaphore_wait        = tkl_semaphore_wait,
    .semaphore_post        = tkl_semaphore_post,
    .semaphore_release     = tkl_semaphore_release,
    //! mutex
    .mutex_create_init = tkl_mutex_create_init,
    .mutex_lock        = tkl_mutex_lock,
    .mutex_unlock      = tkl_mutex_unlock,
    .mutex_release     = tkl_mutex_release,
    //! thread
    .thread_create        = tkl_thread_create,
    .thread_release       = tkl_thread_release,
    .thread_set_self_name = tkl_thread_set_self_name,
    .thread_is_self       = tkl_thread_is_self,
    .thread_get_id        = tkl_thread_get_id,
    //! queue
    .queue_create_init = tkl_queue_create_init,
    .queue_post        = tkl_queue_post,
    .queue_fetch       = tkl_queue_fetch,
    .queue_free        = tkl_queue_free,
#if defined(ENABLE_EXT_RAM) && (ENABLE_EXT_RAM == 1)
    //! ex ram（外部 RAM，如 PSRAM）：复用平台既有 tkl_system_psram_*（Linux 无独立外部
    //! RAM，平台实现退化为普通内存）；宏隔离与 core 的 TAL_API_OPS_T 一致
    .ex_ram_malloc = tkl_system_psram_malloc,
    .ex_ram_free   = tkl_system_psram_free,
#endif
};

/* ---------------------------------------------------------------------------
 * TAL_NETWORK_OPS_T：标准网络卡（socket 全量绑定 tkl_net_*）；
 * 局域网/对端成员按 core 侧同名宏门控（core 的开关宏不传播到平台编译单元，
 * 故此处逐个判断）
 * --------------------------------------------------------------------------- */
TAL_NETWORK_OPS_T tal_network_ops_platform = {
    .name = "tkl",
    .type = TAL_NET_TYPE_PLATFORM,
    .ipaddr = 0,
    .ops =
        {
            .get_errno = tkl_net_get_errno,
            .fd_set = tkl_net_fd_set,
            .fd_isset = tkl_net_fd_isset,
            .fd_zero = tkl_net_fd_zero,
            .select = tkl_net_select,
            .get_nonblock = tkl_net_get_nonblock,
            .set_block = tkl_net_set_block,
            .close = tkl_net_close,
            .socket_create = tkl_net_socket_create,
            .connect = tkl_net_connect,
            .bind = tkl_net_bind,
            .send = tkl_net_send,
            .recv = tkl_net_recv,
            .set_timeout = tkl_net_set_timeout,
            .set_reuse = tkl_net_set_reuse,
            .disable_nagle = tkl_net_disable_nagle,
            .gethostbyname = tkl_net_gethostbyname,
            .set_keepalive = tkl_net_set_keepalive,
            .str2addr = tkl_net_str2addr,
            .addr2str = tkl_net_addr2str,
            .setsockopt = tkl_net_setsockopt,
#if defined(ENABLE_LOCAL_LINK) || defined(ENABLE_TUYA_P2P) || defined(ENABLE_AI_COMPONENTS)
            /* 局域网/对端服务成员 */
            .listen = tkl_net_listen,
            .accept = tkl_net_accept,
#endif
#if defined(ENABLE_LOCAL_LINK) || defined(ENABLE_TUYA_P2P)
            .send_to = tkl_net_send_to,
            .recvfrom = tkl_net_recvfrom,
#endif
#if defined(ENABLE_LOCAL_LINK)
            /* 局域网协议专用成员（tuya_lan.c 收帧） */
            .recv_nd_size = tkl_net_recv_nd_size,
#endif
#if defined(ENABLE_TUYA_P2P)
            /* p2p 专用成员 */
            .fd_clear = tkl_net_fd_clear,
            .shutdown = tkl_net_shutdown,
            .set_bufsize = tkl_net_set_bufsize,
            .getsockname = tkl_net_getsockname,
            .getpeername = tkl_net_getpeername,
#endif
        },
};

/* ---------------------------------------------------------------------------
 * TAL_BLUETOOTH_OPS_T：空卡（成员全 NULL）——本仓 esp32 侧尚未接 tkl_bt，
 * core 侧分发检测到 NULL 统一优雅降级（OPRT_NOT_SUPPORTED）。
 * --------------------------------------------------------------------------- */
TAL_BLUETOOTH_OPS_T tal_bluetooth_ops_platform = {
    /* stack */
    .stack_init               = NULL,
    .stack_deinit             = NULL,
    .stack_gatt_link          = NULL,
    /* 事件注册 */
    .gap_callback_register    = NULL,
    .gatt_callback_register   = NULL,
    /* GAP */
    .gap_addr_set             = NULL,
    .gap_address_get          = NULL,
    .gap_adv_start            = NULL,
    .gap_adv_stop             = NULL,
    .gap_adv_rsp_data_set     = NULL,
    .gap_adv_rsp_data_update  = NULL,
    .gap_scan_start           = NULL,
    .gap_scan_stop            = NULL,
    .gap_connect              = NULL,
    .gap_disconnect           = NULL,
    .gap_conn_param_update    = NULL,
    .gap_rssi_get             = NULL,
    /* GATT server */
    .gatts_service_add        = NULL,
    .gatts_value_set          = NULL,
    .gatts_value_notify       = NULL,
    .gatts_exchange_mtu_reply = NULL,
    /* GATT client */
    .gattc_all_service_discovery = NULL,
    .gattc_all_char_discovery    = NULL,
    .gattc_char_desc_discovery   = NULL,
    .gattc_write_without_rsp     = NULL,
    .gattc_read                  = NULL,
    .gattc_exchange_mtu_request  = NULL,
};

/* ---------------------------------------------------------------------------
 * TAL_NETIF_OPS_T：取 IP / 链路状态（esp_netif 事件实现，见 netif_esp.c）
 * --------------------------------------------------------------------------- */
TAL_NETIF_OPS_T tal_netif_ops_platform = {
    .get_ip      = tal_netif_esp_get_ip,
    .set_link_cb = tal_netif_esp_set_link_cb,
};
