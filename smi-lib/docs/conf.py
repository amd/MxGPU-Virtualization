
#
# Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import re
import sys
from pathlib import Path

sys.path.append(str(Path('_extension').resolve()))

def get_version_info(filepath):
    version_major = None
    version_minor = None
    version_release = None

    with open(filepath, "r") as f:
        for line in f:
            line = line.strip()
            if line.startswith("major="):
                version_major = line.split("=", 1)[1]
            elif line.startswith("minor="):
                version_minor = line.split("=", 1)[1]
            elif line.startswith("release="):
                version_release = line.split("=", 1)[1]

    if version_major is not None and version_minor is not None and version_release is not None:
        return int(version_major), int(version_minor), int(version_release)
    else:
        raise ValueError("Couldn't find all VERSION numbers.")

version_major, version_minor, version_release = get_version_info("../VERSION")

version_number = "{}.{}.{}".format(version_major, version_minor, version_release)

source_suffix = {
    '.md': 'markdown'
}

project = 'AMD SMI'
author = ""
copyright = '2025 Advanced Micro Devices, Inc. All rights reserved'
version = version_number
release = version_number

extensions = [
    'sphinx.ext.autodoc',
    'sphinx.ext.viewcode',
    'myst_parser',
    'sphinx_design'
]

myst_enable_extensions = ["colon_fence"]

templates_path = ['_templates']
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']

html_theme = 'sphinx_book_theme'
html_theme_options = {
    "use_download_button": False,
    "show_toc_level": 2
}

html_title = "AMD SMI {} documentation".format(version_number)
external_toc_path = "./sphinx/_toc.yml"
suppress_warnings = ["etoc.toctree", "toc.not_included", "myst.xref_missing"]

doxygen_root = "doxygen"
doxysphinx_enabled = True
doxygen_project = {
    "name": "AMD SMI C API reference",
    "path": "../build/amdsmi/docs/doxygen/xml",
}
