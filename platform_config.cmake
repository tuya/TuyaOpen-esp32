list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter)

# Type headers: the platform copy of tuya_cloud_types.h lacks CHAR_T / VOID_T /
# INT_T / OPERATE_RET; put the tuyaopen/include/common copy first.
list(INSERT PLATFORM_PUBINC_1 0 "${CMAKE_CURRENT_LIST_DIR}/tuya_open_sdk/tuyaos_adapter/tuyaopen/include/common")

# Ops contract headers shared by core and the port.
list(APPEND PLATFORM_PUBINC_1 "${TOP_SOURCE_DIR}/tuyaopen-core/port/tuyaopen/interface")


include(${TOP_SOURCE_DIR}/boards/ESP32/common/CMakeLists.txt)

set(ENV_IDF_PATH $ENV{IDF_PATH})
set(IDF_PATH "${PLATFORM_PATH}/esp-idf")
if(ENV_IDF_PATH)
    set(IDF_PATH ${ENV_IDF_PATH})
endif()
message(STATUS "IDF_TOOLS_PATH: ${IDF_TOOLS_PATH}")
set(PLATFORM_PUBINC_2
    ${IDF_PATH}/components/mbedtls/mbedtls/include
    ${IDF_PATH}/components/mbedtls/port/include
    ${IDF_PATH}/components/esp_rom/include
    ${IDF_PATH}/components/soc/${TOS_PROJECT_CHIP}/include
    ${IDF_PATH}/components/soc/${TOS_PROJECT_CHIP}/register
    ${IDF_PATH}/components/hal/include
    ${IDF_PATH}/components/hal/platform_port/include
    ${IDF_PATH}/components/esp_rom/${TOS_PROJECT_CHIP}/include
    ${IDF_PATH}/components/lwip/lwip/src/include
    ${IDF_PATH}/components/lwip/port/include
    ${IDF_PATH}/components/lwip/port/esp32xx/include
    ${IDF_PATH}/components/lwip/port/freertos/include
    ${IDF_PATH}/components/newlib/platform_include
    ${IDF_PATH}/components/esp_system/include
    ${IDF_PATH}/components/esp_hw_support/include
    ${IDF_PATH}/components/xtensa/include
    ${IDF_PATH}/components/xtensa/${TOS_PROJECT_CHIP}/include
    ${IDF_PATH}/components/freertos/FreeRTOS-Kernel/include
    ${IDF_PATH}/components/freertos/FreeRTOS-Kernel/portable/xtensa/include
    ${IDF_PATH}/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/freertos
    ${IDF_PATH}/components/freertos/config/include
    ${IDF_PATH}/components/freertos/config/include/freertos
    ${IDF_PATH}/components/freertos/config/xtensa/include
    ${IDF_PATH}/components/freertos/config/xtensa/include/freertos
    ${IDF_PATH}/components/freertos/esp_additions/include
    ${IDF_PATH}/components/freertos/esp_additions/include/freertos
    ${IDF_PATH}/components/heap/include
    ${IDF_PATH}/components/esp_common/include
    ${PLATFORM_PATH}/tuya_open_sdk/build/config
    ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter/include/utilities/include/
    ${BOARD_INC}
)

add_definitions(-DTUYAOPEN_PLATFORM_ESP32=1)

set(PLATFORM_PUBINC
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})
