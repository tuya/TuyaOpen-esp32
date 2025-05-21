#!/usr/bin/env python3
# coding=utf-8

import sys
import os

from tools.prepare import platform_prepare


def main():
    target = "esp32s3" if len(sys.argv) < 2 else sys.argv[1]
    print(f"target: {target}")
    root = os.path.dirname(os.path.abspath(__file__))
    if not platform_prepare(root, target):
        sys.exit(1)
    sys.exit(0)


if __name__ == "__main__":
    main()
