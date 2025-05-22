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
endif()

target_include_directories(mbedcrypto
    PUBLIC
        $ENV{IDF_PATH}/components/mbedtls/port/mbedtls_rom
    )
