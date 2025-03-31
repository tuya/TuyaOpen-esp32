#!/usr/bin/env bash

set -e


PROJ_NAME=$1
TARGET=$2
FRAMEWORK=$3
CHIP=$4

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

exit 0
