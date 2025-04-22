set(TUYAOS_ADAPTER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
# message(STATUS "TUYAOS_ADAPTER_DIR: ${TUYAOS_ADAPTER_DIR}")

file(GLOB_RECURSE SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/system/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.c")

list(APPEND SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_adc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_flash.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_network.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_ota.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_pin.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_pwm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_rtc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_timer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_uart.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_watchdog.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_wifi.c"
    )

if(CONFIG_IDF_TARGET_ESP32S2)
    message(STATUS "ESP32S2 isn't support bluetooth")
else()
    list(APPEND SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_bt.c")
endif()

set(include_dirs
    "include/adc"
    "include/bluetooth"
    "include/flash"
    "include/gpio"
    "include/hci"
    "include/init/include"
    "include/network"
    "include/pinmux"
    "include/pwm"
    "include/rtc"
    "include/spi"
    "include/system"
    "include/timer"
    "include/uart"
    "include/watchdog"
    "include/wifi"
    "include/utilities/include"
    )

if(NOT "$ENV{TUYAOS_HEADER_DIR}" STREQUAL "")

    string(REPLACE " " ";" TUYAOS_HEADER_DIR_LIST $ENV{TUYAOS_HEADER_DIR})

    foreach(head ${TUYAOS_HEADER_DIR_LIST})
        list(APPEND include_dirs ${head})
    endforeach()

endif()

# message(STATUS "TUYAOS_ADAPTER_INCLUDE_DIRS: ${include_dirs}")

idf_component_register(SRCS "${SOURCES}"
                    INCLUDE_DIRS "${include_dirs}" REQUIRES mbedtls nvs_flash bt esp_pm esp_adc efuse soc driver app_update)
                    