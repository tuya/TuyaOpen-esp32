/* Ethernet Basic Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include "esp_system.h"
// #include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "ethw5500.h"
#include "esp_mac.h"
#include <esp_netif_types.h>
#include <esp_err.h>
// #include <esp32_mock.h>

static const char *TAG = "ethw5500";
#define CONFIG_EXAMPLE_ETH_DEINIT_AFTER_S -1 // 在经过指定秒数后停止并反初始化以太网

// 定义是否启用自动重连
// static uint8_t WIFI_AUTO_RECONNECT_ENABLED = 0;
// static EventGroupHandle_t s_wifi_event_group;
// #define WIFI_CONNECTED_BIT BIT0
// #define WIFI_FAIL_BIT      BIT1
// static int s_retry_num = 0;
// #define EXAMPLE_ESP_MAXIMUM_RETRY 5 // 最大尝试连接到ap次数

Tnet ethw5500Fig = {0}; // 网络配置结构体，用于存储网络配置信息;

/** 网线事件 */
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    // ethw5500Fig.mac_addr置0
    memset(ethw5500Fig.mac_addr, 0, sizeof(ethw5500Fig.mac_addr));
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, ethw5500Fig.mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x", ethw5500Fig.mac_addr[0], ethw5500Fig.mac_addr[1],
                 ethw5500Fig.mac_addr[2], ethw5500Fig.mac_addr[3], ethw5500Fig.mac_addr[4], ethw5500Fig.mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        ethw5500Fig.gain_ip = 0;
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        ethw5500Fig.gain_ip = 0;
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        ethw5500Fig.gain_ip = 0;
        break;
    default:
        break;
    }
    if (ethw5500Fig.eth_cb)
    {
        ethw5500Fig.eth_cb(arg,  event_base,  event_id,  event_data);
    }
    
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    // ESP_LOGI(TAG, "~~~~~~~~~~~");
    // ESP_LOGI(TAG, "ip:" IPSTR, IP2STR(&ip_info->ip));
    // ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    // ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    // ESP_LOGI(TAG, "~~~~~~~~~~~");

    ESP_LOGI(TAG, "~~~~~~~~~~~");
    // IPv4 地址最大长度为 15字符 + 终止符
    esp_ip4addr_ntoa(&ip_info->ip, ethw5500Fig.ip , sizeof(ethw5500Fig.ip )); // 转换为字符串
    ESP_LOGI(TAG, "ethw5500Fig.ip:%s", ethw5500Fig.ip);


    esp_ip4addr_ntoa(&ip_info->netmask, ethw5500Fig.mask , sizeof(ethw5500Fig.mask )); // 转换为字符串
    ESP_LOGI(TAG, "ethw5500Fig.netmask:%s", ethw5500Fig.mask);

    esp_ip4addr_ntoa(&ip_info->gw, ethw5500Fig.gw , sizeof(ethw5500Fig.gw )); // 转换为字符串
    ESP_LOGI(TAG, "ethw5500Fig.gw:%s", ethw5500Fig.gw);
    ESP_LOGI(TAG, "~~~~~~~~~~~");

    ethw5500Fig.gain_ip = 1; // 获得ip
    if (ethw5500Fig.ip_cb) {
        ethw5500Fig.ip_cb(arg,  event_base,  event_id,  event_data);
    }
}

void nvs_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

esp_err_t ethw5500_main(void)
{

    nvs_main();
    esp_err_t err = ESP_OK;
    err = esp_netif_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_netif_init 失败");
        return err;
    }

    err = esp_event_loop_create_default();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_event_loop_create_default 失败");
        return err;
    }

    // if (ethw5500Fig.ip == NULL) {
    //     // 申请空间
    //     ethw5500Fig.ip = malloc(50);
    //     // 清零
    //     memset(ethw5500Fig.ip, 0, 50);
    // }

    // Initialize Ethernet driver
    // uint8_t eth_port_cnt = 0; // 以太网端口数量
    // esp_eth_handle_t *eth_handles;
    err = example_eth_init(&ethw5500Fig.eth_handles, &ethw5500Fig.eth_port_cnt);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "example_eth_init 失败");
        return err;
    }

    esp_netif_t *eth_netifs[ethw5500Fig.eth_port_cnt];
    esp_eth_netif_glue_handle_t eth_netif_glues[ethw5500Fig.eth_port_cnt];

    // 为以太网创建esp-netif实例
    if (ethw5500Fig.eth_port_cnt == 1) //
    {
        ESP_LOGI(TAG,"只有一个以太网接口");
        // Use ESP_NETIF_DEFAULT_ETH when just one Ethernet interface is used and you don't need to modify
        // default esp-netif configuration parameters.
        esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
        eth_netifs[0] = esp_netif_new(&cfg);
        eth_netif_glues[0] = esp_eth_new_netif_glue(ethw5500Fig.eth_handles[0]);
        // Attach Ethernet driver to TCP/IP stack
        err = esp_netif_attach(eth_netifs[0], eth_netif_glues[0]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_netif_attach 失败");
            return err;
        }

    } else // 多个以太网接口
    {
        ESP_LOGI(TAG,"多个以太网接口");
        return ESP_FAIL;
    }

    // 注册用户定义的事件处理程序
    err = esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_event_handler_register 失败");
        return err;
    }
    err = esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_event_handler_register 注册w5500回调 失败");
        return err;
    }
    // 启动以太网驱动程序状态机
    for (int i = 0; i < ethw5500Fig.eth_port_cnt; i++) {
        err = esp_eth_start(ethw5500Fig.eth_handles[i]);
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "esp_eth_start 启动w5500 失败");
            return err;
        }
    }
    return err;
}

// 停止以太网
void ethw5500_stop(void)
{
    for (int i = 0; i < ethw5500Fig.eth_port_cnt; i++) {
        esp_eth_stop(ethw5500Fig.eth_handles[ethw5500Fig.eth_port_cnt]);
    }


    // #if CONFIG_EXAMPLE_ETH_DEINIT_AFTER_S >= 0
    //     // For demonstration purposes, wait and then deinit Ethernet network
    //     vTaskDelay(pdMS_TO_TICKS(CONFIG_EXAMPLE_ETH_DEINIT_AFTER_S * 1000));
    //     ESP_LOGI(TAG, "stop and deinitialize Ethernet network...");
    //     // Stop Ethernet driver state machine and destroy netif
    //     for (int i = 0; i < ethw5500Fig.eth_port_cnt; i++) {
    //         ESP_ERROR_CHECK(esp_eth_stop(ethw5500Fig.eth_handles[i]));
    //         ESP_ERROR_CHECK(esp_eth_del_netif_glue(eth_netif_glues[i]));
    //         esp_netif_destroy(eth_netifs[i]);
    //     }
    //     esp_netif_deinit();
    //     ESP_ERROR_CHECK(example_eth_deinit(ethw5500Fig.eth_handles, ethw5500Fig.eth_port_cnt));
    //     ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_ETH_GOT_IP, got_ip_event_handler));
    //     ESP_ERROR_CHECK(esp_event_handler_unregister(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler));
    //     ESP_ERROR_CHECK(esp_event_loop_delete_default());
    // #endif // EXAMPLE_ETH_DEINIT_AFTER_S > 0



}
// 启动以太网
void ethw5500_start(void)
{
    for (int i = 0; i < ethw5500Fig.eth_port_cnt; i++) {
        esp_eth_start(ethw5500Fig.eth_handles[i]);
    }
}
