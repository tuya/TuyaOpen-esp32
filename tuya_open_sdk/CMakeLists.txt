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

if(NOT "$ENV{BUILD_PARAM_DIR}" STREQUAL "")
    include($ENV{BUILD_PARAM_DIR}/build_param.cmake)
    idf_build_set_property(COMPILE_OPTIONS "-DLV_CONF_PATH=${OPEN_ROOT}/src/liblvgl/v9/conf/lv_conf.h" APPEND)
    # Add include paths for lvgl to find tuya headers
    idf_build_set_property(COMPILE_OPTIONS "-I${CMAKE_CURRENT_LIST_DIR}/tuyaos_adapter/include/utilities/include" APPEND)
    string(REPLACE " " ";" OPEN_HEADER_DIR_LIST ${OPEN_HEADER_DIR})
    foreach(header_dir ${OPEN_HEADER_DIR_LIST})
        idf_build_set_property(COMPILE_OPTIONS "-I${header_dir}" APPEND)
    endforeach()
endif()