list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter)
set(PLATFORM_PUBINC_2 
    ${PLATFORM_PATH}/esp-idf/components/mbedtls/mbedtls/include
    ${PLATFORM_PATH}/esp-idf/components/mbedtls/port/include
    ${PLATFORM_PATH}/esp-idf/components/soc/esp32c3/include
    ${PLATFORM_PATH}/tuya_open_sdk/build/config
    ${PLATFORM_PATH}/tools/${TOS_PROJECT_CHIP}
)

set(PLATFORM_PUBINC 
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})
