#!/usr/bin/env python3
# coding=utf-8

import atexit
import os
import platform
import shlex
import shutil
import subprocess
import tempfile

import requests


COUNTRY_CODE = ""  # "China" or other


def set_country_code():
    global COUNTRY_CODE
    if len(COUNTRY_CODE):
        return COUNTRY_CODE

    try:
        response = requests.get('http://www.ip-api.com/json', timeout=5)
        response.raise_for_status()

        result = response.json()
        country = result.get("country", "")
        print(f"country code: {country}")

        COUNTRY_CODE = country
    except requests.exceptions.RequestException as e:
        print(f"country code error: {e}")

    return COUNTRY_CODE


def get_country_code():
    global COUNTRY_CODE
    if len(COUNTRY_CODE):
        return COUNTRY_CODE
    return set_country_code()


# "linux", "darwin_x86", "darwin_arm64", "windows"
SYSTEM_NAME = ""


def set_system_name():
    global SYSTEM_NAME
    _env = platform.system().lower()
    if "linux" in _env:
        SYSTEM_NAME = "linux"
    elif "darwin" in _env:
        machine = "x86" if "x86" in platform.machine().lower() else "arm64"
        SYSTEM_NAME = f"darwin_{machine}"
    else:
        SYSTEM_NAME = "windows"
    return SYSTEM_NAME


def get_system_name():
    global SYSTEM_NAME
    if len(SYSTEM_NAME):
        return SYSTEM_NAME
    return set_system_name()


def rm_rf(file_path):
    if os.path.isfile(file_path):
        os.remove(file_path)
    elif os.path.isdir(file_path):
        shutil.rmtree(file_path)
    return True


def copy_file(source, target, force=True) -> bool:
    '''
    force: Overwrite if the target file exists
    '''
    if not os.path.exists(source):
        print(f"Not found [{source}].")
        return False
    if not force and os.path.exists(target):
        return True

    target_dir = os.path.dirname(target)
    if target_dir:
        os.makedirs(target_dir, exist_ok=True)
    shutil.copy(source, target)
    return True


def do_subprocess(cmd: str) -> int:
    '''
    return: 0: success, other: error
    '''
    if not cmd:
        print("Subprocess cmd is empty.")
        return 0

    print(f"do subprocess: {cmd}")

    ret = 1  # 0: success
    try:
        ret = os.system(cmd)
    except Exception as e:
        print(f"Do subprocess error: {str(e)}")
        print(f"do subprocess: {cmd}")
        return 1
    return ret


def execute_idf_commands(root, cmd, directory) -> bool:
    if not os.path.exists(directory):
        print(f"Error: Directory [{directory}] does not exist.")
        return False
    idf_path = os.path.join(root, "esp-idf")
    idf_tools_path = os.path.join(root, ".espressif")
    os.environ["IDF_PATH"] = idf_path
    os.environ["IDF_TOOLS_PATH"] = idf_tools_path
    if get_system_name() == "windows":
        export_bat = os.path.join(idf_path, "export.bat")
        command = f"{export_bat} && "
    else:
        export_sh = os.path.join(idf_path, "export.sh")
        command = f". {export_sh} && "

    command += f"cd {directory} && {cmd}"
    if 0 != do_subprocess(command):
        return False
    return True


def set_target(root, chip, suffix=""):
    '''
    1. Copy sdkconfig.defaults before generate sdkconfig.h
    use: idf.py set-target xxx
    '''
    tuya_path = os.path.join(root, "tuya_open_sdk")
    sdk_config = os.path.join(tuya_path, f"sdkconfig_{chip}")
    sdk_config += suffix
    sdk_config_default = os.path.join(tuya_path, "sdkconfig.defaults")
    if not copy_file(sdk_config, sdk_config_default):
        return False
    cmd = f"idf.py set-target {chip}"
    directory = os.path.join(root, "tuya_open_sdk")
    if not execute_idf_commands(root, cmd, directory):
        return False
    sdkconfig = os.path.join(tuya_path, "sdkconfig")
    sdkconfig_old = os.path.join(tuya_path, "sdkconfig.old")
    rm_rf(sdkconfig)
    rm_rf(sdkconfig_old)
    return True


def need_settarget(target_file, target):
    if not os.path.exists(target_file):
        return True
    with open(target_file, "r", encoding='utf-8') as f:
        old_target = f.read().strip()
    print(f"old_target: {old_target}")
    if target != old_target:
        return True
    return False


def record_target(target_file, target):
    with open(target_file, "w", encoding='utf-8') as f:
        f.write(target)
    return True


