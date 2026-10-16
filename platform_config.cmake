list_subdirectories(PLATFORM_PUBINC_1 ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter)

# [交付仓 ESP32 接入] 类型头优先：本平台 tuyaos_adapter/include/utilities/include 的
# tuya_cloud_types.h 是旧 TKL 版本，**缺 TAL 系别名**（CHAR_T / VOID_T / INT_T /
# OPERATE_RET——LINUX 平台副本已有、ESP32 没有），core 与 ai_components 都依赖它们；
# 该课由 dev 树 esp32 port 的 include/common 副本补齐（dev 树 iot_demo 即用此副本
# 构建通过），置于 adapter 目录之前使其优先命中。
list(INSERT PLATFORM_PUBINC_1 0 "${CMAKE_CURRENT_LIST_DIR}/tuya_open_sdk/tuyaos_adapter/tuyaopen/include/common")

# [tuyaopen-core integration] 契约层（C2，与 LINUX 平台同构）：core 与平台共用的
# ops 类型（tal_api_ops.h / tal_netif_ops.h / tal_network_ops.h /
# tal_bluetooth_ops.h + tal_bluetooth_def.h）物理放在
# tuya_open_sdk/tuyaos_adapter/tuyaopen/interface/；core 的 tal_*.h 反向 include
# 它们（经 PLATFORM_PUBINC → tuyaopen-headers 注入 core 编译单元），平台侧由
# tuyaopen_port 组件与 tal_ops.c 自带。ops 卡注册见 tuyaopen/tuyaopen-core_ops.c。
list(APPEND PLATFORM_PUBINC_1 "${CMAKE_CURRENT_LIST_DIR}/tuya_open_sdk/tuyaos_adapter/tuyaopen/interface")

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
    ${PLATFORM_PATH}/tuya_open_sdk/build/config
    ${PLATFORM_PATH}/tuya_open_sdk/tuyaos_adapter/include/utilities/include/
    ${BOARD_INC}
)

set(PLATFORM_PUBINC 
    ${PLATFORM_PUBINC_1}
    ${PLATFORM_PUBINC_2})
