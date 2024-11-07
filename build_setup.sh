#!/usr/bin/env bash

set -e


PROJ_NAME=$1
TARGET=$2
CHIP=$3
echo TARGET=$TARGET
if [ x"$TARGET" = x"" ]; then
    exit 1
fi

 
echo PROJ_NAME=$PROJ_NAME
echo TARGET=$TARGET
echo CHIP=$CHIP

TOP_DIR=$(dirname "$0")
echo $TOP_DIR

echo "Start board build_setup ..."

MORROR=0

IDF_GITEE_TOOLS_PATH=${TOP_DIR}/esp-gitee-tools
IDF_PATH=${TOP_DIR}/esp-idf
IDF_TOOLS_PATH=${TOP_DIR}/.espressif
export IDF_PATH
export IDF_TOOLS_PATH

PYTHON_CMD="python3"

function get_country_code()
{
    echo "get_country_code ..."
    if command -v python3 &>/dev/null; then
        PYTHON_CMD=python3
    elif command -v python &>/dev/null && python --version | grep -q '^Python 3'; then
        PYTHON_CMD=python
    else
        echo "Python 3 is not installed."
        exit 1
    fi

    MORROR=$(${PYTHON_CMD} ${TOP_DIR}/tools/get_conutry.py)
}

function enable_mirror()
{
    get_country_code

    if [ x"$MORROR" = x"1" ]; then
        echo "enable mirror"
        if [ ! -d ${IDF_GITEE_TOOLS_PATH} ]; then
            echo "clone esp-gitee-tools"
            git clone https://gitee.com/EspressifSystems/esp-gitee-tools.git ${IDF_GITEE_TOOLS_PATH}
        fi
        
        # enable mirror
        bash ${IDF_GITEE_TOOLS_PATH}/jihu-mirror.sh set
    fi
}

function disable_mirror()
{
    if [ x"$MORROR" = x"1" ]; then
        echo "disable mirror"
        bash ${IDF_GITEE_TOOLS_PATH}/jihu-mirror.sh unset
    fi
}

function download_esp_idf()
{
    echo "download esp-idf ..."
    git clone --recursive https://github.com/espressif/esp-idf -b v5.2.1 --depth=1 ${IDF_PATH}
    if [ $? -ne 0 ]; then
        return 1
    fi

    return 0
}

function download_esp_idf_tools()
{
    echo "download esp-idf-tools ..."

    cd ${IDF_PATH}
    git submodule update --init --recursive
    cd -
        
    if [ x"$MORROR" = x"1" ]; then
        bash ${IDF_GITEE_TOOLS_PATH}/install.sh ${IDF_PATH}
    else
        bash ${IDF_PATH}/install.sh all
    fi

}


if [ ! -d ${IDF_PATH} ]; then
    echo "IDF_PATH is empty"
    
    enable_mirror
    download_esp_idf

    if [ $? -ne 0 ]; then
        echo "git clone esp-idf failed ..."
        exit 1
    else
        download_esp_idf_tools
    fi
fi

if [ ! -d ${IDF_TOOLS_PATH} ] || [ ! -d ${IDF_TOOLS_PATH}/tools ];then
    echo "IDF_TOOLS_PATH is empty ..."

    enable_mirror
    download_esp_idf_tools
fi

if [ x"$CHIP" = x"esp32" ]; then
    TOOLCHAIN_PATH=${IDF_TOOLS_PATH}/tools/xtensa-esp-elf
elif [ x"$CHIP" = x"esp32c3" ]; then
    TOOLCHAIN_PATH=${IDF_TOOLS_PATH}/tools/riscv32-esp-elf
else
    echo "Target $TARGET not support"
    exit 1
fi

if [ ! -d ${TOOLCHAIN_PATH} ]; then
    echo "TOOLCHAIN_PATH is empty ..."
    enable_mirror
    download_esp_idf_tools
fi

disable_mirror
echo "Run build setup success ..."

exit 0
