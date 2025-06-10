#!/usr/bin/env bash
# 参数说明：
# $1 - params path: echo_app_top
# $2 - user cmd: build / clean / ...

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

BUILD_PARAM_DIR=$1
BUILD_PARAM_FILE=$BUILD_PARAM_DIR/build_param.config
. $BUILD_PARAM_FILE

export BUILD_PARAM_DIR=$BUILD_PARAM_DIR

EXAMPLE_NAME=$CONFIG_PROJECT_NAME
EXAMPLE_VER=$CONFIG_PROJECT_VERSION
HEADER_DIR=$OPEN_HEADER_DIR
LIBS_DIR=$OPEN_LIBS_DIR
LIBS=$PLATFORM_NEED_LIBS
OUTPUT_DIR=$BIN_OUTPUT_DIR
USER_CMD=$2
TARGET=$PLATFORM_CHIP
BOARD_NAME=$PLATFORM_BOARD

# echo EXAMPLE_NAME=$EXAMPLE_NAME
# echo EXAMPLE_VER=$EXAMPLE_VER
# echo HEADER_DIR=$HEADER_DIR
# echo LIBS_DIR=$LIBS_DIR
# echo LIBS=$LIBS
# echo OUTPUT_DIR=$OUTPUT_DIR
# echo USER_CMD=$USER_CMD
# echo TARGET=$TARGET
# echo BOARD_NAME=$BOARD_NAME
# exit 0

export TUYAOS_HEADER_DIR=$HEADER_DIR
export TUYAOS_LIBS_DIR=$LIBS_DIR
export TUYAOS_LIBS=$LIBS

APP_BIN_NAME=$EXAMPLE_NAME
APP_VERSION=$EXAMPLE_VER

if [ "$USER_CMD" = "build" ]; then
    USER_CMD=all
elif [ "${USER_CMD}" = "clean" ]; then

    cd tuya_open_sdk
    rm -rf build
    
    rm -rf sdkconfig
    rm -rf sdkconfig.old
    rm -rf sdkconfig.defaults

    rm -rf .target
    rm -rf .app
    rm -rf .prepare
    exit 0
fi

TOP_DIR=$(cd "$(dirname "$0")" && pwd)
BULID_PATH=${TOP_DIR}/tuya_open_sdk/build
APP_BIN_DIR=${OUTPUT_DIR}
export TUYAOS_BOARD_PATH=$TOP_DIR/../../boards/ESP32


${TOP_DIR}/platform_prepare.sh $TARGET
if [ $? -ne 0 ]; then
    echo "platform_prepare.sh failed."
    exit 1
fi

if command -v idf.py &>/dev/null; then
    echo "Use existing esp-idf tools."
else
    IDF_PATH=${TOP_DIR}/esp-idf
    IDF_TOOLS_PATH=${TOP_DIR}/.espressif
    export IDF_PATH
    export IDF_TOOLS_PATH
    cd ${IDF_PATH}
    . ${IDF_PATH}/export.sh > /dev/null
    cd -
fi


echo "Build Target: $TARGET"

cd tuya_open_sdk
if [ "${USER_CMD}" = "menuconfig" ]; then
    idf.py menuconfig
    exit 0
fi

# app check
if [ -f ${TOP_DIR}/.app ]; then
    OLD_APP_BIN_NAME=$(cat ${TOP_DIR}/.app)
    echo "OLD_APP_BIN_NAME: ${OLD_APP_BIN_NAME}"
fi

echo ${APP_BIN_NAME} > ${TOP_DIR}/.app
if [ "$OLD_APP_BIN_NAME" != "$APP_BIN_NAME" ]; then
    rm -rf build
    echo "set-target ${TARGET}"
    idf.py set-target ${TARGET}
    rm -rf sdkconfig
    rm -rf sdkconfig.old
fi

if [ -f ${TOP_DIR}/.target ]; then
    OLD_TARGET=$(cat ${TOP_DIR}/.target)
    echo OLD_TARGET: ${OLD_TARGET}
fi

if [ x"$TARGET" = x"esp32" ]; then
    cp -rf sdkconfig_esp32 sdkconfig.defaults
elif [ x"$TARGET" = x"esp32c3" ]; then
    cp -rf sdkconfig_esp32c3 sdkconfig.defaults
elif [ x"$TARGET" = x"esp32s3" ]; then
    cp -rf sdkconfig_esp32s3 sdkconfig.defaults
else
    echo "Target $TARGET not support"
    exit 1
fi

if [ x"$CONFIG_PLATFORM_FLASHSIZE_16M" = x"y" ]; then
    echo "set flash size 16M"
    rm -rf partitions.csv
    cp -rf partitions_16M.csv partitions.csv
elif [ x"$CONFIG_PLATFORM_FLASHSIZE_8M" = x"y" ]; then
    echo "set flash size 8M"
    rm -rf partitions.csv
    cp -rf partitions_8M.csv partitions.csv
else
    echo "set flash size 4M"
    rm -rf partitions.csv
    cp -rf partitions_4M.csv partitions.csv
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

${PYTHON_CMD} ${TOP_DIR}/tools/gen_image.py \
    ${TARGET} \
    ${BULID_PATH} \
    ${APP_BIN_DIR}/${APP_BIN_NAME}_QIO_${APP_VERSION}.bin

cp -rf ${APP_BIN_DIR}/${APP_BIN_NAME}.bin ${APP_BIN_DIR}/${APP_BIN_NAME}_UA_${APP_VERSION}.bin
cp -rf ${APP_BIN_DIR}/${APP_BIN_NAME}.bin ${APP_BIN_DIR}/${APP_BIN_NAME}_UG_${APP_VERSION}.bin

cp -rf ${BULID_PATH}/tuya_open_sdk.elf ${APP_BIN_DIR}/${APP_BIN_NAME}_${APP_VERSION}.elf
cp -rf ${BULID_PATH}/tuya_open_sdk.map ${APP_BIN_DIR}/${APP_BIN_NAME}_${APP_VERSION}.map

if [ -f ${BULID_PATH}/srmodels/srmodels.bin ]; then
    cp -rf ${BULID_PATH}/srmodels/srmodels.bin ${APP_BIN_DIR}/srmodels.bin
fi
echo "*************************************************************************"
echo "*************************************************************************"
echo "*************************************************************************"
echo "*********************${APP_BIN_NAME}_QIO_${APP_VERSION}.bin**************"
echo "*************************************************************************"
echo "****************************COMPILE SUCCESS******************************"
echo "*************************************************************************"