MIRROR_LIST = [
    "adafruit/asf4",
    "ARMmbed/mbed-os-cypress-capsense-button",
    "ARMmbed/mbed-os-posix-socket",
    "ARMmbed/mbed-os",
    "ARMmbed/mbedtls",
    "atgreen/libffi",
    "ATmobica/mcuboot",
    "bluekitchen/btstack",
    "DaveGamble/cJSON",
    "eclipse/tinydtls",
    "espressif/asio",
    "espressif/aws-iot-device-sdk-embedded-C",
    "espressif/connectedhomeip",
    "espressif/esp-adf",
    "espressif/esp-adf-libs",
    "espressif/esp-at",
    "espressif/esp-ble-mesh-lib",
    "espressif/esp-bootloader-plus",
    "espressif/esp-box",
    "espressif/esp-coex-lib",
    "espressif/esp-cryptoauthlib",
    "espressif/esp-dev-kits",
    "espressif/esp-dl",
    "espressif/esp-hosted",
    "espressif/esp-idf",
    "espressif/esp-idf-provisioning-ios",
    "espressif/esp-ieee802154-lib",
    "espressif/esp-insights",
    "espressif/esp-iot-bridge",
    "espressif/esp-iot-solution",
    "espressif/esp-lwip",
    "espressif/esp-matter",
    "espressif/esp-mesh-lite",
    "espressif/esp-mqtt",
    "espressif/esp-nimble",
    "espressif/esp-phy-lib",
    "espressif/esp-rainmaker",
    "espressif/esp-rainmaker-cli",
    "espressif/esp-rainmaker-common",
    "espressif/esp-rainmaker-ios",
    "espressif/esp-serial-flasher",
    "espressif/esp-sr",
    "espressif/esp-thread-lib",
    "espressif/esp-who",
    "espressif/esp32-bt-lib",
    "espressif/esp32-camera",
    "espressif/esp32-wifi-lib",
    "espressif/esp32c2-bt-lib",
    "espressif/esp32c3-bt-lib",
    "espressif/esp32c5-bt-lib",
    "espressif/esp32c6-bt-lib",
    "espressif/esp32h2-bt-lib",
    "espressif/esptool",
    "espressif/json_generator",
    "espressif/json_parser",
    "espressif/mbedtls",
    "espressif/openthread",
    "espressif/tinyusb",
    "espressif/tlsf",
    "FreeRTOS/FreeRTOS-Kernel",
    "google/boringssl",
    "google/pigweed",
    "h2o/neverbleed",
    "hathach/nxp_driver",
    "hathach/tinyusb",
    "Infineon/abstraction-rtos",
    "Infineon/anycloud-ota",
    "Infineon/bluetooth-freertos",
    "Infineon/btsdk-include",
    "Infineon/btsdk-tools",
    "Infineon/btstack",
    "Infineon/clib-support",
    "Infineon/connectivity-utilities",
    "Infineon/core-lib",
    "Infineon/core-make",
    "Infineon/freertos",
    "Infineon/kv-store",
    "Infineon/mtb-hal-cat1",
    "Infineon/mtb-pdl-cat1",
    "Infineon/ot-ifx-release",
    "Infineon/OT-Matter-30739A0",
    "Infineon/OT-Matter-TARGET_CYW930739M2EVB-01",
    "Infineon/psoc6cm0p",
    "Infineon/recipe-make-cat1a",
    "Infineon/retarget-io",
    "Infineon/secure-sockets",
    "Infineon/serial-flash",
    "Infineon/TARGET_CY8CKIT-062S2-43012",
    "Infineon/whd-bsp-integration",
    "Infineon/wifi-connection-manager",
    "Infineon/wifi-host-driver",
    "Infineon/wifi-mw-core",
    "intel/tinycbor",
    "jedisct1/libhydrogen",
    "jedisct1/libsodium",
    "jeremyjh/ESP32_TFT_library",
    "kmackay/micro-ecc",
    "leethomason/tinyxml2",
    "libexpat/libexpat",
    "lvgl/lvgl",
    "lwip-tcpip/lwip",
    "micropython/axtls",
    "micropython/micropython",
    "micropython/micropython-lib",
    "micropython/mynewt-nimble",
    "micropython/stm32lib",
    "matter-mtk/genio-matter-lwip",
    "matter-mtk/genio-matter-mdnsresponder",
    "mruby/mruby",
    "nayuki/QR-Code-generator",
    "nanopb/nanopb",
    "nestlabs/nlassert",
    "nestlabs/nlfaultinjection",
    "nestlabs/nlio",
    "nestlabs/nlunit-test",
    "nghttp2/nghttp2",
    "nodejs/http-parser",
    "obgm/libcoap",
    "ocornut/imgui",
    "open-source-parsers/jsoncpp",
    "openthread/ot-br-posix",
    "openthread/ot-nxp",
    "openthread/ot-qorvo",
    "openthread/openthread",
    "openweave/cirque",
    "pellepl/spiffs",
    "pfalcon/berkeley-db-1.xx",
    "project-chip/zap",
    "protobuf-c/protobuf-c",
    "pybind/pybind11",
    "Qorvo/qpg-openthread",
    "Qorvo/QMatter",
    "raspberrypi/pico-sdk",
    "tatsuhiro-t/neverbleed",
    "ThrowTheSwitch/CMock",
    "throwtheswitch/cexception",
    "throwtheswitch/unity",
    "ThrowTheSwitch/Unity",
    "troglobit/editline",
    "warmcat/libwebsockets",
    "zserge/jsmn",
]


