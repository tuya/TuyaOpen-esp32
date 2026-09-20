#!/usr/bin/env python3
# coding=utf-8

import os
import subprocess

from tools.util import (
    rm_rf, get_country_code, copy_file,
    do_subprocess, get_system_name, build_git_command_with_jihu_mirror,
    build_git_command, do_subprocess_without_jihu_mirror,
    get_idf_paths, get_idf_revision, export_idf_environment,
)


MAX_INSTALL_ATTEMPTS = 2


def idf_revision_matches(idf_path, revision):
    try:
        if is_pinned_revision(revision):
            command = ["git", "-C", idf_path, "rev-parse", "HEAD"]
        else:
            # v5.4 is deliberately selected by its immutable tag.  Checking
            # it catches a stale cache whose esp-idf-v5.4 directory was
            # manually moved to another revision.
            command = ["git", "-C", idf_path, "describe", "--tags",
                       "--exact-match", "HEAD"]
        result = subprocess.run(
            command,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            timeout=10,
            check=False,
        )
    except OSError:
        return False
    return result.returncode == 0 and result.stdout.strip() == revision


def need_prepare(root, prepare_file, target):
    version, idf_path, idf_tools_path = get_idf_paths(root, target)
    if not os.path.exists(idf_path) \
            or not os.path.exists(idf_tools_path):
        print(f"ESP-IDF {version} path or tools path not exists, need prepare.")
        return True
    if not os.path.exists(prepare_file):
        return True
    revision = get_idf_revision(target)
    if not idf_revision_matches(idf_path, revision):
        print(f"ESP-IDF revision is not {revision}, need prepare.")
        return True
    with open(prepare_file, "r", encoding='utf-8') as f:
        old_prepare = f.read().strip()
    prepare_key = f"{target}:{version}:{revision}"
    print(f"old_prepare: {old_prepare}")
    if prepare_key != old_prepare:
        return True
    return False


def record_prepare(prepare_file, target):
    version = get_idf_paths(os.path.dirname(prepare_file), target)[0]
    with open(prepare_file, "w", encoding='utf-8') as f:
        f.write(f"{target}:{version}:{get_idf_revision(target)}")
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


def is_pinned_revision(revision):
    return len(revision) == 40 and all(c in "0123456789abcdef" for c in revision)


def run_idf_git_command(idf_path, cmds, use_official_git=False):
    if use_official_git:
        cmd = build_git_command(cmds)
        return do_subprocess_without_jihu_mirror(f"cd {idf_path} && {cmd}")

    cmd = build_git_command_with_jihu_mirror(cmds)
    return do_subprocess(f"cd {idf_path} && {cmd}")


def checkout_pinned_revision(idf_path, revision):
    cmds = ["git", "fetch", "--depth=1", "origin", revision]
    use_official_git = False
    if run_idf_git_command(idf_path, cmds) != 0:
        if get_country_code() != "China":
            return False
        print("Jihulab mirror does not contain the pinned ESP-IDF revision; "
              "retry with the official remote.")
        use_official_git = True
        if run_idf_git_command(idf_path, cmds, use_official_git) != 0:
            return False, use_official_git

    cmds = ["git", "checkout", "--detach", "FETCH_HEAD"]
    return run_idf_git_command(idf_path, cmds, use_official_git) == 0, use_official_git


def download_esp_idf(revision):
    print("Downloading ESP_IDF ...")

    idf_path = os.environ["IDF_PATH"]
    if not os.path.exists(idf_path):
        print("Initialing esp-idf ...")
        if is_pinned_revision(revision):
            # A SHA is not a remote branch name. Create a minimal repository,
            # fetch that exact reachable object, then check it out.
            cmds = ["git", "init", idf_path]
            cmd = build_git_command_with_jihu_mirror(cmds)
            if do_subprocess(cmd) != 0:
                return False

            cmds = [
                "git", "-C", idf_path, "remote", "add", "origin",
                "https://github.com/espressif/esp-idf",
            ]
            cmd = build_git_command_with_jihu_mirror(cmds)
            if do_subprocess(cmd) != 0:
                return False
        else:
            cmds = [
                "git",
                "clone",
                "https://github.com/espressif/esp-idf",
                "-b",
                revision,
                "--depth=1",
                idf_path,
            ]
            cmd = build_git_command_with_jihu_mirror(cmds)
            if do_subprocess(cmd) != 0:
                return False

    use_official_git = False
    if is_pinned_revision(revision):
        checkout_result = checkout_pinned_revision(idf_path, revision)
        if not checkout_result:
            return False
        checkout_ok, use_official_git = checkout_result
        if not checkout_ok:
            return False

    cmds = [
        "git",
        "submodule",
        "update",
        "--init",
        "--recursive",
        "--depth=1",
    ]
    if run_idf_git_command(idf_path, cmds, use_official_git) != 0:
        return False

    print("Download ESP_IDF success.")
    return True


def copy_idf_tools_py(root, target):
    # The platform copy carries ESP-IDF master compatibility required by S31.
    # v5.4 must keep its own idf_tools.py: replacing it changes the old IDF's
    # tool manifest and makes it resolve the wrong toolchain versions.
    if not is_pinned_revision(get_idf_revision(target)):
        return True
    source_file = os.path.join(root, "tools", "idf_tools.py")
    idf_path = os.environ["IDF_PATH"]
    target_file = os.path.join(idf_path, "tools", "idf_tools.py")
    return copy_file(source_file, target_file)


def target_supported_by_idf(target):
    idf_path = os.environ["IDF_PATH"]
    target_path = os.path.join(idf_path, "components", "soc", target)
    if os.path.isdir(target_path):
        return True

    print(f"ESP-IDF [{idf_path}] does not support target [{target}].")
    return False


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
    if not target_supported_by_idf(target):
        return False

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

    export_idf_environment(root, target)
    if not download_esp_idf(get_idf_revision(target)):
        print("Download ESP_IDF failed.")
        return False

    if not copy_idf_tools_py(root, target):
        print("Copy idf_tools.py failed.")
        return False
    if not install_target_with_retry(root, target):
        print(f"Install target [{target}] failed.")
        return False

    # Do not discard the current project's generated configuration until the
    # required IDF and its target tools have been prepared successfully.
    delete_temp_files(root)
    record_prepare(prepare_file, target)
    return True
