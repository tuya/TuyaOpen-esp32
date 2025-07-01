
#ifndef __NET_H__
#define __NET_H__


#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "ethernet_init.h"
#include "sdkconfig.h"
// #include "esp_wifi.h"

#include <string.h>




// typedef struct
// {
//     char ip[16];    /* ip addr:  xxx.xxx.xxx.xxx  */
//     char mask[16];  /* net mask: xxx.xxx.xxx.xxx  */
//     char gw[16];    /* gateway:  xxx.xxx.xxx.xxx  */
//     char dns[16];    /* dns server:  xxx.xxx.xxx.xxx  */
//     BOOL_T dhcpen;  /* enable dhcp or not */
// } NW_IP_S;

typedef struct
{
    //ip回调
    void (*ip_cb)(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    void (*eth_cb)(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
    //是否获得ip
    uint8_t gain_ip;
    esp_eth_handle_t *eth_handles; // eth句柄
    uint8_t eth_port_cnt; // eth端口数量 只有1个 

    uint8_t mac_addr[6];
    char ip[16];    /* ip addr:  xxx.xxx.xxx.xxx  */
    char mask[16];  /* net mask: xxx.xxx.xxx.xxx  */
    char gw[16];    /* gateway:  xxx.xxx.xxx.xxx  */
    char dns[16];    /* dns server:  xxx.xxx.xxx.xxx  */
    bool dhcpen;  /* enable dhcp or not */
}Tnet;

extern Tnet ethw5500Fig;
esp_err_t ethw5500_main(void);
void ethw5500_stop(void);
void ethw5500_start(void);
#endif

