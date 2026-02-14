idf_component_register(SRCS "ethw5500.c"
                    INCLUDE_DIRS "."
                    REQUIRES ethernet_init esp_netif esp_eth nvs_flash esp_wifi )
