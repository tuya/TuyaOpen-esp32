list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter)

include(${TOP_SOURCE_DIR}/boards/ESP32/common/CMakeLists.txt)

if("${TOS_PROJECT_CHIP}" STREQUAL "esp32s31")
    set(IDF_PATH "${PLATFORM_PATH}/esp-idf-master")
else()
    set(IDF_PATH "${PLATFORM_PATH}/esp-idf-v5.4")
endif()
set(ENV_IDF_PATH $ENV{IDF_PATH})
if(ENV_IDF_PATH)
    set(IDF_PATH ${ENV_IDF_PATH})
endif()
message(STATUS "IDF_TOOLS_PATH: ${IDF_TOOLS_PATH}")
set(PLATFORM_PUBINC_2
    ${IDF_PATH}/components/mbedtls/mbedtls/include
    ${IDF_PATH}/components/mbedtls/port/include
    ${IDF_PATH}/components/esp_rom/include
    ${IDF_PATH}/components/soc/${TOS_PROJECT_CHIP}/include
    ${PLATFORM_PATH}/tuya_open_sdk/build/config
    ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter/include/utilities/include/
    ${BOARD_INC}
)

# ESP-IDF 6.x / mbedTLS 4 compatibility is only required by the S31's pinned
# master tree.  Keep v5.4's public include set identical to platform master.
if("${TOS_PROJECT_CHIP}" STREQUAL "esp32s31")
    list(APPEND PLATFORM_PUBINC_2
        ${IDF_PATH}/components/mbedtls/port/psa_driver/include
        ${IDF_PATH}/components/mbedtls/port/psa_crypto_storage/include
        ${IDF_PATH}/components/mbedtls/mbedtls/tf-psa-crypto/include
        ${IDF_PATH}/components/mbedtls/mbedtls/tf-psa-crypto/drivers/builtin/include
        ${PLATFORM_PATH}/include/mbedtls4_compat
        ${IDF_PATH}/components/esp_common/include
        ${IDF_PATH}/components/esp_security/include
        ${IDF_PATH}/components/esp_hal_security/include
        ${IDF_PATH}/components/esp_rom/${TOS_PROJECT_CHIP}/include/${TOS_PROJECT_CHIP}
    )
endif()

set(PLATFORM_PUBINC
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})

if("${TOS_PROJECT_CHIP}" STREQUAL "esp32s31")
    # Empty body matches mbedTLS 4's private_access.h definition.
    add_compile_definitions(MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS=)
    add_compile_definitions(MBEDTLS_PSA_BUILTIN_ALG_HMAC=1)
endif()
