# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


import os
import subprocess

# Specify the AStyle options
ASTYLE_OPTIONS = ["--style=kr", "--suffix=none", "--indent=force-tab=4", "--max-code-length=100"]

# List of filenames to exclude from formatting
EXCLUDE_H_FILES = {"amdsmi.h", "json.h"}

# Folder paths for .cpp and .h files
CPP_FOLDER = "../../src"
HEADER_FOLDER = "../../inc"

def format_files(folder, extensions):
    for root, _, files in os.walk(folder):
        for filename in files:
            if filename.endswith(extensions):
                full_path = os.path.join(root, filename)
                if filename not in EXCLUDE_H_FILES:
                    subprocess.run(["astyle.exe"] + ASTYLE_OPTIONS + [full_path])

format_files(CPP_FOLDER, ".cpp")
format_files(HEADER_FOLDER, ".h")
