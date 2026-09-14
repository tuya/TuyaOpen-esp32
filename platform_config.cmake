list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter)

include(${TOP_SOURCE_DIR}/boards/ESP32/common/CMakeLists.txt)

set(ENV_IDF_PATH $ENV{IDF_PATH})
set(IDF_PATH "${PLATFORM_PATH}/esp-idf")
if(ENV_IDF_PATH)
    set(IDF_PATH ${ENV_IDF_PATH})
endif()
message(STATUS "IDF_TOOLS_PATH: ${IDF_TOOLS_PATH}")
# mbedtls 4.x (IDF 6.x) split headers across three dirs; mbedtls4_compat
# forwards the private-ized 3.x-style ones (mbedtls/rsa.h -> mbedtls/private/rsa.h)
set(PLATFORM_PUBINC_2
    ${IDF_PATH}/components/mbedtls/mbedtls/include
    ${IDF_PATH}/components/mbedtls/port/include
    ${IDF_PATH}/components/mbedtls/port/psa_driver/include
    ${IDF_PATH}/components/mbedtls/port/psa_crypto_storage/include
    ${IDF_PATH}/components/mbedtls/mbedtls/tf-psa-crypto/include
    ${IDF_PATH}/components/mbedtls/mbedtls/tf-psa-crypto/drivers/builtin/include
    ${PLATFORM_PATH}/include/mbedtls4_compat
    ${IDF_PATH}/components/esp_common/include
    ${IDF_PATH}/components/esp_security/include
    ${IDF_PATH}/components/esp_hal_security/include
    ${IDF_PATH}/components/esp_rom/include
    ${IDF_PATH}/components/esp_rom/${TOS_PROJECT_CHIP}/include/${TOS_PROJECT_CHIP}
    ${IDF_PATH}/components/soc/${TOS_PROJECT_CHIP}/include
    ${PLATFORM_PATH}/tuya_open_sdk/build/config
    ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter/include/utilities/include/
    ${BOARD_INC}
)

set(PLATFORM_PUBINC
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})

# mbedtls 4.x include dirs above are added for every chip, so the compat
# switches must be too — esp32/s3 builds hit implicit-declaration of the
# legacy mbedtls_cipher_* API without the first one.
# Empty body matches private_access.h's own `#define`; a 1-valued -D body
# trips -Werror=redefined where that header is included unconditionally.
add_compile_definitions(MBEDTLS_DECLARE_PRIVATE_IDENTIFIERS=)
# esp32s3/esp32s31 (HARDWARE_SHA=n) take esp_config.h's ROM-MD5 path, whose guarded
# EMPTY-body HMAC define clashes with tf-psa-crypto's 1-valued one — pre-define
# the final value to skip it. Chips with the SHA driver path must NOT pre-define.
if("${TOS_PROJECT_CHIP}" STREQUAL "esp32s3" OR "${TOS_PROJECT_CHIP}" STREQUAL "esp32s31")
    add_compile_definitions(MBEDTLS_PSA_BUILTIN_ALG_HMAC=1)
endif()
