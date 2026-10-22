# Ops card component: builds core's port/tuyaopen/tuyaopen-core_ops.c (cards and
# contract headers) plus the platform-local netif_esp.c (netif implementation;
# esp32 has no tkl_wired).
#
# Core symbols come from the prebuilt libtuyaos.a (PLATFORM_NEED_LIBS); this
# component provides the core_*_ops_platform instances referenced by core.
# tuyaopen_license.c is built by the tuyaos_adapter component; the kv card is
# provided by src/tuya_service/lib_kv.

set(TUYAOPEN_DIR "${CMAKE_CURRENT_LIST_DIR}/../../tuyaos_adapter/tuyaopen")
# The relative path assumes this platform is checked out at <delivery>/platform/ESP32.
set(TUYAOPEN_CORE_PORT_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../tuyaopen-core/port/tuyaopen")

idf_component_register(
    SRCS
        "${TUYAOPEN_CORE_PORT_DIR}/tuyaopen-core_ops.c"
        "${TUYAOPEN_DIR}/netif_esp.c"
    INCLUDE_DIRS
        "${TUYAOPEN_DIR}"
        "${TUYAOPEN_CORE_PORT_DIR}/interface"
        "${TUYAOPEN_DIR}/include/common"
        "${TUYAOPEN_DIR}/include/system"
    REQUIRES
        # platform tkl headers used by the card members
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
    # The card instances must always be linked: libtuyaos.a (prebuilt, later on
    # the link line) references core_*_ops_platform, and this component library
    # is scanned before any reference is pending. WHOLE_ARCHIVE keeps all of
    # its objects in the link.
    WHOLE_ARCHIVE
)
