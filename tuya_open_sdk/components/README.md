# 在需要的仓库中添加 components 作为子模块
```shell
git submodule add git@e.coding.net:qq547176052/ty/ty00_pwm.git components
```


## 组件库

### 直接调用 xxx_app 即可


CMakeLists.txt
```cmake
idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ""
                    REQUIRES ap2eth net WF100D bsp_mqtt web_ota bsp_udp bsp_tcp in_out_io bsp_pwm bsp_uart)
```

main.c
```c
/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"


#include <string.h>
// #include "ap2eth.h"
// #include "net.h"
// #include "wf100d.h"
// #include "bsp_mqtt.h"
// #include "web_ota.h"
// #include "bsp_udp.h"
// #include "bsp_tcp.h"
// #include "in_out_io.h"
// #include "bsp_pwm.h"
// #include "bsp_uart.h"

void app_main(void)
{
    printf("开始\n");

    // ap2eth_app();
    // net_app();
    // wf100d_app();//气压

    // bsp_mqtt_app();
    // web_ota_main();
    // bsp_udp_app();
    // bsp_tcp_app();
    // in_out_io_app();
    // bsp_pwm_app();
    // bsp_uart_app();




    for (;;) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

}
```