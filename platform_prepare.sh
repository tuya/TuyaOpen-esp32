#!/usr/bin/env bash

set -e

TARGET=$1
echo TARGET=$TARGET
if [ x"$TARGET" = x"" ]; then
    echo "Use default target: esp32s3"
    TARGET=esp32s3
    echo TARGET=$TARGET
fi

TOP_DIR=$(cd "$(dirname "$0")" && pwd)
echo $TOP_DIR

if [ -f ${TOP_DIR}/.prepare ]; then
    OLD_TARGET=$(cat ${TOP_DIR}/.prepare)
    echo "old target: ${OLD_TARGET}"
    if [ x"${TARGET}" == x"${OLD_TARGET}" ] ; then
        echo "Use existing platform prepare."
        exit 0
    fi

    rm -rf .prepare
fi

echo "Start platform prepare ..."

rm -rf ${TOP_DIR}/.target

rm -rf ${TOP_DIR}/tuya_open_sdk/sdkconfig
rm -rf ${TOP_DIR}/tuya_open_sdk/sdkconfig.old
rm -rf ${TOP_DIR}/tuya_open_sdk/sdkconfig.defaults
rm -rf ${TOP_DIR}/tuya_open_sdk/build

MORROR=0

IDF_GITEE_TOOLS_PATH=${TOP_DIR}/esp-gitee-tools
IDF_VERSION="v5.4"

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
    git clone --recursive https://github.com/espressif/esp-idf -b $IDF_VERSION --depth=1 ${IDF_PATH}
    if [ $? -ne 0 ]; then
        return 1
    fi

    return 0
}

function update_idf_tools()
{
    echo "Installing target: $TARGET ..."
    cd ${IDF_PATH}
    git submodule update --init --recursive
    ./install.sh $TARGET
    if [ $? -ne 0 ]; then
        return 1
    fi

    cd -
    return 0
}

if command -v idf.py &>/dev/null; then
    echo "Use existing esp-idf tools."
else
    IDF_PATH=${TOP_DIR}/esp-idf
    IDF_TOOLS_PATH=${TOP_DIR}/.espressif
    export IDF_PATH
    export IDF_TOOLS_PATH
fi

enable_mirror

if [ ! -d ${IDF_PATH} ]; then
    echo "Downloading esp_idf ..."
    echo "
    NOTE:
        If esp-idf is installed and $IDF_VERSION is available,
        You can stop this process and start esp-idf.
    "
    echo -n "5..."; sleep 1
    echo -n "4..."; sleep 1
    echo -n "3..."; sleep 1
    echo -n "2..."; sleep 1
    echo "1..."; sleep 1

    download_esp_idf
    if [ $? -ne 0 ]; then
        echo "git clone esp-idf failed ..."
        exit 1
    fi
fi

update_idf_tools
if [ $? -ne 0 ]; then
    echo "update idf tools failed ..."
    exit 1
fi

disable_mirror

echo ${TARGET} > ${TOP_DIR}/.prepare

echo "Run platform prepare success ..."
