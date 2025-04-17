---
myst:
  html_meta:
    "description lang=en": "Learn how to use the AMD SMI command line tool."
    "keywords": "api, smi, c++, lib, system, management, interface, example"
---

# Overview - AMD SMI tool

AMD SMI tool is a command line utility that utilizes AMD SMI Library APIs to monitor AMD GPUs. The tool is used to monitor AMD’s GPUs status in a virtualization environment in Linux host Operating Systems and Windows guest Operating Systems. The tool outputs GPU/driver information in plain text, in JSON, or in CSV formats while it can also show the info in the console or save to the specified output file.

## AMD SMI tool build

Before running the command to build the tool, make sure you are meeting the following requirements on your system:
    -cmake minimum version 3.15
    -g++ minimum version 8

When running make inside the gim/smi-lib/cli/cpp folder, the AMD SMI Tool will be built. Here are some useful commands for building the AMD SMI library:

- Run `make` in the gim/smi-lib/cli/cpp folder to build the tool.
- Run `make clean` to remove all files generated during the build process, such as object files and executables, to ensure clean build environment.

After build is successfully finished, navigate to gim/smi-lib/cli/cpp/build folder and tool binary should be there.
Open terminal and navigate to this location and now you can execute smi tool.

## Folder structure

SMI Tool folder structure is shown below:

```text
cli/
└── cpp/
    ├── cmake/                # Contains all CMake files used in the build
    │   └── linux/
    ├── docs/
    │   └── external/         # Contains documents
    ├── inc/                  # Internal include files
    ├── src/                  # Source files
    │   ├── guest/            # Windows Guest-specific source files
    │   └── host/             # Host-specific source files
    ├── test/                 # Contains integration tests
    └── utils/
        ├── scripts/          # Utility scripts
        └── third_party/
            └── inc/          # Third-party libraries
                ├── json/
                └── tabulate/
```

## Usage and basic commands

```shell-session
sudo ./amd-smi <arguments>
```

- `<arguments>` arguments can be command name, subcommands, modifiers, and other arguments.

To get detailed information about the available commands and options, you can run help command.
The help command provides a comprehensive overview of the tool's functionalities and usage instructions.
Simply run tool without arguments or with command help.

```shell-session
$ sudo ./amd-smi help

Copyright 2023-2025 Advanced Micro Devices, Inc. All rights reserved.

usage: amd-smi help

AMD System Management Interface | AMD SMI tool version 24.16.5.0

AMD-SMI Commands:
                      Descriptions:
    version           Display version information
    list              List GPU information
    static            Gets static information about the specified GPU
    metric            Gets metric information about the specified GPU
    monitor           Monitor metrics for target devices
    bad-pages         Gets bad page information about the specified GPU
    event             Displays event information for the given GPU
    firmware          Gets firmware information about the specified GPU
    set               Set options for devices
    reset             Reset options for devices
    xgmi              Displays xgmi information of the devices
    topology          Displays topology information of the devices
    partition         Displays partition information of the devices
```

From help message you can se which subcommand are supported on the system and a short description for each command.

To access the help documentation for a specific command, simply use that command name followed by the help command.
For example, if you want to get help for "list" command you can use tool the following way.

```shell-session
$ sudo ./amd-smi list --help

Copyright 2023-2024 Advanced Micro Devices, Inc. All rights reserved.

usage: amd-smi list [-h | --help] [--json | --csv] [--file FILE] [-g | --gpu [GPU ...]]

List all GPUs and VFs on the system and their most basic general information.
If no GPU is specified, returns metric information for all GPUs on the system.

List arguments:
                          Description:
    -h, --help            show this help message and exit
    -g, --gpu [GPU ...]   Select a GPU ID, BDF or UUID, if not selected it will return for all GPUs

Command Modifiers:
                      Description:
--json                Displays output in JSON format (humman readable by default).
--csv                 Displays output in CSV format (humman readable by default).
--file FILE           Saves output into a file on the provided path (stdout by default).
```

Here is the example of how you can now use the tool to get list command output in human(plain), json and csv format, by using modifiers.

```shell-session
$ sudo ./amd-smi list

GPU: 0
    BDF: 0000:0c:00.0
    UUID: 67ff74a1-0000-1000-8081-b5b9fd6edd00
    VF: 0
        BDF: 0000:0c:02.0
        UUID: 670074a1-0000-1000-8081-b5b9fd6edd00
```

```shell-session
$ sudo ./amd-smi list --json

[
    {
        "gpu": 0,
        "bdf": "0000:0c:00.0",
        "uuid": "67ff74a1-0000-1000-8081-b5b9fd6edd00",
        "vfs": [
            {
                "vf": 0,
                "bdf": "0000:0c:02.0",
                "uuid": "670074a1-0000-1000-8081-b5b9fd6edd00"
            }
        ]
    }
]
```

```shell-session
$ sudo ./amd-smi list --csv

gpu,gpu_bdf,gpu_uuid,vf,vf_bdf,vf_uuid
0,0000:0c:00.0,67ff74a1-0000-1000-8081-b5b9fd6edd00,0,0000:0c:02.0,670074a1-0000-1000-8081-b5b9fd6edd00
```

And also all outputs can be saved to a file.

```shell-session
sudo ./amd-smi list --file=<file_name>
```
