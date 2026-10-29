---
myst:
  html_meta:
    "description lang=en": "Versioning conventions for AMD SMI Library and Tools."
    "keywords": "system, management, interface, versioning, version, semantic, major, minor, release, compatibility, library, tool, API"
---

<a id="amd-smi-versioning"></a>

# Versioning guide for AMD SMI library and tool

## Overview

This section explains the versioning conventions used in the AMD SMI (System Management Interface) library and AMD SMI tool. Understanding these versioning rules helps developers maintain backward compatibility and communicate changes effectively.

## SMI library versioning

The SMI library uses a three-part version number format:

    major.minor.release

Version information is stored in the VERSION file at the root of the smi-lib repository:

    major=<int_major>
    minor=<int_minor>
    release=<int_release>


## Library version number components

### Major version (major)

The major version is incremented when changes affect backward compatibility of the API:

- Modification or removal of existing structures that break backward compatibility
- Breaking changes to enum definitions (removing values or reordering)
- Modifications to API function signatures
- Deprecating or removing existing APIs
- Any other breaking change to declarations in the amdsmi.h header file

Comments, including Doxygen comments, are not API changes. Editing, adding, or removing only comments does not increment the major version.

When the major version is incremented, both minor and release versions are reset to 0.

### Minor version (minor)

The minor version is incremented when new functionality is added in a backward-compatible manner:

- Addition of new structures
- Addition of new enum values (without reordering existing ones)
- Addition of new API functions
- Non-breaking modifications to existing structures (e.g., adding new fields at the end)
- Any other non-breaking additions to declarations in the amdsmi.h header file

Comment-only additions do not increment the minor version.

When the minor version is incremented, the release version is reset to 0.

### Release version (release)

The release version is incremented for implementation changes that don't affect the public API:

- Bug fixes
- Performance improvements
- Internal code refactoring
- Comment-only changes in the amdsmi.h header file
- Any other library change outside the paths listed below

The following paths do not represent a shipped library change. If they are the only files changed, the library version is left unchanged:

- `smi-lib/cli/`
- `smi-lib/tests/DTP/`
- `smi-lib/src/mock/`
- `smi-lib/inc/mock/`

`smi-lib/utils/hooks/version-check.sh` applies these rules to the aggregate diff against `origin/dev`, using that ref as the version baseline on every commit. If `origin/dev` is absent, the same comparison uses `HEAD`. It classifies `amdsmi.h` after C and C++ comments are removed: a removed or modified code line increments major, an addition with no removed code increments minor, and every other relevant change increments release.

## SMI tool versioning

The SMI tool uses a similar three-part version number format:

    major.minor.release

Version information is defined in the smi_cli_helpers.h file:

    #define AMDSMI_TOOL_VERSION_MAJOR 27
    #define AMDSMI_TOOL_VERSION_MINOR 6
    #define AMDSMI_TOOL_VERSION_RELEASE 0


## Tool version number components

### Major version

The major version is incremented when changes affect the tool's command interface:

- Deprecating or removing existing commands
- Modifying input/output format of commands

### Minor version

The minor version is incremented for command changes that maintain backward compatibility:

- Adding new commands
- Improvements to existing commands
- Adding new options to existing commands without changing the basic input/output format

### Release version

The release version is incremented for minor bug fixes and maintenance updates that don't add features or change command behavior.

## Version usage

### In code

Version numbers should be used throughout the codebase:

- For conditional compilation
- For feature detection
- For API compatibility checks

### In documentation

When referencing features or behavior, always include the version number where the feature was introduced or modified.

## Version compatibility

The SMI library and tool versions are not directly tied to each other, but certain tool versions may require minimum library versions to function correctly.

### Updating versions

1. Identify the type of change being made
2. Update the appropriate version number in the VERSION file or header file
3. Document the version change in the changelog

## Examples

### AMD SMI library

#### Example 1: bug fix in library implementation

- Before: 31.1.2
- Change: Fix a bug in the temperature reporting function
- After: 31.1.3 (only release version incremented)

#### Example 2: new API function

- Before: 31.1.3
- Change: Add new function amdsmi_get_new_metric()
- After: 31.2.0 (minor version incremented, release reset)

#### Example 3: modify existing API signature

- Before: 31.2.0
- Change: Change parameter types in amdsmi_existing_function()
- After: 32.0.0 (major version incremented, minor and release reset)

#### Example 4: Comment-only Header Change

- Before: 32.0.0
- Change: Clarify a Doxygen comment in amdsmi.h
- After: 32.0.1 (only release version incremented)

#### Example 5: DTP or Mock Change Only

- Before: 32.0.1
- Change: Update a DTP test or mock implementation
- After: 32.0.1 (version unchanged)

### AMD SMI tool:

#### Example 1: add new tool command

- Before: 27.6.0
- Change: Add new command amdsmi new-command
- After: 27.7.0 (minor version incremented)