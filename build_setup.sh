#!/usr/bin/env bash

set -e


PROJ_NAME=$1
TARGET=$2
FRAMEWORK=$3
CHIP=$4

echo TARGET=$TARGET
if [ x"$TARGET" = x"" ]; then
    exit 1
fi

echo PROJ_NAME=$PROJ_NAME
echo TARGET=$TARGET
echo FRAMEWORK=$FRAMEWORK
echo CHIP=$CHIP

TOP_DIR=$(dirname "$0")
echo $TOP_DIR

echo "Start board build_setup ..."

${TOP_DIR}/platform_prepare.sh $CHIP  # default esp32s3
if [ $? -ne 0 ]; then
    echo "platform_prepare.sh failed."
    exit 1
fi

function prepare_build()
{
    if command -v idf.py &>/dev/null; then
        echo "Use existing esp-idf tools."
    else
        IDF_PATH=${TOP_DIR}/esp-idf
        IDF_TOOLS_PATH=${TOP_DIR}/.espressif
        export IDF_PATH
        export IDF_TOOLS_PATH
        cd ${IDF_PATH}
        . export.sh > /dev/null
        cd -
    fi

    echo "Build CHIP: $CHIP"
    rm -rf .prepare
    rm -rf .target
    rm -rf .app

    cd ${TOP_DIR}/tuya_open_sdk
    
    rm -rf sdkconfig.defaults
    rm -rf sdkconfig
    rm -rf sdkconfig.old


    if [ x"$CHIP" = x"esp32" ]; then
        cp -rf sdkconfig_esp32 sdkconfig.defaults
    elif [ x"$CHIP" = x"esp32c3" ]; then
        cp -rf sdkconfig_esp32c3 sdkconfig.defaults
    elif [ x"$CHIP" = x"esp32s3" ]; then
        cp -rf sdkconfig_esp32s3 sdkconfig.defaults
    else
        echo "Target $CHIP not support"
        exit 1
    fi

    echo "set-target ${CHIP}"
    idf.py set-target ${CHIP}

}

TOP_DIR=$(dirname "$0")
if [ -d "${TOP_DIR}/tuya_open_sdk/build" ]; then
    echo "Directory build exists"

    if [ ! -f "${TOP_DIR}/tuya_open_sdk/build/config/sdkconfig.h" ]; then
        echo "Clean build directory"
        rm -rf ${TOP_DIR}/tuya_open_sdk/build
        
        prepare_build
    fi
else
    echo "Directory build does not exist!"

    prepare_build
fi

exit 0
