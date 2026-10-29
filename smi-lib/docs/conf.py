# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


import os
import re
import sys
from pathlib import Path

from sphinx.errors import ConfigError

DOCS_DIR = Path(__file__).parent.resolve()
DOXYGEN_DIR = DOCS_DIR / "doxygen"
AMDSMI_DIR = DOCS_DIR.parent
AMDSMI_H = AMDSMI_DIR / "interface" / "amdsmi.h"

html_baseurl = os.environ.get("READTHEDOCS_CANONICAL_URL", "instinct.docs.amd.com")
html_context = {}
if os.environ.get("READTHEDOCS", "") == "True":
    html_context["READTHEDOCS"] = True

sys.path.append(str(DOCS_DIR / "extension"))


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

    if (
        version_major is not None
        and version_minor is not None
        and version_release is not None
    ):
        return int(version_major), int(version_minor), int(version_release)
    else:
        raise ValueError("Couldn't find all VERSION numbers.")


version_major, version_minor, version_release = get_version_info("../VERSION")

version_number = "{}.{}.{}".format(version_major, version_minor, version_release)

# project info
project = "AMD SMI (SR-IOV Linux host)"
author = "Advanced Micro Devices, Inc."
copyright = "Copyright %Y Advanced Micro Devices, Inc. All rights reserved."
version = version_number
release = version_number

html_theme = "rocm_docs_theme"
html_theme_options = {
    "flavor": "instinct",
    "repository_provider": "github",
    "repository_url": "https://github.com/amd/MxGPU-Virtualization",
    "link_main_doc": True,
    "announcement": "This page documents AMD-SMI for virtualization hosts only. Please see the standard <a href='https://rocm.docs.amd.com/projects/amdsmi/en/latest/'>AMD-SMI</a> site for all other uses.",
    "nav_secondary_items": {
        "Community": "https://github.com/ROCm/ROCm/discussions",
        "Blogs": "https://rocm.blogs.amd.com/",
        "ROCm&#8482 Docs": "https://rocm.docs.amd.com",
        "ROCm Developer Hub": "https://www.amd.com/en/developer/resources/rocm-hub.html",
    },
    "show_toc_level": 4,
    "use_repository_button": True,
    "use_issues_button": True,
    "use_download_button": True,
}
html_title = "AMD SMI {}<br>(SR-IOV Linux host)".format(version_number)
suppress_warnings = ["etoc.toctree"]
external_toc_path = "./sphinx/_toc.yml"

external_projects_current_project = "amdsmi"
extensions = ["rocm_docs", "rocm_docs.doxygen", "amdsmi_docs.doxygen"]

exclude_patterns = [
    "dev/**",
]

# Generate llms.txt and llms-full.txt.
rocm_docs_generate_llms = True

# Doxygen-related settings
doxygen_root = DOCS_DIR / "doxygen"
breathe_projects = {"amdsmi-virt": doxygen_root / "_out" / "xml"}
breathe_default_project = "amdsmi-virt"
breathe_domain_by_extension = {"h": "c"}
amdsmi_doxygen_tagfile = doxygen_root / "_out" / "tagfile.xml"
doxysphinx_enabled = False


# Make Doxyfile consistent with this Sphinx config
def generate_doxyfile(_app, _config):
    doxyfile_in = doxygen_root / "Doxyfile.in"
    doxyfile_out = doxygen_root / "Doxyfile"

    if not doxyfile_in.exists():
        raise ConfigError(f"Missing Doxyfile.in at {doxyfile_in}")

    replacements = {
        "@PROJECT_NUMBER@": version,
        "@INPUT@": str(AMDSMI_H),
        "@OUTPUT_DIRECTORY@": str(doxygen_root / "_out"),
        "@GENERATE_TAGFILE@": str(amdsmi_doxygen_tagfile),
    }

    def _replace(m):
        key = m.group(0)
        if key not in replacements:
            raise ConfigError(f"Unknown template variable {key} in Doxyfile.in")
        return replacements[key]

    content = re.sub(r"@\w+@", _replace, doxyfile_in.read_text())
    doxyfile_out.write_text(content)


def setup(app):
    app.connect("config-inited", generate_doxyfile, priority=100)
    return {"parallel_read_safe": True, "parallel_write_safe": True}
