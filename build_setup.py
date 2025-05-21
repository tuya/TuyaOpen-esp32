#!/usr/bin/env python3
# coding=utf-8

import sys
import os

from tools.util import record_target, set_target
from tools.prepare import platform_prepare, delete_temp_files


SUPPORT_CHIPS = [
    "esp32",
    "esp32c3",
    "esp32s3",
]


def need_setup(root) -> bool:
    '''
    1. sdkconfig.h file exists: no need to be regenerated
    2. Delete the build directory before generation sdkconfig.h
    '''
    build_path = os.path.join(root, "tuya_open_sdk", "build")
    sdkconfig = os.path.join(build_path,
                             "config", "sdkconfig.h")
    if os.path.isfile(sdkconfig):
        return False

    return True


def setup_some_files(root, chip) -> bool:
    delete_temp_files(root)


def main():
    '''
    1. 主要作用是生成sdkconfig.h，因为tuyaopen src编译需要
    2. 生成sdkconfig.h的命令: idf.py set-target xxx
    3. 有可能platform已经下载但是没有prpare，所以prpare一次
    4. 这里的操作没有cmake参数，所以在build_example时需要重新解析cmake
    5. 重新解析cmake的命令: idf.py set-target xxx
    '''
    if len(sys.argv) < 5:
        print(f"Error: At least 4 parameters are needed {sys.argv}.")
    project_name = sys.argv[1]
    platform = sys.argv[2]
    framework = sys.argv[3]
    chip = sys.argv[4]
    print(f'''project_name: ${project_name}
platform: {platform}
framework: {framework}
chip: {chip}''')

    if chip not in SUPPORT_CHIPS:
        print(f"Error: {chip} is not supported.")
        sys.exit(1)

    root = os.path.dirname(os.path.abspath(__file__))

    # prepare first
    if not platform_prepare(root, chip):
        sys.exit(1)

    if not need_setup(root):
        print("No need setup.")
        sys.exit(0)

    delete_temp_files(root)

    if not set_target(root, chip):
        print("Error: set-target failed.")
        sys.exit(1)

    # When build_example.py need set-target again
    target_file = os.path.join(root, ".target")
    record_target(target_file, "need-set-target")

    sys.exit(0)


if __name__ == "__main__":
    main()
