if(DEFINED ENV{BUILD_PARAM_DIR} AND NOT "$ENV{BUILD_PARAM_DIR}" STREQUAL "")
    include($ENV{BUILD_PARAM_DIR}/build_param.cmake)
endif()

set(TUYAOS_ADAPTER_DIR ${CMAKE_CURRENT_SOURCE_DIR})
# message(STATUS "TUYAOS_ADAPTER_DIR: ${TUYAOS_ADAPTER_DIR}")

file(GLOB_RECURSE SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/system/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.c"
)

# stdio_compat.c aliases the reent stderr via _impure_ptr — riscv-newlib only.
# Picolibc already provides a global stderr, so compiling the shim there would
# fail because _impure_ptr is not part of the Picolibc ABI.
if(NOT (CONFIG_IDF_TARGET_ARCH_RISCV AND CONFIG_LIBC_NEWLIB))
    list(REMOVE_ITEM SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/system/stdio_compat.c")
endif()

if (CONFIG_ENABLE_AUDIO_CODECS)
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
    "${CMAKE_CURRENT_SOURCE_DIR}/tuyaopen/tuyaopen_license.c"
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
list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/tuyaopen")

if(NOT "$ENV{TUYAOS_HEADER_DIR}" STREQUAL "")

    string(REPLACE " " ";" TUYAOS_HEADER_DIR_LIST $ENV{TUYAOS_HEADER_DIR})

    foreach(head ${TUYAOS_HEADER_DIR_LIST})
        list(APPEND include_dirs ${head})
    endforeach()

endif()

# Board common TDD layer (lcd, button, led, audio, camera drivers).
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
    mbedtls
    nvs_flash
    bt
    esp_pm
    esp_adc
    efuse
    soc
    driver
    # IDF 6.x split RMT/LEDC out of "driver"; tkl adapters link their symbols directly
    esp_driver_rmt
    esp_driver_ledc
    app_update
    fatfs
    sdmmc
)

# The component manager strips the registry namespace from dependency names.
# In the offline fallback, cached component directories retain names such as
# espressif__esp_codec_dev, so use those names when registering the cache as
# ordinary project components.
if("$ENV{IDF_COMPONENT_MANAGER}" STREQUAL "0")
    list(APPEND adapter_requires
        waveshare__esp_lcd_sh8601
        espressif__esp_io_expander_tca9554
        espressif__esp_io_expander
        espressif__esp_codec_dev
        espressif__esp_lcd_touch_ft5x06
        espressif__esp_lcd_touch_gt911
        espressif__esp_lcd_touch_gt1151
    )
    if(CONFIG_ENABLE_AUDIO_CODECS)
        list(APPEND adapter_requires espressif__esp-sr)
    endif()
else()
    list(APPEND adapter_requires
        esp_lcd_sh8601
        esp_io_expander_tca9554
        esp_io_expander
        esp_codec_dev
        esp_lcd_touch_ft5x06
        esp_lcd_touch_gt911
        esp_lcd_touch_gt1151
    )
    if(CONFIG_ENABLE_AUDIO_CODECS)
        list(APPEND adapter_requires esp-sr)
    endif()
endif()

if(CONFIG_ENABLE_AI_COMPONENTS)
    if("$ENV{IDF_COMPONENT_MANAGER}" STREQUAL "0")
        list(APPEND adapter_requires espressif__esp-sr)
    else()
        list(APPEND adapter_requires esp-sr)
    endif()
endif()

# esp32-camera DVP driver (ESP32 / ESP32-S3 only)
if(IDF_TARGET STREQUAL "esp32s3" OR IDF_TARGET STREQUAL "esp32")
    list(APPEND adapter_requires espressif__esp32-camera)
endif()

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
endif()

if(IDF_TARGET STREQUAL "esp32p4" OR IDF_TARGET STREQUAL "esp32s31")
    # esp_video provides the V4L2 camera interface (MIPI-CSI on P4, DVP on S31).
    list(APPEND adapter_requires espressif__esp_video espressif__esp_cam_sensor)
endif()

if(IDF_TARGET STREQUAL "esp32p4")
    # esp_lcd_st7701 is a MIPI-DSI panel driver (P4 only). Matches the
    # target == esp32p4 gate in main/idf_component.yml — on other targets the
    # component has no matching version and must not be required.
    list(APPEND adapter_requires esp_lcd_st7701)
endif()

# PPA (Pixel-Processing Accelerator) backs tkl_dma2d on the ESP32-P4 and
# ESP32-S31 (both set SOC_PPA_SUPPORTED=1). Use IDF_TARGET (not
# CONFIG_IDF_TARGET_*) so the requirement is visible during the early
# component-requirement expansion, when CONFIG_ vars are not yet set.
if("${IDF_TARGET}" STREQUAL "esp32p4" OR "${IDF_TARGET}" STREQUAL "esp32s31")
    list(APPEND adapter_requires esp_driver_ppa)
endif()

idf_component_register(SRCS "${SOURCES}"
                    INCLUDE_DIRS "${include_dirs}"
                    REQUIRES ${adapter_requires}
                    )

if(CONFIG_IDF_TARGET_ESP32P4)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--wrap=hci_rx_handler")
endif()

# esp-sr prebuilt archives reference a global `stderr`; archives are scanned
# before that reference appears, so force-pull the stdio_compat shim object
# only for configurations that actually link esp-sr.
if(CONFIG_ENABLE_AUDIO OR CONFIG_ENABLE_AUDIO_CODECS OR CONFIG_ENABLE_AI_COMPONENTS)
    target_link_libraries(${COMPONENT_LIB} INTERFACE "-Wl,--undefined=stderr")
endif()
