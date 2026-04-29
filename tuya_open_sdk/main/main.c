#include <stdio.h>
#include "driver/gpio.h"
#include "driver/uart.h"

#include "nvs_flash.h"

#if CONFIG_IDF_TARGET_ESP32S3
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern void tkl_flash_agent_init(void);
extern void tkl_flash_agent_process(void);

static void flash_agent_task(void *arg)
{
    while (1) {
        tkl_flash_agent_process();
    }
}
#endif

extern void tuya_app_main(void);
void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

#if CONFIG_IDF_TARGET_ESP32S3
    tkl_flash_agent_init();
    xTaskCreate(flash_agent_task, "flash_agent", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
#endif

    tuya_app_main();
}
