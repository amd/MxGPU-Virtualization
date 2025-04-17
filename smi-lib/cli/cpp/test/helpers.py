#
# Copyright (C) 2024 Advanced Micro Devices. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
from pathlib import Path
from collections.abc import Sequence
from enum import Enum
import os

pwd = os.getcwd()

class OutputFormat(str, Enum):
    HUMAN = ""
    JSON = "--json"
    CSV = "--csv"

class PlatformType(Enum):
    HOST = 1
    GUEST = 2
    BAREMETAL = 3
    UNKNOWN = 4

class PlatformName(Enum):
    LINUX = 1
    WINDOWS = 2
    UNKNOWN = 3

class GpuVfValue(Enum):
    INDEX = 1
    BDF = 2
    UUID = 3

class ErrorType(Enum):
    NO_ERROR = 0
    INVALID_COMMAND = -1
    INVALID_PARAMETER = -2
    DEVICE_NOT_FOUND = -3
    PATH_NOT_FOUND = -4
    INVALID_TYPE_OR_FORMAT = -5
    REQUIRES_VALUE = -6
    COMMAND_NOT_SUPPORTED_ON_SYSTEM = -7
    PARAMETER_NOT_SUPPORTED_ON_SYSTEM = -8
    UNKNOWN_ERROR = -1000