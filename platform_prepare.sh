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
    else
        echo "Remove old platform prepare."
    fi
fi

echo "Start platform prepare ..."

rm -rf ${TOP_DIR}/.target
rm -rf ${TOP_DIR}/.app
rm -rf ${TOP_DIR}/.prepare
rm -rf ${TOP_DIR}/tuya_open_sdk/sdkconfig
rm -rf ${TOP_DIR}/tuya_open_sdk/sdkconfig.old
rm -rf ${TOP_DIR}/tuya_open_sdk/sdkconfig.defaults
rm -rf ${TOP_DIR}/tuya_open_sdk/build

MIRROR=0

IDF_GITEE_TOOLS_PATH=${TOP_DIR}/esp-gitee-tools
IDF_VERSION="v5.4"

PYTHON_CMD="python3"

function get_country_code()
{
    if [ -f ${TOP_DIR}/.mirror ]; then
        echo "Use existing country code."
        MIRROR=$(cat ${TOP_DIR}/.mirror)
        echo "MIRROR=${MIRROR}"
    else
        echo "get_country_code ..."
        if command -v python3 &>/dev/null; then
            PYTHON_CMD=python3
        elif command -v python &>/dev/null && python --version | grep -q '^Python 3'; then
            PYTHON_CMD=python
        else
            echo "Python 3 is not installed."
            exit 1
        fi

        MIRROR=$(${PYTHON_CMD} ${TOP_DIR}/tools/get_conutry.py)
        echo "MIRROR=${MIRROR}"

        echo ${MIRROR} > ${TOP_DIR}/.mirror
    fi
}

function enable_dl_mirror()
{
    if [ x"$MIRROR" = x"1" ]; then
        echo "enable cn mirror"

        export IDF_GITHUB_ASSETS="dl.espressif.cn/github_assets"
    else
        export IDF_GITHUB_ASSETS="dl.espressif.com/github_assets"
    fi
}

function enable_gitee_mirror()
{
    if [ x"$MIRROR" = x"1" ]; then
        echo "enable mirror"
        # enable mirror
        bash ${TOP_DIR}/tools/jihu-mirror.sh set
    fi
}

function disable_gitee_mirror()
{
    if [ x"$MIRROR" = x"1" ]; then
        echo "disable mirror"
        bash ${TOP_DIR}/tools/jihu-mirror.sh unset
    fi
}

function download_esp_idf()
{
    echo "download esp-idf ..."

    enable_gitee_mirror

    git clone --recursive https://github.com/espressif/esp-idf -b $IDF_VERSION --depth=1 ${IDF_PATH}

    if [ $? -ne 0 ]; then
        disable_gitee_mirror
        return 1
    fi

    disable_gitee_mirror
    return 0
}

function update_idf_tools()
{
    echo "Installing target: $TARGET ..."
    cd ${IDF_PATH}
    enable_gitee_mirror
    git submodule update --init --recursive
    disable_gitee_mirror
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

get_country_code

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

enable_dl_mirror

update_idf_tools
if [ $? -ne 0 ]; then
    echo "update idf tools failed ..."
    exit 1
fi

echo ${TARGET} > ${TOP_DIR}/.prepare

echo "Run platform prepare success ..."
