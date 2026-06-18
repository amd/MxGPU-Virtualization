#!/bin/bash -e

# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

cd "$(dirname "${BASH_SOURCE[0]}")"
autoreconf
./configure "$@"
