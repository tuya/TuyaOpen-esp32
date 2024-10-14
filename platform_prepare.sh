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

rm -rf .target

rm -rf tuya_open_sdk/sdkconfig
rm -rf tuya_open_sdk/sdkconfig.old
rm -rf tuya_open_sdk/sdkconfig.defaults
rm -rf tuya_open_sdk/build


IDF_PATH=${TOP_DIR}/esp-idf
IDF_TOOLS_PATH=${TOP_DIR}/.espressif
export IDF_PATH
export IDF_TOOLS_PATH
if [ ! -d ${IDF_PATH} ]; then
    echo "IDF_PATH is empty"
    git clone --recursive https://github.com/espressif/esp-idf -b v5.2.1 --depth=1
    if [ $? -ne 0 ]; then
        echo "git clone esp-idf failed ..."
        exit 1
    else
        cd ${IDF_PATH}
        git submodule update --init --recursive
        . ./install.sh all
        cd -
        
        echo "git clone esp-idf success ..."
    fi
fi

if [ ! -d ${IDF_TOOLS_PATH} ] || [ ! -d ${IDF_TOOLS_PATH}/tools ];then
    echo "IDF_TOOLS_PATH is empty ..."
    cd ${IDF_PATH}
    git submodule update --init --recursive
    . ./install.sh all
    cd -
fi

echo "Run platform prepare success ..."
