/**
 * @file netif_esp.c
 * @brief Local network interface info via esp_netif / esp_wifi events:
 *        local IP and link status callback.
 *
 * Link up means an IP address has been obtained (IP_EVENT_STA_GOT_IP);
 * WIFI_EVENT_STA_CONNECTED only means association. The default event loop and
 * the station netif must be created before core_netif_set_link_cb is called.
 *
 * @copyright Copyright (c) 2021-2026 Tuya Inc. All Rights Reserved.
 *
 */

#include "core_netif_ops.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"

#include <string.h>

#include "port_log.h"

#include "netif_esp.h"

/* station netif ifkey, created by esp_netif_create_default_wifi_sta */
#define NETIF_STA_IFKEY "WIFI_STA_DEF"

static CORE_NETIF_LINK_CHANGE_CB s_netif_link_cb = NULL;
static BOOL_T s_netif_handler_registered = FALSE;

/* station netif handle, NULL if not created */
static esp_netif_t *__netif_get_sta(void)
{
    return esp_netif_get_handle_from_ifkey(NETIF_STA_IFKEY);
}

/* true when the station has an IP address */
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

/* IP ready -> up; station disconnected -> down */
static void __netif_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    CORE_NETIF_LINK_STAT_E status;

    (void)arg;
    (void)event_data;

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        status = CORE_NETIF_LINK_UP;
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        status = CORE_NETIF_LINK_DOWN;
    } else {
        return;
    }

    if (s_netif_link_cb) {
        s_netif_link_cb(status);
    }
}

OPERATE_RET core_netif_esp_get_ip(NW_IP_S *ip)
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

OPERATE_RET core_netif_esp_set_link_cb(CORE_NETIF_LINK_CHANGE_CB cb)
{
    if (cb) {
        if (!s_netif_handler_registered) {
            /* the default event loop must exist */
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

        /* report the current state immediately after registration */
        cb(__netif_has_ip() ? CORE_NETIF_LINK_UP : CORE_NETIF_LINK_DOWN);
    }

    return OPRT_OK;
}
