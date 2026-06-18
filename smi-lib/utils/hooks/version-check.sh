#!/bin/bash

# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


REPO_ROOT=$(git rev-parse --show-toplevel)
VERSION_FILE="$REPO_ROOT/smi-lib/VERSION"
HEADER_FILE="$REPO_ROOT/smi-lib/interface/amdsmi.h"
if [ ! -f "$VERSION_FILE" ]; then
    echo "VERSION file not found!"
    exit 1
fi

if git log --branches --not --remotes | grep -q .; then
    original_version=$(git show origin/dev:"smi-lib/VERSION")
else
    original_version=$(git show HEAD:"VERSION")
fi
original_major=$(echo "$original_version" | grep 'major=' | cut -d '=' -f 2)
original_minor=$(echo "$original_version" | grep 'minor=' | cut -d '=' -f 2)
original_release=$(echo "$original_version" | grep 'release=' | cut -d '=' -f 2)
source "$VERSION_FILE"
update_version() {
    local new_major=$1
    local new_minor=$2
    local new_release=$3
    printf "major=%s\n" "$new_major" > "$VERSION_FILE"
    printf "minor=%s\n" "$new_minor" >> "$VERSION_FILE"
    printf "release=%s" "$new_release" >> "$VERSION_FILE"
}
major_change=false
diff_output=$(git diff --no-ext-diff --unified=0 -a --no-prefix origin/dev -- "$HEADER_FILE")

# Check if there are any changes in the HEADER_FILE
if [ -z "$diff_output" ]; then
    changes_outside_cli=$(git diff origin/dev -- ':!smi-lib/cli/' | grep -q . && echo "yes" || echo "no")
    if [ "$changes_outside_cli" = "yes" ]; then
        update_version $original_major $original_minor "$((original_release + 1))"
        git add "$VERSION_FILE"
        echo "Release version has been incremented in $VERSION_FILE."
    else
        echo "Only changes in cli/ directory found. Release version has not been incremented."
    fi
    exit 0
fi
# Detect deletions or modifications in header file
if echo "$diff_output" | sed 's/^--- [^ ]* //; /^---/d' | egrep '^\-' | grep -q .; then
    major=$((original_major + 1))
    minor=0
    release=0
    update_version $major $minor $release
    echo "Major version has been incremented in $VERSION_FILE."
else
    major=$original_major
    minor=$((original_minor + 1))
    release=0
    update_version $major $minor $release
    echo "Minor version has been incremented in $VERSION_FILE."
fi
git add "$VERSION_FILE"
exit 0