_JIHU_MIRROR_READY = False


def _git_version_tuple():
    try:
        out = subprocess.check_output(["git", "--version"], text=True).strip()
    except Exception:
        return (0, 0, 0)
    # Output like: "git version 2.34.1" / "git version 2.39.2.windows.1"
    parts = out.split()
    if len(parts) < 3:
        return (0, 0, 0)
    nums = parts[2].split(".")
    try:
        return tuple(int(x) for x in nums[:3] if x.isdigit())
    except Exception:
        return (0, 0, 0)


def _install_jihu_via_config_global():
    # Path A (git >= 2.32): isolated temp gitconfig via GIT_CONFIG_GLOBAL.
    # No race with other git processes; a leaked temp file (on hard kill)
    # is harmless.
    fd, path = tempfile.mkstemp(suffix=".gitconfig", prefix="esp_jihu_")
    os.close(fd)
    for repo in MIRROR_LIST:
        github = f"https://github.com/{repo}"
        jihu = f"https://jihulab.com/esp-mirror/{repo}"
        for key in (f"url.{jihu}.insteadOf", f"url.{jihu}.git.insteadOf"):
            subprocess.run(["git", "config", "--file", path, key, github],
                           check=True)
    os.environ["GIT_CONFIG_GLOBAL"] = path

    def _cleanup():
        try:
            os.unlink(path)
        except OSError:
            pass
    atexit.register(_cleanup)


def _install_jihu_via_global_fallback():
    # Path B (git < 2.32): write into the user's real --global gitconfig.
    # Trade-off on old git: rules are visible to other concurrent git
    # processes during the window, and a hard kill will leave them behind.
    # atexit handles normal exits.
    print("[WARN] git < 2.32 detected; falling back to --global gitconfig. "
          "If this process is killed, run "
          "`bash platform/ESP32/tools/jihu-mirror.sh unset` to clean up.")
    for repo in MIRROR_LIST:
        github = f"https://github.com/{repo}"
        jihu = f"https://jihulab.com/esp-mirror/{repo}"
        for key in (f"url.{jihu}.insteadOf", f"url.{jihu}.git.insteadOf"):
            subprocess.run(["git", "config", "--global", key, github],
                           check=True)

    def _unset():
        for repo in MIRROR_LIST:
            jihu = f"https://jihulab.com/esp-mirror/{repo}"
            for key in (f"url.{jihu}.insteadOf", f"url.{jihu}.git.insteadOf"):
                subprocess.run(["git", "config", "--global", "--unset", key],
                               check=False)
    atexit.register(_unset)


def _ensure_jihu_mirror():
    global _JIHU_MIRROR_READY
    if _JIHU_MIRROR_READY:
        return
    if _git_version_tuple() >= (2, 32, 0):
        _install_jihu_via_config_global()
    else:
        _install_jihu_via_global_fallback()
    _JIHU_MIRROR_READY = True


def _join_cmd(cmds) -> str:
    # shlex.quote uses POSIX rules: on Windows CMD it produces single-quoted
    # paths that CMD treats as literal characters (e.g. 'D:\\foo' becomes part
    # of the argument). Use subprocess.list2cmdline on Windows for proper
    # CMD-style quoting; fall back to shlex.quote elsewhere.
    str_cmds = [str(c) for c in cmds]
    if get_system_name() == "windows":
        return subprocess.list2cmdline(str_cmds)
    return " ".join(shlex.quote(c) for c in str_cmds)


def build_git_command_with_jihu_mirror(cmds) -> str:
    if not cmds or cmds[0] != "git" or get_country_code() != "China":
        return _join_cmd(cmds)

    print("Use jihulab mirror for current git command ...")
    _ensure_jihu_mirror()
    return _join_cmd(cmds)
