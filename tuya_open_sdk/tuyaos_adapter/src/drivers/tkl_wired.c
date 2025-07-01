/**
 * @file tkl_wired.c
 * @brief the default weak implements of wired driver, this implement only used when OS=linux
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */
// 有线
#include "tuya_cloud_types.h"
#include "tkl_wired.h"
#include "esp_log.h"
#include <pthread.h>
#include "ethw5500.h"


#define DBG_TAG "tkl_wired"

TKL_WIRED_STATUS_CHANGE_CB event_cb;
pthread_t wired_event_thread = 0;

/**
 * @brief 根据网口名称获取有线网络状态
 *
 * @param[in] if_name 网口名称
 * @param[out] status 返回的有线网络状态
 *
 * @return OPERATE_RET 返回操作结果，OPRT_OK表示成功
 */
static OPERATE_RET __tkl_wired_get_status_by_name(const char *if_name, TKL_WIRED_STAT_E *status)
{

    // if (status == NULL) {
    //     return OPRT_INVALID_PARM;
    // }
    ESP_LOGD(DBG_TAG,"if_name:%s", if_name);
    ESP_LOGI(DBG_TAG, "tkl_wired 获取网络接口状态if_name:%s",if_name);
    *status = TKL_WIRED_LINK_UP; // 返回有连接
    // OPRT_COM_ERROR
    return OPRT_OK;
}

static void *wifi_status_event_cb(void *arg)
{
    TKL_WIRED_STAT_E last_stat = -1;
    TKL_WIRED_STAT_E stat = TKL_WIRED_LINK_DOWN;
    ESP_LOGI(DBG_TAG, "tkl_wired wifi_status_event_cb");

    pthread_detach(pthread_self());
    while (1) {
        tkl_wired_get_status(&stat);
        if (stat != last_stat) {
            event_cb(stat);
            last_stat = stat;
        }

        sleep(3);
    }

    return NULL;
}

/**
 * @brief 获取有线连接的状态
 *
 * @param[out]  is_up: 有线连接状态是否正常
 *
 * @return 成功时返回OPRT_OK。其他错误请参考tuya_error_code.h
 */
TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wired_get_status(TKL_WIRED_STAT_E *status)
{
    ESP_LOGI(DBG_TAG, "tkl_wired tkl_wired_get_status");
    return OPRT_OK;
}


static void ip_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

}
TKL_WIRED_STATUS_CHANGE_CB TKL_eth_cb = NULL;
// TKL_WIRED_LINK_DOWN = 0,    ///< 网线已拔下
// TKL_WIRED_LINK_UP,          ///< 网线已插入，IP已获取
static void eth_cb(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    if (!TKL_eth_cb)
    {
        return ;//不存在
    }

    if (event_id == ETHERNET_EVENT_CONNECTED)
    {
        ESP_LOGI(DBG_TAG, "Ethernet Link Up");
        TKL_eth_cb(TKL_WIRED_LINK_UP);
    }else{
        TKL_eth_cb(TKL_WIRED_LINK_DOWN);
        ESP_LOGI(DBG_TAG, "Ethernet Link 其他状态");
    }

}

/**
 * @brief  设置状态变更回调
 *
 * @param[in]   cb: 链接状态改变时的回调函数
 *
 * @return OPRT_OK 成功时。其他人出错，请参考 tuya_error_code.h
 */
TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wired_set_status_cb(TKL_WIRED_STATUS_CHANGE_CB cb)
{
    if (cb)
    {
        TKL_eth_cb = cb;
    }
    ethw5500Fig.ip_cb = ip_cb;
    ethw5500Fig.eth_cb = eth_cb;
    ethw5500_main();
    return OPRT_OK;
}
//获取有线网络接口的 IP 地址信息，并通过参数返回给调用者
// static OPERATE_RET __tkl_wired_get_ip_by_name(const char *if_name, NW_IP_S *ip)
// {
//     strcpy(if_name, "eth0");
//     strcpy(ip, ethw5500Fig.ip);
//     ESP_LOGI(DBG_TAG, "tkl_wired __tkl_wired_get_ip_by_name");
//     return OPRT_OK;
// }
static OPERATE_RET __tkl_wired_get_ip_by_name(NW_IP_S *ip) {
    if (ip == NULL) { // 仅检查ip指针
        ESP_LOGE(DBG_TAG, "Invalid parameters: ip is NULL 指针问题");
        return OPRT_INVALID_PARM;
    }

    // 假设 ethw5500Fig.ip 已正确初始化（如通过DHCP获取）
    strncpy(ip->ip, ethw5500Fig.ip, sizeof(ip->ip) - 1);
    ip->ip[sizeof(ip->ip) - 1] = '\0'; // 确保终止

    // 可选：填充其他字段（如子网掩码、网关）
    strncpy(ip->mask, ethw5500Fig.mask, sizeof(ip->mask) - 1);
    ip->mask[sizeof(ip->mask) - 1] = '\0';

    strncpy(ip->gw, ethw5500Fig.gw, sizeof(ip->gw) - 1);
    ip->gw[sizeof(ip->gw) - 1] = '\0';

    ESP_LOGI(DBG_TAG, "tkl_wired __tkl_wired_get_ip_by_name: IP=%s", ip->ip);
    return OPRT_OK;
}
/**
 * @brief  设置有线连接的IP地址
 *
 * @param[in]   ip: IP地址
 *
 * @return OPRT_OK on success. Others on error, please refer to
 * tuya_error_code.h
 */
OPERATE_RET tkl_wired_set_ip(NW_IP_S *ip)
{
    ESP_LOGI(DBG_TAG, "tkl_wired 设置有线连接的IP地址 tkl_wired_set_ip");

    return OPRT_NOT_SUPPORTED;//不支持
}

/**
 * @brief  获取有线连接的IP地址
 *
 * @param[in]   ip: the ip address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wired_get_ip(NW_IP_S *ip)
{
    // 检查输入参数有效性
    // if (ethw5500Fig.ip == NULL || ip == NULL) {
    //     return OPRT_INVALID_PARM;
    // }

    // ESP_LOGI(DBG_TAG, "tkl_wired 获取有线连接的IP地址 ethw5500Fig.ip %s", ethw5500Fig.ip);

    // 安全复制IP地址到结构体成员
    strncpy(ip->ip, ethw5500Fig.ip, sizeof(ip->ip) - 1);
    ip->ip[sizeof(ip->ip) - 1] = '\0'; // 确保字符串终止

    // ESP_LOGI(DBG_TAG, "tkl_wired 获取有线连接的IP地址 ip %s", ip->ip);
    return OPRT_OK;
}

/**
 * @brief  获取有线连接的MAC地址
 *
 * @param[in]   mac: the mac address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wired_get_mac(NW_MAC_S *mac)
{
    ESP_LOGI(DBG_TAG, "tkl_wired 获取有线连接的MAC地址 tkl_wired_get_mac");
    mac->mac[0] = ethw5500Fig.mac_addr[0];
    mac->mac[1] = ethw5500Fig.mac_addr[1];
    mac->mac[2] = ethw5500Fig.mac_addr[2];
    mac->mac[3] = ethw5500Fig.mac_addr[3];
    mac->mac[4] = ethw5500Fig.mac_addr[4];
    mac->mac[5] = ethw5500Fig.mac_addr[5];
    // strcpy(mac, ethw5500Fig.mac_addr);
    return OPRT_OK;
}

/**
 * @brief  设置有线连接的MAC地址
 *
 * @param[in]   mac: the mac address
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
TUYA_WEAK_ATTRIBUTE OPERATE_RET tkl_wired_set_mac(const NW_MAC_S *mac)
{
    ESP_LOGI(DBG_TAG, "tkl_wired tkl_wired_set_mac");
    return OPRT_NOT_SUPPORTED;//不支持
}
