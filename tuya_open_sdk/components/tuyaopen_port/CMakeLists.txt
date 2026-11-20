# ESP32 netif implementation (netif_esp.c). Shared core ops cards are built by
# src/tuya_service/core_port.
#
# tuyaopen_license.c is built by the tuyaos_adapter component; the kv card is
# provided by src/tuya_service/lib_kv.

set(TUYAOPEN_DIR "${CMAKE_CURRENT_LIST_DIR}/../../tuyaos_adapter/tuyaopen")
# The relative path assumes this platform is checked out at <delivery>/platform/ESP32.
set(TUYAOPEN_CORE_PORT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../tuyaopen-core/port/tuyaopen")

idf_component_register(
    SRCS
        "${TUYAOPEN_DIR}/netif_esp.c"
    INCLUDE_DIRS
        "${TUYAOPEN_DIR}"
        "${TUYAOPEN_CORE_PORT_DIR}/interface"
        "${TUYAOPEN_DIR}/include/common"
        "${TUYAOPEN_DIR}/include/system"
    REQUIRES
        # platform tkl headers used by the netif implementation
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
    # Keep netif_esp.o linked for libtuyaos.a's references.
    WHOLE_ARCHIVE
)
