# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT



import os
import argparse
import glob
import tempfile
import shutil
import platform
from subprocess import run, PIPE
from ctypeslib.clang2py import main as clangToPy

HEADER = """# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


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
    parser.add_argument('-c', '--clang', type=str, required=False,
                        default='clang', help='Path to clang executable')
    args = vars(parser.parse_args())

    return args['output'], args['input'], args['library'], args['clang']


def _discover_windows_dk_includes():
    # libclang needs <time.h> (ucrt) and <vcruntime.h> (MSVC) to parse
    # amdsmi.h on Windows. Find them under $DK_ROOT and return their parent
    # directories. Returns [] if DK_ROOT is unset or nothing is found.
    dk_root = os.environ.get("DK_ROOT")
    if not dk_root:
        return []
    dk_root = dk_root.rstrip("/\\")

    def _pick_newest(pattern):
        matches = glob.glob(os.path.join(dk_root, *pattern))
        return max(matches, key=os.path.getmtime) if matches else None

    extras = []
    vcruntime = _pick_newest(("vc", "*", "include", "vcruntime.h"))
    if vcruntime:
        extras.append(os.path.dirname(vcruntime))
    ucrt_time = _pick_newest(("ms_wdk", "*", "Include", "10.*", "ucrt", "time.h"))
    if ucrt_time:
        extras.append(os.path.dirname(ucrt_time))
    return [os.path.normpath(p) for p in extras]


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
    output_file, input_files, library, clang_path =  parseArgument()

    library_name = os.path.basename(library)

    clang_include_dir = \
        run([clang_path, "--print-resource-dir"], stdout=PIPE, stderr=PIPE, encoding="utf-8").stdout.strip()

    os_platform = platform.system()
    extra_includes = []
    if os_platform == "Windows":
        clang_include_dir += "\\include"
        if "Program Files(x86)" in clang_include_dir:
            clang_include_dir = clang_include_dir.replace("Program Files(x86)", "Progra~2")
        elif "Program Files" in clang_include_dir:
            clang_include_dir = clang_include_dir.replace("Program Files", "Progra~1")
        extra_includes = _discover_windows_dk_includes()

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

    include_args = " ".join("-I{}".format(p) for p in [clang_include_dir, *extra_includes])
    arguments.append("--clang-args={} {}".format(include_args, "-DWS_RECORD"))
    clangToPy(arguments)

    replace_line(output_file, line_to_replace, new_line)


if __name__ == "__main__":
    main()
