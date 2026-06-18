#!/bin/bash -e

# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

cd "$(dirname "${BASH_SOURCE[0]}")/../.."
make -C ./smi-lib
make -C ./smi-lib/cli/cpp
make -C ./smi-lib install PREFIX=/usr/local
make -C ./smi-lib/cli/cpp install PREFIX=/usr/local
make -C ./smi-lib clean
make -C ./smi-lib/cli/cpp clean
