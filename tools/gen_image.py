#!/usr/bin/env python3
# -*- coding: utf-8 -*-
##
# @file gen_image.py
# @brief 将esp-idf生成的多个bin文件合并成独立文件方便tyutool使用
#        根据flasher_args.json可以获取所有bin文件和对应的地址
#        使用esptool.py的——merge_bin命令完成合并
#        参数1： 芯片类型（esp32|esp32c3|esp32s3|...）
#        参数2： build路径，在此路径中查找flasher_args.json，并解析
#        参数3： 输出bin文件，输出的bin文件从0地址烧录
# @author tuya
# @version 1.0.0
# @date 2025-04-23


import json
import subprocess
import sys
import os


def merge_exec(chip, build_path, out_bin):
    args_file = os.path.join(build_path, "flasher_args.json")
    if not os.path.exists(args_file):
        print(f"Error: Not found [{args_file}]")
        sys.exit(1)

    with open(args_file, 'r') as f:
        args_data = json.load(f)

    flash_files = args_data['flash_files']

    merge_cmd = [
        'esptool.py',
        '--chip', chip,
        'merge_bin',
        '-o', out_bin
    ]
    for addr in flash_files:
        bin_path = flash_files[addr]
        merge_cmd += [addr, os.path.join(build_path, bin_path)]

    print("Merging bin ...")
    # print(merge_cmd)
    subprocess.run(merge_cmd, check=True)
    pass


def main():
    NOTE = '''
    Please use like:
    python3 gen_image.py esp32s3 ./build output.bin
    '''
    if len(sys.argv) < 4:
        print(NOTE)
        sys.exit(1)
    chip = sys.argv[1]
    build_path = sys.argv[2]
    out_bin = sys.argv[3]
    merge_exec(chip, build_path, out_bin)
    pass


if __name__ == '__main__':
    main()
