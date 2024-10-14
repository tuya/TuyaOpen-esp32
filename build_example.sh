#!/usr/bin/env bash
# 参数说明：
# $1 - example name: echo_app_top
# $2 - example version: 1.0.0
# $3 - header files directory:
# $4 - libs directory:
# $5 - libs: tuyaos tuyaapp
# $6 - output directory:

PYTHON_CMD="python3"

print_not_null()
{
    # $1 为空，返回错误
    if [ x"$1" = x"" ]; then
        return 1
    fi

    echo "$1"
}

get_python_version()
{
    if command -v python3 &>/dev/null; then
        PYTHON_CMD=python3
    elif command -v python &>/dev/null && python --version | grep -q '^Python 3'; then
        PYTHON_CMD=python
    else
        echo "Python 3 is not installed."
        exit 1
    fi
}

set -e
cd `dirname $0`

EXAMPLE_NAME=$1
EXAMPLE_VER=$2
HEADER_DIR=$3
LIBS_DIR=$4
LIBS=$5
OUTPUT_DIR=$6
USER_CMD=$7
TARGET=$8

# echo EXAMPLE_NAME=$EXAMPLE_NAME
# echo EXAMPLE_VER=$EXAMPLE_VER
# echo HEADER_DIR=$HEADER_DIR
# echo LIBS_DIR=$LIBS_DIR
# echo LIBS=$LIBS
# echo OUTPUT_DIR=$OUTPUT_DIR
# echo USER_CMD=$USER_CMD
# echo TARGET=$TARGET

export TUYAOS_HEADER_DIR=$HEADER_DIR
export TUYAOS_LIBS_DIR=$LIBS_DIR
export TUYAOS_LIBS=$LIBS

APP_BIN_NAME=$EXAMPLE_NAME
APP_VERSION=$EXAMPLE_VER

if [ "$USER_CMD" = "build" ]; then
    USER_CMD=all
fi

TOP_DIR=$(pwd)
BULID_PATH=${TOP_DIR}/tuya_open_sdk/build
APP_BIN_DIR=${OUTPUT_DIR}


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

if [ ! -d ${IDF_TOOLS_PATH} ];then
    echo "IDF_TOOLS_PATH is empty ..."
    cd ${IDF_PATH}
    git submodule update --init --recursive
    . ./install.sh all
    cd -
fi

cd ${IDF_PATH}
. export.sh > /dev/null
cd -

CONTENT="
#ifndef MBEDTLS_THREADING_ALT_H
#define MBEDTLS_THREADING_ALT_H

typedef struct mbedtls_threading_mutex_t {
    void * mutex;
    char is_valid;
} mbedtls_threading_mutex_t;

#endif /* threading_alt.h */
"

# 指定文件名
FILENAME="threading_alt.h"
if [ ! -f ${IDF_PATH}/components/mbedtls/mbedtls/include/mbedtls/${FILENAME} ]; then
    echo "======== file ${FILENAME} not exist"

    echo -e "$CONTENT" > "$FILENAME"

    mv -f ${FILENAME} ${IDF_PATH}/components/mbedtls/mbedtls/include/mbedtls
fi

echo "Build Target: $TARGET"

cd tuya_open_sdk
if [ "${USER_CMD}" = "clean" ]; then
    idf.py clean
    rm -rf .target
    exit 0
elif [ "${USER_CMD}" = "menuconfig" ]; then
    idf.py menuconfig
    exit 0
fi

if [ -f ${TOP_DIR}/.target ]; then
    OLD_TARGET=$(cat ${TOP_DIR}/.target)
    echo OLD_TARGET: ${OLD_TARGET}
fi

if [ x"$TARGET" = x"esp32" ]; then
    cp -rf sdkconfig_esp32 sdkconfig.defaults
elif [ x"$TARGET" = x"esp32c3" ]; then
    cp -rf sdkconfig_esp32c3 sdkconfig.defaults
else
    echo "Target $TARGET not support"
    exit 1
fi

if [ ! -f ${TOP_DIR}/.target ] || [ x"${TARGET}" != x"${OLD_TARGET}" ] ; then
    echo "set-target ${TARGET}"
    idf.py set-target ${TARGET}
    rm -rf sdkconfig
    rm -rf sdkconfig.old
fi

echo ${TARGET} > ${TOP_DIR}/.target

# idf.py --verbose build

idf.py build

echo "Start generate ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin"

echo "**************************************************************************"
mkdir -p ${APP_BIN_DIR}

cp -rf ${BULID_PATH}/bootloader/bootloader.bin ${APP_BIN_DIR}/bootloader.bin
cp -rf ${BULID_PATH}/partition_table/partition-table.bin ${APP_BIN_DIR}/partition-table.bin
cp -rf ${BULID_PATH}/ota_data_initial.bin ${APP_BIN_DIR}/ota_data_initial.bin
cp -rf ${BULID_PATH}/tuya_open_sdk.bin ${APP_BIN_DIR}/${APP_BIN_NAME}.bin

#	echo "BULID_PATH=${BULID_PATH}"
get_python_version

${PYTHON_CMD} ${TOP_DIR}/tools/$TARGET/image_gen.py \
    ${APP_BIN_DIR}/bootloader.bin \
    ${APP_BIN_DIR}/partition-table.bin \
    ${APP_BIN_DIR}/ota_data_initial.bin \
    ${APP_BIN_DIR}/${APP_BIN_NAME}.bin \
    ${APP_BIN_DIR}/${APP_BIN_NAME}_QIO_${APP_VERSION}.bin

cp -rf ${APP_BIN_DIR}/${APP_BIN_NAME}.bin ${APP_BIN_DIR}/${APP_BIN_NAME}_UA_${APP_VERSION}.bin
cp -rf ${APP_BIN_DIR}/${APP_BIN_NAME}.bin ${APP_BIN_DIR}/${APP_BIN_NAME}_UG_${APP_VERSION}.bin

cp -rf ${BULID_PATH}/tuya_open_sdk.elf ${APP_BIN_DIR}/${APP_BIN_NAME}_${APP_VERSION}.elf
cp -rf ${BULID_PATH}/tuya_open_sdk.map ${APP_BIN_DIR}/${APP_BIN_NAME}_${APP_VERSION}.map

echo "*************************************************************************"
echo "*************************************************************************"
echo "*************************************************************************"
echo "*********************${APP_BIN_NAME}_QIO_${APP_VERSION}.bin**************"
echo "*************************************************************************"
echo "****************************COMPILE SUCCESS******************************"
echo "*************************************************************************"
