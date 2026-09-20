set(include_dirs
    ".")

if(NOT "$ENV{TUYAOS_HEADER_DIR}" STREQUAL "")
    string(REPLACE " " ";" TUYAOS_HEADER_DIR_LIST $ENV{TUYAOS_HEADER_DIR})

    foreach(head ${TUYAOS_HEADER_DIR_LIST})
        list(APPEND include_dirs ${head})
    endforeach()
endif()

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ${include_dirs})

if(NOT "$ENV{TUYAOS_LIBS}" STREQUAL "")
    set(TUYA_OPENSDK_LIB_PATH $ENV{TUYAOS_LIBS_DIR})

    string(REPLACE " " ";" TUYAOS_LIBS_LIST $ENV{TUYAOS_LIBS})

    foreach(lib ${TUYAOS_LIBS_LIST})
        # message("TUYAOS_LIBS value: ${lib}")
        add_prebuilt_library(${lib} "${TUYA_OPENSDK_LIB_PATH}/lib${lib}.a" REQUIRES tuyaos_adapter mbedtls)
        target_link_libraries(${COMPONENT_LIB} PUBLIC ${lib})
    endforeach()

    if(IDF_VERSION_MAJOR GREATER_EQUAL 6)
        # Resolve circular dependencies between the S31 / IDF 6 archives.
        set(TUYA_PREBUILT_LIB_PATHS "")
        foreach(lib ${TUYAOS_LIBS_LIST})
            list(APPEND TUYA_PREBUILT_LIB_PATHS "${TUYA_OPENSDK_LIB_PATH}/lib${lib}.a")
        endforeach()
        target_link_libraries(${COMPONENT_LIB} INTERFACE
            "-Wl,--start-group"
            ${TUYA_PREBUILT_LIB_PATHS}
            "-Wl,--end-group"
        )
    else()
        # ESP-IDF v5.4 link contract from platform master.
        target_link_libraries(${COMPONENT_LIB} INTERFACE
            "-u lv_mem_init"
            "-u lv_mem_deinit"
            "-u lv_mem_add_pool"
            "-u lv_malloc_core"
            "-u lv_realloc_core"
            "-u lv_free_core"
            "-u lv_mem_monitor_core"
            "-u lv_mem_test_core"
            "-u lv_port_log_print"
            "-u tdl_audio_driver_register"
            "-u tdl_tp_device_register"
            "-u tdl_disp_device_register"
        )
    endif()
endif()

# IDF >= 6.x folds mbedcrypto into the single "mbedtls" component target;
# v5.4 keeps the separate mbedcrypto component that owns mbedtls_rom.
if(TARGET mbedcrypto)
    set(TUYA_MBED_TARGET mbedcrypto)
else()
    set(TUYA_MBED_TARGET mbedtls)
endif()
target_include_directories(${TUYA_MBED_TARGET}
    PUBLIC
        $ENV{IDF_PATH}/components/mbedtls/port/mbedtls_rom
    )
