if(DEFINED ENV{BUILD_PARAM_DIR} AND NOT "$ENV{BUILD_PARAM_DIR}" STREQUAL "")
    include($ENV{BUILD_PARAM_DIR}/build_param.cmake)
endif()

set(TUYAOS_ADAPTER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
# message(STATUS "TUYAOS_ADAPTER_DIR: ${TUYAOS_ADAPTER_DIR}")

file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/system/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.c"
)

if (CONFIG_ENABLE_AUDIO)
    file(GLOB_RECURSE SOURCES_AUDIO "${CMAKE_CURRENT_SOURCE_DIR}/src/audio/*.c")
    list(APPEND SOURCES ${SOURCES_AUDIO}
        "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_i2s.c"
    )
endif()

list(APPEND SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_adc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_flash.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_i2c.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_network.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_ota.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_pin.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_pinmux.c"
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
    "include/i2c"
    "include/i2s"
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
    "include/vad"
    "include/kws"
    "include/media"
    )

if(NOT "$ENV{TUYAOS_HEADER_DIR}" STREQUAL "")

    string(REPLACE " " ";" TUYAOS_HEADER_DIR_LIST $ENV{TUYAOS_HEADER_DIR})

    foreach(head ${TUYAOS_HEADER_DIR_LIST})
        list(APPEND include_dirs ${head})
    endforeach()

endif()

if(NOT "$ENV{TUYAOS_BOARD_PATH}" STREQUAL "")
    if(EXISTS "$ENV{TUYAOS_BOARD_PATH}/common/CMakeLists.txt")
        include("$ENV{TUYAOS_BOARD_PATH}/common/CMakeLists.txt")
        list(APPEND SOURCES ${BOARD_SRC})
        list(APPEND include_dirs ${BOARD_INC})
    endif()
endif()

# message(STATUS "TUYAOS_ADAPTER_INCLUDE_DIRS: ${include_dirs}")

set(adapter_requires
    esp_lcd
    esp_lcd_sh8601
    esp_io_expander_tca9554
    esp_io_expander
    esp-sr
    esp_codec_dev
    mbedtls
    nvs_flash
    bt
    esp_pm
    esp_adc
    efuse
    soc
    driver
    app_update
    esp_lcd_touch_ft5x06
)

# espressif__esp_hosted is only available on chips without native Wi-Fi
# (currently ESP32-P4 paired with a C6 co-processor). On S3/C3/ESP32 this
# component is not pulled in by idf_component.yml so we must not require it.
if(CONFIG_IDF_TARGET_ESP32P4)
    list(APPEND adapter_requires espressif__esp_hosted)
endif()

idf_component_register(SRCS "${SOURCES}"
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${adapter_requires}
                    )

if(CONFIG_IDF_TARGET_ESP32P4)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=hci_rx_handler")
endif()
