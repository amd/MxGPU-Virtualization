# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

"""Configuration file for the Sphinx documentation builder."""

import os

html_baseurl = os.environ.get("READTHEDOCS_CANONICAL_URL", "instinct.docs.amd.com")
html_context = {}
if os.environ.get("READTHEDOCS", "") == "True":
    html_context["READTHEDOCS"] = True
project = "AMD Instinct Virtualization Driver"

version = "0.1.0"
release = version
html_title = project
author = "Advanced Micro Devices, Inc."
copyright = "Copyright (c) 2026 Advanced Micro Devices, Inc. All rights reserved."

# Required settings
html_theme = "rocm_docs_theme"
html_theme_options = {
    "flavor": "instinct",
    "repository_provider": "github",
    "repository_url": "https://github.com/amd/MxGPU-Virtualization",
    "link_main_doc": True,
    "use_repository_button": True,
    "use_issues_button": True,
    "use_download_button": True,
}
extensions = ["rocm_docs"]

external_projects_current_project = "instinct"

# Generate llms.txt and llms-full.txt.
rocm_docs_generate_llms = True

# Table of contents
external_toc_path = "./sphinx/_toc.yml"

exclude_patterns = [".venv"]
