#!/usr/bin/env python3
# coding=utf-8
# 参数说明：
# $1 - params path: $1/build_param.[cmake/config/json]
# $2 - user cmd: build/clean/...

import os
import sys
import json

from tools.util import (
    copy_file, need_settarget,
    record_target, set_target,
    execute_idf_commands
)
from tools.prepare import delete_temp_files


def clean(root):
    delete_temp_files(root)
    pass


def parser_para_file(json_file):
    if not os.path.isfile(json_file):
        print(f"Error: Not found [{json_file}].")
        return {}
    try:
        f = open(json_file, 'r', encoding='utf-8')
        json_data = json.load(f)
        f.close()
    except Exception as e:
        print(f"Parser json error:  [{str(e)}].")
        return {}
    return json_data


def set_environment(build_param_path, param_data):
    os.environ["BUILD_PARAM_DIR"] = build_param_path
    os.environ["TUYAOS_HEADER_DIR"] = param_data["OPEN_HEADER_DIR"]
    os.environ["TUYAOS_LIBS_DIR"] = param_data["OPEN_LIBS_DIR"]
    os.environ["TUYAOS_LIBS"] = param_data["PLATFORM_NEED_LIBS"]
    open_root = param_data["OPEN_ROOT"]
    board_path = os.path.join(open_root, "boards", "ESP32")
    os.environ["TUYAOS_BOARD_PATH"] = board_path
    pass


def set_partitions(root, param_data):
    if param_data.get("CONFIG_PLATFORM_FLASHSIZE_16M", False):
        flash = "16M"
    elif param_data.get("CONFIG_PLATFORM_FLASHSIZE_8M", False):
        flash = "8M"
    else:
        flash = "4M"

    print(f"Set flash size {flash}")
    tuya_path = os.path.join(root, "tuya_open_sdk")
    source = os.path.join(tuya_path, f"partitions_{flash}.csv")
    target = os.path.join(tuya_path, "partitions.csv")
    copy_file(source, target)
    pass


def merge_bin(root, chip, build_path, out_bin):
    args_file = os.path.join(build_path, "flasher_args.json")
    if not os.path.exists(args_file):
        print(f"Error: Not found [{args_file}]")
        return False

    with open(args_file, 'r') as f:
        args_data = json.load(f)

    flash_files = args_data['flash_files']

    cmd = f"python -m esptool --chip {chip} merge_bin -o {out_bin}"
    for addr in flash_files:
        bin_path = flash_files[addr]
        cmd += f" {addr} {os.path.join(build_path, bin_path)}"

    print("Merging bin ...")
    if not execute_idf_commands(root, cmd, build_path):
        print("Error: Build failed.")
        return False
    return True


def copy_assets(build_path, output_path, app_name, app_ver):
    os.makedirs(output_path, exist_ok=True)
    copy_file(os.path.join(build_path, f"{app_name}_QIO_{app_ver}.bin"),
              os.path.join(output_path, f"{app_name}_QIO_{app_ver}.bin"))
    copy_file(os.path.join(build_path, "bootloader", "bootloader.bin"),
              os.path.join(output_path, "bootloader.bin"))
    copy_file(os.path.join(build_path, "partition_table",
                           "partition-table.bin"),
              os.path.join(output_path, "partition-table.bin"))
    copy_file(os.path.join(build_path, "ota_data_initial.bin"),
              os.path.join(output_path, "ota_data_initial.bin"))
    copy_file(os.path.join(build_path, "tuya_open_sdk.bin"),
              os.path.join(output_path, f"{app_name}.bin"))
    copy_file(os.path.join(build_path, "tuya_open_sdk.bin"),
              os.path.join(output_path, f"{app_name}_UA_{app_ver}.bin"))
    copy_file(os.path.join(build_path, "tuya_open_sdk.bin"),
              os.path.join(output_path, f"{app_name}_UG_{app_ver}.bin"))
    copy_file(os.path.join(build_path, "tuya_open_sdk.elf"),
              os.path.join(output_path, f"{app_name}_{app_ver}.elf"))
    copy_file(os.path.join(build_path, "tuya_open_sdk.map"),
              os.path.join(output_path, f"{app_name}_{app_ver}.map"))
    srmodels_bin = os.path.join(build_path, "srmodels", "srmodels.bin")
    if os.path.isfile(srmodels_bin):
        copy_file(srmodels_bin,
                  os.path.join(output_path, "srmodels.bin"))

    return True


def main():
    '''
    1. 提前配置一些环境变量
    1. 如果编译的项目变化，则清理现场并set-target
    1. 如果target变化，则清理现场并set-target
    1. 配置正确的partitions.csv
    1. 调用idf.py build生成固件
    1. 打包生成单独固件
    1. 拷贝产物到输出路径中
    '''
    if len(sys.argv) < 2:
        print(f"Error: At least 2 parameters are needed {sys.argv}.")
    build_param_path = sys.argv[1]
    user_cmd = sys.argv[2]
    root = os.path.dirname(os.path.abspath(__file__))
    if "clean" == user_cmd:
        clean(root)
        sys.exit(0)

    build_param_file = os.path.join(build_param_path, "build_param.json")
    param_data = parser_para_file(build_param_file)
    if not len(param_data):
        sys.exit(1)

    # Set environment variables
    set_environment(build_param_path, param_data)

    # check app / check target
    app_file = os.path.join(root, ".app")
    target_file = os.path.join(root, ".target")
    app_name = param_data["CONFIG_PROJECT_NAME"]
    chip = param_data["PLATFORM_CHIP"]
    if need_settarget(app_file, app_name) or need_settarget(target_file, chip):
        clean(root)
        if not set_target(root, chip):
            print("Error: set-target failed.")
            sys.exit(1)
    record_target(app_file, app_name)
    record_target(target_file, chip)

    set_partitions(root, param_data)

    cmd = "idf.py build"
    directory = os.path.join(root, "tuya_open_sdk")
    if not execute_idf_commands(root, cmd, directory):
        print("Error: Build failed.")
        sys.exit(1)

    app_version = param_data["CONFIG_PROJECT_VERSION"]
    idf_build_path = os.path.join(root, "tuya_open_sdk", "build")
    out_bin = os.path.join(idf_build_path, f"{app_name}_QIO_{app_version}.bin")
    print(f"Start generate {app_name}_QIO_{app_version}.bin")
    if not merge_bin(root, chip, idf_build_path, out_bin):
        print("Error: Merge bin.")
        sys.exit(1)

    output_path = param_data["BIN_OUTPUT_DIR"]
    if not copy_assets(idf_build_path, output_path, app_name, app_version):
        print("Error: copy assets.")
        sys.exit(1)

    sys.exit(0)


if __name__ == "__main__":
    main()
