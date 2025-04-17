#
# Copyright (C) 2023-2024 Advanced Micro Devices. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import os
import subprocess

# Specify the AStyle options
ASTYLE_OPTIONS = ["--style=kr", "--suffix=none", "--indent=force-tab=4", "--max-code-length=100"]

# List of filenames to exclude from formatting
EXCLUDE_H_FILES = {"amdsmi.h", "json.h"}

# Folder paths for .cpp and .h files
CPP_FOLDER = "src"
HEADER_FOLDER = "include"

def format_files(folder, extensions):
    for root, _, files in os.walk(folder):
        for filename in files:
            if filename.endswith(extensions):
                full_path = os.path.join(root, filename)
                if filename not in EXCLUDE_H_FILES:
                    subprocess.run(["astyle.exe"] + ASTYLE_OPTIONS + [full_path])

format_files(CPP_FOLDER, ".cpp")
format_files(HEADER_FOLDER, ".h")
