#!/bin/bash

# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT

# Library version is calculated from the aggregate diff against origin/dev.
# If origin/dev is absent, HEAD is the baseline for the version, header, and tree.
#   major   - amdsmi.h code line removed or modified
#   minor   - amdsmi.h code line added, with no removed code
#   release - any other change outside the ignore list
#   none    - only ignored paths changed
#
# Comments in amdsmi.h are not API changes. A comment-only header edit is a
# release bump because the shipped header changed but its declarations did not.

REPO_ROOT=$(git rev-parse --show-toplevel)
cd "$REPO_ROOT" || exit 1

VERSION_FILE="$REPO_ROOT/smi-lib/VERSION"
HEADER_FILE="$REPO_ROOT/smi-lib/interface/amdsmi.h"
HEADER_PATH="smi-lib/interface/amdsmi.h"
BASE_REF="origin/dev"

# These paths never require a version bump on their own.
VERSION_IGNORE_PATHS=(
    "smi-lib/cli"
    "smi-lib/tests/DTP"
    "smi-lib/src/mock"
    "smi-lib/inc/mock"
    "smi-lib/VERSION"
)

if [ ! -f "$VERSION_FILE" ]; then
    echo "VERSION file not found!"
    exit 1
fi

strip_c_comments() {
    awk '
    BEGIN {
        in_block = 0
        in_string = 0
        in_char = 0
        sq = sprintf("%c", 39)
        dq = sprintf("%c", 34)
    }
    {
        sub(/\r$/, "")
        out = ""
        i = 1
        n = length($0)
        while (i <= n) {
            c = substr($0, i, 1)
            d = (i < n) ? substr($0, i + 1, 1) : ""
            if (in_block) {
                if (c == "*" && d == "/") {
                    in_block = 0
                    i += 2
                } else {
                    i++
                }
                continue
            }
            if (in_string || in_char) {
                out = out c
                if (c == "\\") {
                    if (d != "") {
                        out = out d
                    }
                    i += 2
                } else {
                    if (in_string && c == dq) {
                        in_string = 0
                    } else if (in_char && c == sq) {
                        in_char = 0
                    }
                    i++
                }
                continue
            }
            if (c == "/" && d == "*") {
                in_block = 1
                i += 2
                continue
            }
            if (c == "/" && d == "/") {
                break
            }
            if (c == dq) {
                in_string = 1
            } else if (c == sq) {
                in_char = 1
            }
            out = out c
            i++
        }
        if (match(out, /[ \t]+$/)) {
            out = substr(out, 1, RSTART - 1)
        }
        if (out ~ /[^ \t]/) {
            print out
        }
    }
    '
}

if git rev-parse --verify --quiet "$BASE_REF" >/dev/null; then
    COMPARE_REF="$BASE_REF"
else
    COMPARE_REF="HEAD"
fi
version_source="$COMPARE_REF:smi-lib/VERSION"

original_version=$(git show "$version_source") || {
    echo "Could not read $version_source"
    exit 1
}
original_major=$(echo "$original_version" | grep 'major=' | cut -d '=' -f 2)
original_minor=$(echo "$original_version" | grep 'minor=' | cut -d '=' -f 2)
original_release=$(echo "$original_version" | grep 'release=' | cut -d '=' -f 2)
if ! [[ "$original_major" =~ ^[0-9]+$ && "$original_minor" =~ ^[0-9]+$ && "$original_release" =~ ^[0-9]+$ ]]; then
    echo "Invalid version in $version_source"
    exit 1
fi

update_version() {
    local new_major=$1
    local new_minor=$2
    local new_release=$3
    printf "major=%s\n" "$new_major" > "$VERSION_FILE"
    printf "minor=%s\n" "$new_minor" >> "$VERSION_FILE"
    printf "release=%s" "$new_release" >> "$VERSION_FILE"
}

has_removed_code() {
    echo "$code_diff" | grep -E '^-' | grep -vE '^---' | grep -q .
}

has_added_code() {
    echo "$code_diff" | grep -E '^\+' | grep -vE '^\+\+\+' | grep -q .
}

old_header=$(mktemp)
new_header=$(mktemp)
old_code=$(mktemp)
new_code=$(mktemp)
cleanup() {
    rm -f "$old_header" "$new_header" "$old_code" "$new_code"
}
trap cleanup EXIT

if git cat-file -e "$COMPARE_REF:$HEADER_PATH" 2>/dev/null; then
    git show "$COMPARE_REF:$HEADER_PATH" > "$old_header"
else
    : > "$old_header"
fi
if [ -f "$HEADER_FILE" ]; then
    cat "$HEADER_FILE" > "$new_header"
else
    : > "$new_header"
fi

strip_c_comments < "$old_header" > "$old_code"
strip_c_comments < "$new_header" > "$new_code"
code_diff=$(diff -U0 "$old_code" "$new_code" || true)

if has_removed_code; then
    update_version "$((original_major + 1))" 0 0
    echo "Major version has been incremented in $VERSION_FILE."
elif has_added_code; then
    update_version "$original_major" "$((original_minor + 1))" 0
    echo "Minor version has been incremented in $VERSION_FILE."
else
    ignore_pathspec=()
    for path in "${VERSION_IGNORE_PATHS[@]}"; do
        ignore_pathspec+=(":!$path")
    done
    if git diff --name-only --no-ext-diff -a "$COMPARE_REF" -- . "${ignore_pathspec[@]}" | grep -q .; then
        update_version "$original_major" "$original_minor" "$((original_release + 1))"
        echo "Release version has been incremented in $VERSION_FILE."
    else
        echo "No version-relevant changes found. Version has not been incremented."
        exit 0
    fi
fi

git add "$VERSION_FILE"
exit 0
