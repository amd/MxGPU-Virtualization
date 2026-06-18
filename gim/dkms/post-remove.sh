#!/bin/bash -e

# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

cd "$(dirname "${BASH_SOURCE[0]}")/../.."
make -C ./smi-lib clean
make -C ./smi-lib/cli/cpp clean
rm -f /usr/local/include/amdsmi.h /usr/local/bin/amd-smi /usr/local/lib/libamdsmi.*
