---
myst:
  html_meta:
    "description lang=en": "Versioning conventions for AMD SMI Library and Tools."
    "keywords": "system, management, interface, versioning, version, semantic, major, minor, release, compatibility, library, tool, API"
---

<a id="amd-smi-versioning"></a>

# Versioning Guide for AMD SMI Library and Tool

## Overview

This section explains the versioning conventions used in the AMD SMI (System Management Interface) library and AMD SMI tool. Understanding these versioning rules helps developers maintain backward compatibility and communicate changes effectively.

## SMI Library Versioning

The SMI library uses a three-part version number format:

    major.minor.release

Version information is stored in the VERSION file at the root of the smi-lib repository:

    major=<int_major>
    minor=<int_minor>
    release=<int_release>


## Version Number Components

### Major Version (major)

The major version is incremented when changes affect backward compatibility of the API:

- Modification or removal of existing structures that break backward compatibility
- Breaking changes to enum definitions (removing values or reordering)
- Modifications to API function signatures
- Deprecating or removing existing APIs
- Any other breaking change in the amdsmi.h header file

When the major version is incremented, both minor and release versions are reset to 0.

### Minor Version (minor)

The minor version is incremented when new functionality is added in a backward-compatible manner:

- Addition of new structures
- Addition of new enum values (without reordering existing ones)
- Addition of new API functions
- Non-breaking modifications to existing structures (e.g., adding new fields at the end)
- Any other non-breaking additions to the amdsmi.h header file

When the minor version is incremented, the release version is reset to 0.

### Release Version (release)

The release version is incremented for implementation changes that don't affect the public API:

- Bug fixes
- Performance improvements
- Internal code refactoring
- Any changes outside the amdsmi.h header file (excluding cli/cpp folders)

## SMI Tool Versioning

The SMI tool uses a similar three-part version number format:

    major.minor.release

Version information is defined in the smi_cli_helpers.h file:

    #define AMDSMI_TOOL_VERSION_MAJOR 27
    #define AMDSMI_TOOL_VERSION_MINOR 6
    #define AMDSMI_TOOL_VERSION_RELEASE 0


## Version Number Components

### Major Version

The major version is incremented when changes affect the tool's command interface:

- Deprecating or removing existing commands
- Modifying input/output format of commands

### Minor Version

The minor version is incremented for command changes that maintain backward compatibility:

- Adding new commands
- Improvements to existing commands
- Adding new options to existing commands without changing the basic input/output format

### Release Version

The release version is incremented for minor bug fixes and maintenance updates that don't add features or change command behavior.

## Version Usage

### In Code

Version numbers should be used throughout the codebase:

- For conditional compilation
- For feature detection
- For API compatibility checks

### In Documentation

When referencing features or behavior, always include the version number where the feature was introduced or modified.

## Version Compatibility

The SMI library and tool versions are not directly tied to each other, but certain tool versions may require minimum library versions to function correctly.

### Updating Versions

1. Identify the type of change being made
2. Update the appropriate version number in the VERSION file or header file
3. Document the version change in the changelog

## Examples

### AMD SMI library:

#### Example 1: Bug Fix in Library Implementation

- Before: 31.1.2
- Change: Fix a bug in the temperature reporting function
- After: 31.1.3 (only release version incremented)

#### Example 2: New API Function

- Before: 31.1.3
- Change: Add new function amdsmi_get_new_metric()
- After: 31.2.0 (minor version incremented, release reset)

#### Example 3: Modify Existing API Signature

- Before: 31.2.0
- Change: Change parameter types in amdsmi_existing_function()
- After: 32.0.0 (major version incremented, minor and release reset)

### AMD SMI tool:

#### Example 1: Add New Tool Command

- Before: 27.6.0
- Change: Add new command amdsmi new-command
- After: 27.7.0 (minor version incremented)