# set target system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR Linux)

# set toolchain (RISC-V for ESP32-S31)
set(TOOLCHAIN_DIR "${IDF_TOOLS_PATH}/tools/riscv32-esp-elf/esp-16.1.0_20260609/riscv32-esp-elf/bin")
set(TOOLCHAIN_PRE "riscv32-esp-elf-")

IF (WIN32)
    set(CMAKE_AR "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}ar.exe")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc.exe")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}g++.exe")
ELSE ()
    set(CMAKE_AR "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}ar")
    set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}g++")
ENDIF ()

set(CMAKE_C_COMPILER_AR "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc-ar")
set(CMAKE_CXX_COMPILER_AR "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc-ar")
set(CMAKE_C_COMPILER_RANLIB "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc-ranlib")
set(CMAKE_CXX_COMPILER_RANLIB "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc-ranlib")

SET (CMAKE_C_COMPILER_WORKS 1)
SET (CMAKE_CXX_COMPILER_WORKS 1)

# set CFLAGS
set(CMAKE_C_FLAGS "-mcmodel=medany -march=rv32imafcb_zicsr_zifencei_zcb_zcmp_zcmt_xesploop_xespv -mabi=ilp32f -ffunction-sections -fdata-sections -Wall -Werror=all -Wno-error=unused-function -Wno-error=incompatible-pointer-types -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=deprecated-declarations -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-enum-conversion -gdwarf-4 -ggdb -Os -freorder-blocks -fstrict-volatile-bitfields -fno-jump-tables -fno-tree-switch-conversion -std=gnu17 -Wno-old-style-declaration")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -specs=picolibc.specs -D__PICOLIBC_ERRNO_FUNCTION=__errno -D__STDC_WANT_LIB_EXT1__=0")
