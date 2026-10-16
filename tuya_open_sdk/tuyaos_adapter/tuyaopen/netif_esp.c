/**
 * @file netif_esp.c
 * @brief 本机网络接口信息实现（esp_netif/esp_wifi 事件）：取 IP / 链路状态回调
 *
 * 与 linux 版（POSIX ioctl 3s 轮询）对应：esp32 的网络接口由 WiFi 连接
 * 流程创建，本文件监听 esp_netif/esp_wifi 事件驱动链路状态回调，不做轮询。
 *
 * 链路 up 语义 = 已获 IP（IP_EVENT_STA_GOT_IP）：WIFI_EVENT_STA_CONNECTED
 * 只表示关联成功、IP 未就绪，此时发起云连接会失败重试。对齐 linux 版
 * "注册回调时先取当前状态立即回调一次"的约定（事件注册前已连上的场景
 * 不丢状态）。
 *
 * 依赖顺序：默认事件循环（esp_event_loop_create_default）与 station netif
 * 由 WiFi 连接流程（demo 的 wifi_sta 模块）建立，本文件在
 * tal_netif_set_link_cb 时注册事件处理器——调用方须先完成 WiFi 初始化。
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 *
 */

#include "tal_netif_ops.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include <string.h>

#include "port_log.h"

#include "netif_esp.h"

/* station netif 的 ifkey（esp_netif_create_default_wifi_sta 创建，
 * 见 esp_netif_defaults.h 的 ESP_NETIF_DEFAULT_WIFI_STA） */
#define NETIF_STA_IFKEY "WIFI_STA_DEF"

static TAL_NETIF_LINK_CHANGE_CB s_netif_link_cb = NULL;
static BOOL_T s_netif_handler_registered = FALSE;

/* 取 station netif 句柄（未创建返回 NULL） */
static esp_netif_t *__netif_get_sta(void)
{
    return esp_netif_get_handle_from_ifkey(NETIF_STA_IFKEY);
}

/* 当前是否已获 IP（链路 up 的判据） */
static BOOL_T __netif_has_ip(void)
{
    esp_netif_t *netif = __netif_get_sta();
    esp_netif_ip_info_t ip_info;

    if (NULL == netif) {
        return FALSE;
    }
    if (ESP_OK != esp_netif_get_ip_info(netif, &ip_info)) {
        return FALSE;
    }

    return (ip_info.ip.addr != 0) ? TRUE : FALSE;
}

/* 事件处理器：IP 就绪 → up；station 断线 → down */
static void __netif_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    TAL_NETIF_LINK_STAT_E status;

    (void)arg;
    (void)event_data;

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        status = TAL_NETIF_LINK_UP;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        status = TAL_NETIF_LINK_DOWN;
    } else {
        return;
    }

    if (s_netif_link_cb) {
        s_netif_link_cb(status);
    }
}

OPERATE_RET tal_netif_esp_get_ip(NW_IP_S *ip)
{
    esp_netif_t *netif;
    esp_netif_ip_info_t ip_info;

    if (NULL == ip) {
        return OPRT_INVALID_PARM;
    }

    netif = __netif_get_sta();
    if (NULL == netif) {
        return OPRT_NOT_FOUND;
    }
    if (ESP_OK != esp_netif_get_ip_info(netif, &ip_info) || 0 == ip_info.ip.addr) {
        return OPRT_NOT_FOUND;
    }

    esp_ip4addr_ntoa(&ip_info.ip, ip->ip, sizeof(ip->ip));
    esp_ip4addr_ntoa(&ip_info.netmask, ip->mask, sizeof(ip->mask));
    esp_ip4addr_ntoa(&ip_info.gw, ip->gw, sizeof(ip->gw));

    return OPRT_OK;
}

OPERATE_RET tal_netif_esp_set_link_cb(TAL_NETIF_LINK_CHANGE_CB cb)
{
    if (cb) {
        if (!s_netif_handler_registered) {
            /* 默认事件循环须已建立（WiFi 初始化流程负责） */
            esp_err_t err = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, __netif_event_handler, NULL);
            if (ESP_OK == err) {
                err = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, __netif_event_handler, NULL);
            }
            if (ESP_OK != err) {
                PORT_LOG_ERR("netif event handler register failed: %d (event loop created?)", err);
                return OPRT_COM_ERROR;
            }
            s_netif_handler_registered = TRUE;
        }

        s_netif_link_cb = cb;

        /* 对齐 linux 版：注册时先取当前状态立即回调一次 */
        cb(__netif_has_ip() ? TAL_NETIF_LINK_UP : TAL_NETIF_LINK_DOWN);
    }

    return OPRT_OK;
}
