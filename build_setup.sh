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

${TOP_DIR}/platform_prepare.sh $TARGET
if [ $? -ne 0 ]; then
    echo "platform_prepare.sh failed."
    exit 1
fi

exit 0
