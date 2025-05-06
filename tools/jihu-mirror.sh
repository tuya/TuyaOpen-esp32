#!/bin/bash
#
# Redirects github repos to jihu mirrors.
#
#

set -e
set -u

## repos group
REPOS_ARRAY=(
    adafruit/asf4
    ARMmbed/mbed-os-cypress-capsense-button
    ARMmbed/mbed-os-posix-socket
    ARMmbed/mbed-os
    ARMmbed/mbedtls
    atgreen/libffi
    ATmobica/mcuboot
    bluekitchen/btstack
    DaveGamble/cJSON
    eclipse/tinydtls
    espressif/asio
    espressif/aws-iot-device-sdk-embedded-C
    espressif/connectedhomeip
    espressif/esp-adf
    espressif/esp-adf-libs
    espressif/esp-at
    espressif/esp-ble-mesh-lib
    espressif/esp-bootloader-plus
    espressif/esp-box
    espressif/esp-coex-lib
    espressif/esp-cryptoauthlib
    espressif/esp-dev-kits
    espressif/esp-dl
    espressif/esp-hosted
    espressif/esp-idf
    espressif/esp-idf-provisioning-ios
    espressif/esp-ieee802154-lib
    espressif/esp-insights
    espressif/esp-iot-bridge
    espressif/esp-iot-solution
    espressif/esp-lwip
    espressif/esp-matter
    espressif/esp-mesh-lite
    espressif/esp-mqtt
    espressif/esp-nimble
    espressif/esp-phy-lib
    espressif/esp-rainmaker
    espressif/esp-rainmaker-cli
    espressif/esp-rainmaker-common
    espressif/esp-rainmaker-ios
    espressif/esp-serial-flasher
    espressif/esp-sr
    espressif/esp-thread-lib
    espressif/esp-who
    espressif/esp32-bt-lib
    espressif/esp32-camera
    espressif/esp32-wifi-lib
    espressif/esp32c2-bt-lib
    espressif/esp32c3-bt-lib
    espressif/esp32c5-bt-lib
    espressif/esp32c6-bt-lib
    espressif/esp32h2-bt-lib
    espressif/esptool
    espressif/json_generator
    espressif/json_parser
    espressif/mbedtls
    espressif/openthread
    espressif/tinyusb
    espressif/tlsf
    FreeRTOS/FreeRTOS-Kernel
    google/boringssl
    google/pigweed
    h2o/neverbleed
    hathach/nxp_driver
    hathach/tinyusb
    Infineon/abstraction-rtos
    Infineon/anycloud-ota
    Infineon/bluetooth-freertos
    Infineon/btsdk-include
    Infineon/btsdk-tools
    Infineon/btstack
    Infineon/clib-support
    Infineon/connectivity-utilities
    Infineon/core-lib
    Infineon/core-make
    Infineon/freertos
    Infineon/kv-store
    Infineon/mtb-hal-cat1
    Infineon/mtb-pdl-cat1
    Infineon/ot-ifx-release
    Infineon/OT-Matter-30739A0
    Infineon/OT-Matter-TARGET_CYW930739M2EVB-01
    Infineon/psoc6cm0p
    Infineon/recipe-make-cat1a
    Infineon/retarget-io
    Infineon/secure-sockets
    Infineon/serial-flash
    Infineon/TARGET_CY8CKIT-062S2-43012
    Infineon/whd-bsp-integration
    Infineon/wifi-connection-manager
    Infineon/wifi-host-driver
    Infineon/wifi-mw-core
    intel/tinycbor
    jedisct1/libhydrogen
    jedisct1/libsodium
    jeremyjh/ESP32_TFT_library
    kmackay/micro-ecc
    leethomason/tinyxml2
    libexpat/libexpat
    lvgl/lvgl
    lwip-tcpip/lwip
    micropython/axtls
    micropython/micropython
    micropython/micropython-lib
    micropython/mynewt-nimble
    micropython/stm32lib
    matter-mtk/genio-matter-lwip
    matter-mtk/genio-matter-mdnsresponder
    mruby/mruby
    nayuki/QR-Code-generator
    nanopb/nanopb
    nestlabs/nlassert
    nestlabs/nlfaultinjection
    nestlabs/nlio
    nestlabs/nlunit-test
    nghttp2/nghttp2
    nodejs/http-parser
    obgm/libcoap
    ocornut/imgui
    open-source-parsers/jsoncpp
    openthread/ot-br-posix
    openthread/ot-nxp
    openthread/ot-qorvo
    openthread/openthread
    openweave/cirque
    pellepl/spiffs
    pfalcon/berkeley-db-1.xx
    project-chip/zap
    protobuf-c/protobuf-c
    pybind/pybind11
    Qorvo/qpg-openthread
    Qorvo/QMatter
    raspberrypi/pico-sdk
    tatsuhiro-t/neverbleed
    ThrowTheSwitch/CMock
    throwtheswitch/cexception
    throwtheswitch/unity
    ThrowTheSwitch/Unity
    troglobit/editline
    warmcat/libwebsockets
    zserge/jsmn
)

len=${#REPOS_ARRAY[@]}

if [[ "$@" == "" ]]; then
    echo "Usage:"
    echo "    Set the mirror:   ./jihu-mirror.sh set"
    echo "    Unset the mirror: ./jihu-mirror.sh unset"
fi

for ((i = 0; i < len; i++))
do
    REPO=${REPOS_ARRAY[i]}
    if [[ "$@" == "set" ]]; then
        git config --global url.https://jihulab.com/esp-mirror/$REPO.insteadOf https://github.com/$REPO
        git config --global url.https://jihulab.com/esp-mirror/$REPO.git.insteadOf https://github.com/$REPO
    elif [[ "$@" == "unset" ]]; then
        git config --global --unset url.https://jihulab.com/esp-mirror/$REPO.insteadof
        git config --global --unset url.https://jihulab.com/esp-mirror/$REPO.git.insteadof
    fi
done