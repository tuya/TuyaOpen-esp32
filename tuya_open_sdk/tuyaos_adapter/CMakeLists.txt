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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_dma2d.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_flash.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_i2c.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_network.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_ota.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_pin.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_pinmux.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_pwm.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_rtc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/drivers/tkl_spi.c"
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

# Expose ALL header dirs under include/ (mirrors the T5AI / T3 adapters) so any
# REQUIRES-tuyaos_adapter consumer resolves every public tkl/tal header — no
# hand-maintained subset to drift out of date (the old curated list omitted
# include/security and include/lwip, so tal_api.h failed on tkl_hash.h).
macro(FIND_INCLUDE_DIR result curdir)
    file(GLOB_RECURSE children "${curdir}/*.h")
    set(dirlist "")
    foreach(child ${children})
        string(REGEX REPLACE "(.*)/.*" "\\1" DIR_NAME ${child})
        if((IS_DIRECTORY ${DIR_NAME}) AND (NOT (${DIR_NAME} IN_LIST dirlist)))
            list(APPEND dirlist ${DIR_NAME})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()

FIND_INCLUDE_DIR(include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/include")

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
    esp_lcd_touch_gt911
    fatfs
    sdmmc
)

# espressif__esp_hosted is only available on chips without native Wi-Fi
# (currently ESP32-P4 paired with a C6 co-processor). On S3/C3/ESP32 this
# component is not pulled in by idf_component.yml so we must not require it.
# Use IDF_TARGET (always defined during component processing) rather than
# CONFIG_IDF_TARGET_ESP32P4, which is not reliably set as a CMake variable at
# this point — the old guard never fired, so these REQUIRES were silently
# dropped (WiFi still worked because esp_wifi_remote pulls esp_hosted in
# transitively, but the esp_video include path was missing).
if(IDF_TARGET STREQUAL "esp32p4")
    list(APPEND adapter_requires espressif__esp_hosted)
    # esp_video provides the V4L2 MIPI-CSI camera interface (P4 only).
    list(APPEND adapter_requires espressif__esp_video espressif__esp_cam_sensor)
    # esp_lcd_st7701 is a MIPI-DSI panel driver (P4 only). Matches the
    # target == esp32p4 gate in main/idf_component.yml — on other targets the
    # component has no matching version and must not be required.
    list(APPEND adapter_requires esp_lcd_st7701)
endif()

# PPA (Pixel-Processing Accelerator) backs tkl_dma2d on the ESP32-P4. Use
# IDF_TARGET (not CONFIG_IDF_TARGET_ESP32P4) so the requirement is visible during
# the early component-requirement expansion, when CONFIG_ vars are not yet set.
if("${IDF_TARGET}" STREQUAL "esp32p4")
    list(APPEND adapter_requires esp_driver_ppa)
endif()

idf_component_register(SRCS "${SOURCES}"
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${adapter_requires}
                    )

if(CONFIG_IDF_TARGET_ESP32P4)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=hci_rx_handler")
endif()
