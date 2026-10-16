# [交付仓 ESP32 接入] ops 卡组件：编入 tuya_open_sdk/tuyaos_adapter/tuyaopen/ 下的
# tuyaopen-core_ops.c（卡注册）与 netif_esp.c（netif 实现，esp32 无 tkl_wired）。
#
# 与 LINUX 平台同构：契约层在 tuyaopen/interface/，卡注册集中在 tuyaopen/tuyaopen-core_ops.c，
# 平台实现文件（tuyaos_adapter/src/**）保持原样零改动。
# core 的符号由 main 侧预编译 libtuyaos.a 提供（PLATFORM_NEED_LIBS），本组件只提供
# tal_*_ops_platform 卡实例，供 core 分发器 extern 引用。
#
# 注：tuyaopen_license.c 由 tuyaos_adapter 组件自己编（不在此处 glob，避免重复）；
# kv 卡由 src/tuya_service/lib_kv 组件提供，与 LINUX 口径一致。

set(TUYAOPEN_DIR "${CMAKE_CURRENT_LIST_DIR}/../../tuyaos_adapter/tuyaopen")

idf_component_register(
    SRCS
        "${TUYAOPEN_DIR}/tuyaopen-core_ops.c"
        "${TUYAOPEN_DIR}/netif_esp.c"
    INCLUDE_DIRS
        "${TUYAOPEN_DIR}"
        "${TUYAOPEN_DIR}/interface"
        "${TUYAOPEN_DIR}/include/common"
        "${TUYAOPEN_DIR}/include/system"
    REQUIRES
        # 平台 tkl 头（tkl_output.h / tkl_memory.h / tkl_system.h / tkl_network.h …）：
        # 薄卡成员绑定 tkl_*，需要 adapter 组件的 include 面
        tuyaos_adapter
        esp_wifi
        esp_event
        esp_netif
        nvs_flash
        lwip
        esp_timer
        esp_partition
        app_update
        mbedtls
    # 卡实例必须"无条件进链接"：libtuyaos.a（预编译，位于链接行末尾）extern 引用
    # tal_*_ops_platform，而本组件库排在它之前——普通归档扫描到本库时还没有任何
    # 未决引用，整个库不会被拉入（实测 undefined reference: tal_api_ops_platform 等）。
    # WHOLE_ARCHIVE 让本库对象全部进链接，同时其自身的未决符号（lwip_gethostbyname
    # 等）由链接行上后续的 IDF 库副本解析。
    WHOLE_ARCHIVE
)
