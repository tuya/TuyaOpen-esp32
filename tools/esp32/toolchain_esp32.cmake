
# set target system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR Linux)

# set toolchain
set(TOOLCHAIN_DIR "${IDF_TOOLS_PATH}/tools/xtensa-esp-elf/esp-14.2.0_20241119/xtensa-esp-elf/bin")
set(TOOLCHAIN_PRE "xtensa-esp32-elf-")

IF (WIN32)
    set(CMAKE_AR "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}ar.exe")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc.exe")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}g++.exe")
ELSE ()
    set(CMAKE_AR "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}ar")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}g++")
ENDIF ()

SET (CMAKE_C_COMPILER_WORKS 1)
SET (CMAKE_CXX_COMPILER_WORKS 1)

# set CFLAGS
set(CMAKE_C_FLAGS "-mlongcalls -Wno-frame-address  -ffunction-sections -fdata-sections -Wall -Werror -Wno-error=unused-function -Wno-error=incompatible-pointer-types -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=deprecated-declarations -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-enum-conversion -gdwarf-4 -ggdb -Os -freorder-blocks -fstrict-volatile-bitfields -fno-jump-tables -fno-tree-switch-conversion -std=gnu17 -Wno-old-style-declaration")
