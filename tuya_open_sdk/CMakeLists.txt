# For more information about build system see
# https://docs.espressif.com/projects/esp-idf/en/latest/api-guides/build-system.html
# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS tuyaos_adapter)
# Generic drop-in port: dirs listed in $TUYAOS_EXTRA_COMPONENT_DIRS (space-separated) become
# IDF component ROOTS — every subdir under each is auto-registered as a component. Lets an app
# add project-private, esp-idf-visible components with NO edit to this file. Set by the TuyaOpen
# CLI (tools/cli_command/cli_build.py) to <app>/components when that dir exists.
if(NOT "$ENV{TUYAOS_EXTRA_COMPONENT_DIRS}" STREQUAL "")
    string(REPLACE " " ";" _tuyaos_extra_comp_dirs "$ENV{TUYAOS_EXTRA_COMPONENT_DIRS}")
    list(APPEND EXTRA_COMPONENT_DIRS ${_tuyaos_extra_comp_dirs})
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(tuya_open_sdk)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=incompatible-pointer-types" APPEND)

# esp-dl uses C-style typedefs around C++ aggregate types. GCC 16 diagnoses
# this as non-c-typedef-for-linkage; it is harmless for this managed component
# but becomes fatal because the project enables -Werror globally.
if(TARGET __idf_espressif__esp-dl)
    target_compile_options(__idf_espressif__esp-dl PRIVATE
        -Wno-error=non-c-typedef-for-linkage)
endif()

# The new ESP-NimBLE controller builds an IDF-side mempool in controller-only
# mode.  TuyaOpen's NimBLE host has the legacy symbols as well, so namespace
# only the IDF component target to keep the two implementations separate.
if(CONFIG_SOC_ESP_NIMBLE_CONTROLLER AND NOT CONFIG_BT_NIMBLE_ENABLED)
    target_compile_definitions(__idf_bt PRIVATE
        g_os_mempool_list=idf_g_os_mempool_list
        os_mempool_init=idf_os_mempool_init
        os_mempool_ext_init=idf_os_mempool_ext_init
        os_mempool_unregister=idf_os_mempool_unregister
        os_mempool_clear=idf_os_mempool_clear
        os_mempool_is_sane=idf_os_mempool_is_sane
        os_memblock_from=idf_os_memblock_from
        os_memblock_get=idf_os_memblock_get
        os_memblock_put_from_cb=idf_os_memblock_put_from_cb
        os_memblock_put=idf_os_memblock_put
        os_mempool_info_get_next=idf_os_mempool_info_get_next
        os_mempool_module_init=idf_os_mempool_module_init
    )
endif()

if(NOT "$ENV{BUILD_PARAM_DIR}" STREQUAL "")
    include($ENV{BUILD_PARAM_DIR}/build_param.cmake)
    idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_PATH=${OPEN_ROOT}/src/liblvgl/v9/conf/lv_conf.h" APPEND)
    # Add include paths for lvgl to find tuya headers
    idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_CURRENT_LIST_DIR}/tuyaos_adapter/include/utilities/include" APPEND)
    string(REPLACE " " ";" OPEN_HEADER_DIR_LIST ${OPEN_HEADER_DIR})
    foreach(header_dir ${OPEN_HEADER_DIR_LIST})
        idf_build_set_property(COMPILE_OPTIONS "-I${header_dir}" APPEND)
    endforeach()
    # mbedtls 4.x hides legacy API decls in private headers behind this switch.
    # Empty body must match private_access.h's `#define` or IDF's -Werror
    # turns the benign redefine into an error inside its own mbedtls build.
    idf_build_set_property(COMPILE_OPTIONS "-DMBEDTLS_DECLARE_PRIVATE_IDENTIFIERS=" APPEND)

    # IDF 6.x slimmed the legacy "driver" component down to i2c/touch/twai and
    # it no longer re-exports the split esp_driver_* components' headers.
    # tuyaos_adapter and the managed components (esp_codec_dev, esp_lcd_touch,
    # esp_io_expander, ...) still `REQUIRES driver` and include driver/*.h on
    # every target, so give every TU the split driver include dirs. Managed
    # components can't be patched (component-manager hash check), hence the
    # global injection.
    foreach(drv gpio i2c i2s spi uart ledc gptimer rmt sdspi sdmmc usb_serial_jtag)
        idf_build_set_property(COMPILE_OPTIONS "-I${IDF_PATH}/components/esp_driver_${drv}/include" APPEND)
    endforeach()
    # IDF 6.x keeps the legacy SDMMC headers in a separate include directory;
    # esp_hosted still includes driver/sdmmc_host.h directly.
    if(EXISTS "${IDF_PATH}/components/esp_driver_sdmmc/legacy/include")
        idf_build_set_property(COMPILE_OPTIONS
            "-I${IDF_PATH}/components/esp_driver_sdmmc/legacy/include" APPEND)
    endif()
    # ... whose headers pull hal/*_types.h from the esp_hal_* split
    # components (and hal/ shared helpers) transitively.
    foreach(hal gpio i2c i2s gpspi uart ledc gptimer rmt sdspi timg cam)
        idf_build_set_property(COMPILE_OPTIONS "-I${IDF_PATH}/components/esp_hal_${hal}/include" APPEND)
    endforeach()
    # IDF 6.x split the former generic YUV422 camera format into the four
    # byte-order-specific formats.  esp_video 0.8.x still uses the old name;
    # retain its historical YUYV default without modifying the managed copy.
    if(EXISTS "${IDF_PATH}/components/esp_hal_cam/include/hal/cam_ctlr_types.h")
        idf_build_set_property(COMPILE_OPTIONS
            "-DCAM_CTLR_COLOR_YUV422=CAM_CTLR_COLOR_YUV422_YUYV" APPEND)
    endif()
    idf_build_set_property(COMPILE_OPTIONS "-I${IDF_PATH}/components/hal/include" APPEND)
    idf_build_set_property(COMPILE_OPTIONS "-I${IDF_PATH}/components/hal/platform_port/include" APPEND)
    # IDF 6.x trimmed transitive includes: TUs that used to see
    # esp_idf_version.h (via old driver/ headers) no longer do, breaking
    # `#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(...)` gates in older
    # managed components (they fall through to pre-4.4 branches like
    # HSPI_HOST). Force-include it everywhere — idempotent, tiny.
    # Two separate options: a single "-include <path>" element would be
    # quoted as ONE argv entry by cmake/ninja and gcc would reject it.
    idf_build_set_property(COMPILE_OPTIONS "-include" APPEND)
    idf_build_set_property(COMPILE_OPTIONS "${IDF_PATH}/components/esp_common/include/esp_idf_version.h" APPEND)

    # ROM_MD5=n keeps Tuya's legacy mbedtls_md5_* API available. On IDF 6.x
    # that setting makes esp_config.h emit an empty definition of this macro,
    # while tf-psa-crypto later defines it as 1. Predefine the final value so
    # both definitions are identical and harmless under -Werror.
    idf_build_set_property(COMPILE_OPTIONS "-DMBEDTLS_PSA_BUILTIN_ALG_HMAC=1" APPEND)
endif()
