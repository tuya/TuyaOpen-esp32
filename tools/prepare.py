#!/usr/bin/env python3
# coding=utf-8

import os
import subprocess

from tools.util import (
    rm_rf, get_country_code, copy_file,
    do_subprocess, get_system_name, build_git_command_with_jihu_mirror
)


MAX_INSTALL_ATTEMPTS = 2


def need_prepare(root, prepare_file, target):
    idf_path = os.path.join(root, "esp-idf")
    idf_tools_path = os.path.join(root, ".espressif")
    if not os.path.exists(idf_path) \
            or not os.path.exists(idf_tools_path):
        print("ESP-IDF path or tools path not exists, need prepare.")
        return True
    if not os.path.exists(prepare_file):
        return True
    with open(prepare_file, "r", encoding='utf-8') as f:
        old_target = f.read().strip()
    print(f"old_target: {old_target}")
    if target != old_target:
        return True
    return False


def record_prepare(prepare_file, target):
    with open(prepare_file, "w", encoding='utf-8') as f:
        f.write(target)
    return True


def delete_temp_files(root):
    # no need delete .prepare
    delete_list = [".target", ".app"]
    for d in delete_list:
        delete_file = os.path.join(root, d)
        print(f"delete: {d}")
        rm_rf(delete_file)

    root = os.path.join(root, "tuya_open_sdk")
    delete_list = ["sdkconfig", "sdkconfig.old", "sdkconfig.defaults", "build"]
    for d in delete_list:
        delete_file = os.path.join(root, d)
        print(f"delete: {d}")
        rm_rf(delete_file)
    pass


def exists_idf_py():
    try:
        result = subprocess.run(
            ["idf.py", "--version"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=10
        )
        if result.returncode == 0:
            print(f"idf.py version: {result.stdout.strip()}")
            return True
    except Exception as e:
        print(f"Run idf.py error: {str(e)}")

    return False


def export_idf_path(root):
    idf_path = os.path.join(root, "esp-idf")
    idf_tools_path = os.path.join(root, ".espressif")
    os.environ["IDF_PATH"] = idf_path
    os.environ["IDF_TOOLS_PATH"] = idf_tools_path
    pass


def download_esp_idf():
    print("Downloading ESP_IDF ...")

    idf_path = os.environ["IDF_PATH"]
    if not os.path.exists(idf_path):
        print("Initialing esp-idf ...")
        idf_version = "v5.4"
        cmds = [
            "git",
            "clone",
            "--recursive",
            "https://github.com/espressif/esp-idf",
            "-b",
            idf_version,
            "--depth=1",
            idf_path,
        ]
        cmd = build_git_command_with_jihu_mirror(cmds)
        if do_subprocess(cmd) != 0:
            return False

    cmds = [
        "git",
        "submodule",
        "update",
        "--init",
        "--recursive",
    ]
    cmd = build_git_command_with_jihu_mirror(cmds)
    git_cmd = f"cd {idf_path} && {cmd}"
    if do_subprocess(git_cmd) != 0:
        return False

    print("Download ESP_IDF success.")
    return True


def copy_idf_tools_py(root):
    source_file = os.path.join(root, "tools", "idf_tools.py")
    idf_path = os.environ["IDF_PATH"]
    target_file = os.path.join(idf_path, "tools", "idf_tools.py")
    copy_file(source_file, target_file)
    pass


def install_target(target):
    if get_country_code() != "China":
        os.environ["IDF_GITHUB_ASSETS"] = "dl.espressif.com/github_assets"
    else:
        os.environ["IDF_GITHUB_ASSETS"] = "dl.espressif.cn/github_assets"

    idf_path = os.environ["IDF_PATH"]
    
    # Check if we're in a CI environment
    is_ci = os.getenv('CI') or os.getenv('GITHUB_ACTIONS') or os.getenv('CONTINUOUS_INTEGRATION')
    
    if is_ci and get_system_name() != "windows":
        # Use silent install script in CI to reduce log noise
        cmd = f"cd {idf_path} && "
        cmd += f"./install.sh {target} > /dev/null"
    else:
        # Use normal installation
        cmd = f"cd {idf_path} && "
        if get_system_name() == "windows":
            cmd += f".\\install.bat {target}"
        else:
            cmd += f"./install.sh {target}"

    if do_subprocess(cmd) != 0:
        return False

    print(f"Install target [{target}] success.")
    return True


def cleanup_failed_downloads(root):
    idf_tools_path = os.environ["IDF_TOOLS_PATH"]
    cleanup_list = [
        os.path.join(idf_tools_path, "dist"),
        os.path.join(idf_tools_path, "tools"),
    ]
    for path in cleanup_list:
        if os.path.exists(path):
            print(f"Remove failed download cache: {path}")
            rm_rf(path)


def install_target_with_retry(root, target):
    for idx in range(MAX_INSTALL_ATTEMPTS):
        if install_target(target):
            return True
        cleanup_failed_downloads(root)
        if idx + 1 < MAX_INSTALL_ATTEMPTS:
            print(f"Retry install target [{target}]: {idx + 2}/{MAX_INSTALL_ATTEMPTS}")
    return False


def platform_prepare(root, target):
    prepare_file = os.path.join(root, ".prepare")
    if not need_prepare(root, prepare_file, target):
        print("No need prepare.")
        return True
    print("Need prepare.")
    delete_temp_files(root)

    # if not exists_idf_py():
    export_idf_path(root)
    if not download_esp_idf():
        print("Download ESP_IDF failed.")
        return False

    copy_idf_tools_py(root)
    if not install_target_with_retry(root, target):
        print(f"Install target [{target}] failed.")
        return False

    record_prepare(prepare_file, target)
    return True
