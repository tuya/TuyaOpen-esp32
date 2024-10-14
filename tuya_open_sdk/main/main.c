#include <stdio.h>
#include "driver/gpio.h"
#include "driver/uart.h"

#include "nvs_flash.h"

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

    tuya_app_main();
}
