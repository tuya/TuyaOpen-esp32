
# set target system
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR Linux)

# set toolchain
message(STATUS "[TOP] PLATFORM_PATH: ${PLATFORM_PATH}")

set(TOOLCHAIN_DIR "${PLATFORM_PATH}/.espressif/tools/riscv32-esp-elf/esp-14.2.0_20241119/riscv32-esp-elf/bin")
set(TOOLCHAIN_PRE "riscv32-esp-elf-")

set(CMAKE_AR "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}ar")
set(CMAKE_C_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}gcc")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_DIR}/${TOOLCHAIN_PRE}g++")

SET (CMAKE_C_COMPILER_WORKS 1)
SET (CMAKE_CXX_COMPILER_WORKS 1)

# set CFLAGS
set(CMAKE_C_FLAGS "-march=rv32imc_zicsr_zifencei -Wall -Wextra -Wwrite-strings -Wformat=2 -Wno-format-nonliteral -Wvla -Wlogical-op -Wshadow -Wformat-signedness -Wformat-overflow=2 -Wformat-truncation -ffunction-sections -fdata-sections -Wall -Wno-error=unused-function -Wno-error=incompatible-pointer-types -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=deprecated-declarations -Wextra -Wno-unused-parameter -Wno-sign-compare -Wno-enum-conversion -gdwarf-4 -ggdb -nostartfiles -Os -freorder-blocks -fstrict-volatile-bitfields -fno-jump-tables -fno-tree-switch-conversion -std=gnu17 -Wno-old-style-declaration")
