#!/bin/sh
set -eu

# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

# Build and install userspace AMD SMI once for this package version.
# Package postinst/%posttrans calls this, not DKMS: the kernel modules built
# for every kernel share the same userspace AMD SMI.

ROOT=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
PREFIX=/usr/local
MANIFEST=/var/lib/gim/smi-manifest
NEW=
trap '[ -z "$NEW" ] || rm -f "$NEW"' EXIT

cd "$ROOT"

# Build the replacement before removing the installed version. A build
# failure must leave the current userspace AMD SMI intact.
make -C ./smi-lib
make -C ./smi-lib/cli/cpp

# On upgrade, uninstall the previous version before installing the new one.
# This also removes files that the new package no longer ships.
if [ -f "$MANIFEST" ]; then
	xargs -r rm -f -- < "$MANIFEST"
	rm -f "$MANIFEST"
fi

make -C ./smi-lib install PREFIX="$PREFIX"
make -C ./smi-lib/cli/cpp install PREFIX="$PREFIX"

mkdir -p /var/lib/gim
NEW=$(mktemp "${MANIFEST}.XXXXXX")
{
	printf '%s\n' "$PREFIX/lib/libamdsmi.so"
	for header in ./smi-lib/interface/*.h; do
		if [ -f "$header" ]; then
			printf '%s\n' "$PREFIX/include/$(basename "$header")"
		fi
	done
	# CMake records every file its install step wrote.
	cat ./smi-lib/cli/cpp/build/install_manifest.txt
	printf '\n'
} > "$NEW"

mv "$NEW" "$MANIFEST"
NEW=

make -C ./smi-lib clean
make -C ./smi-lib/cli/cpp clean
# make clean leaves the build directories behind. Remove them so package
# removal can delete /usr/src/gim-*.
rm -rf ./smi-lib/build ./smi-lib/cli/cpp/build
rm -f ./smi-lib/cli/cpp/CMakeLists.txt

ldconfig || true
