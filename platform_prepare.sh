#!/usr/bin/env bash

set -e

TARGET=$1
echo TARGET=$TARGET
if [ x"$TARGET" = x"" ]; then
    echo "Usage: <./ platform_prepare.sh esp32|esp32s2|esp32s3|esp32c2|esp32c3|esp32c6>"
    exit 1
fi

TOP_DIR=$(pwd)
echo $TOP_DIR

echo "Start platform prepare ..."

rm -rf ${TOP_DIR}/.target

rm -rf ${TOP_DIR}/tuya_open_sdk/sdkconfig
rm -rf ${TOP_DIR}/tuya_open_sdk/sdkconfig.old
rm -rf ${TOP_DIR}/tuya_open_sdk/sdkconfig.defaults
rm -rf ${TOP_DIR}/tuya_open_sdk/build

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
    git clone --recursive https://github.com/espressif/esp-idf -b v5.4 --depth=1 ${IDF_PATH}
    if [ $? -ne 0 ]; then
        return 1
    fi

    return 0
}

if [ ! -d ${IDF_PATH} ]; then
    echo "IDF_PATH is empty"
    
    enable_mirror
    download_esp_idf

    if [ $? -ne 0 ]; then
        echo "git clone esp-idf failed ..."
        exit 1
    fi
fi

disable_mirror

echo "Run platform prepare success ..."
