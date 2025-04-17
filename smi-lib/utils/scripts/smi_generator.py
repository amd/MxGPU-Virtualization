#
# Copyright (C) 2022-2023 Advanced Micro Devices. All rights reserved.
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
import argparse
import tempfile
import shutil
import platform
from subprocess import run, PIPE
from ctypeslib.clang2py import main as clangToPy

HEADER = \
"""
#
# Copyright (C) 2019-2023 Advanced Micro Devices. All rights reserved.
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
"""


def parseArgument():
    parser = argparse.ArgumentParser(description="parse input arguments")
    parser.add_argument('-o','--output', type=str, required=True,
                        help='The output file name')
    parser.add_argument('-i','--input', type=str, required=True,
                        nargs='+', help='The input file name')
    parser.add_argument('-l', '--library', type=str, required=True,
                        help='Loading dynamic link libraries')
    args = vars(parser.parse_args())

    return args['output'], args['input'], args['library']


def replace_line(full_path_file_name, string_to_repalce, new_string):
    fh, abs_path = tempfile.mkstemp()
    with os.fdopen(fh, 'w') as new_file:
        new_file.write(HEADER)
        with open(full_path_file_name, 'r+') as old_file:
            for line in old_file:
                new_file.write(line.replace(string_to_repalce, new_string))

    shutil.copymode(full_path_file_name, abs_path)
    os.remove(full_path_file_name)
    shutil.move(abs_path, full_path_file_name)


def main():
    output_file, input_files, library =  parseArgument()

    library_name = os.path.basename(library)

    clang_include_dir = \
        run(["clang", "--print-resource-dir"], stdout=PIPE, stderr=PIPE, encoding="utf-8").stdout.strip()

    os_platform = platform.system()
    if os_platform == "Windows":
        clang_include_dir += "\include"
        if "Program Files(x86)" in clang_include_dir:
            clang_include_dir = clang_include_dir.replace("Program Files(x86)", "Progra~2")
        elif "Program Files" in clang_include_dir:
            clang_include_dir = clang_include_dir.replace("Program Files", "Progra~1")

        arguments = input_files + ["-o", output_file]
        line_to_replace = "_libraries['FIXME_STUB'] = FunctionFactoryStub() #  ctypes.CDLL('FIXME_STUB')"
        new_line = "_libraries['FIXME_STUB'] = ctypes.CDLL('{}')".format(library_name)
    elif os_platform == "Linux":
        clang_include_dir += "/include"
        arguments = input_files + ["-o", output_file, "-l", library]
        library_path = os.path.join(os.path.dirname(__file__), library)
        line_to_replace = "_libraries['{}'] = ctypes.CDLL('{}')".format(library_name, library_path)
        new_line = "_libraries['{}'] = ctypes.CDLL(os.path.join(os.path.dirname(__file__), '{}'))" \
            .format(library_name, library_name)
    else:
        print("Unknown operating system. It is only supporing Linux and Windows.")
        return

    arguments.append("--clang-args=-I{} {}".format(clang_include_dir, "-DWS_RECORD"))
    clangToPy(arguments)

    replace_line(output_file, line_to_replace, new_line)


if __name__ == "__main__":
    main()
