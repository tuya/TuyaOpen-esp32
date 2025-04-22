list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter)

set(ENV_IDF_PATH $ENV{IDF_PATH})
set(IDF_PATH "${PLATFORM_PATH}/esp-idf")
if(ENV_IDF_PATH)
    set(IDF_PATH ${ENV_IDF_PATH})
endif()
message(STATUS "IDF_TOOLS_PATH: ${IDF_TOOLS_PATH}")
set(PLATFORM_PUBINC_2 
    ${IDF_PATH}/components/mbedtls/mbedtls/include
    ${IDF_PATH}/components/mbedtls/port/include
    ${IDF_PATH}/components/soc/${TOS_PROJECT_CHIP}/include
    ${PLATFORM_PATH}/tuya_open_sdk/build/config
)

set(PLATFORM_PUBINC 
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})
