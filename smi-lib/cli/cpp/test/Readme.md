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

# Integration test on Windows

## Requirements
Binary should be in the root folder( C:\Windows\System32\) or in the same folder as integration_test_main.py, under the name amd-smi.exe.

## Saving test results
Add --save in command line in order to save test results in result folder

## Run
python integration_test_main.py
or
python integration_test_main.py --save

# Integration test on Linux

## Requirements
Binary should be in the root folder( /usr/bin/) or in the same folder as integration_test_main.py, under the name amd-smi.

## Run
sudo python3 integration_test_main.py
or
sudo python3 integration_test_main.py --save

# Usage
The tests will run each supported command with options and test whether the tool will execute it with or without problems. On standard output, it will print every executed command and show every failed test. At the end, it will show how many tests were passed, failed and skipped.

# Examples (Windows)

## All tests are passed
python integration_test_main.py
*```
amd-smi.exe static --asic --json
amd-smi.exe static --bus --json
amd-smi.exe static --vbios --json
amd-smi.exe static --board --json
amd-smi.exe static --limit --json
amd-smi.exe static --driver --json
amd-smi.exe static --ras --json
amd-smi.exe static --dfc-ucode --json
amd-smi.exe static --fb-info --json
amd-smi.exe static --num-vf --json
.
----------------------------------------------------------------------
Ran 3 tests in 18.114s

OK

## Some test are faild
*```
amd-smi.exe static --asic --json
amd-smi.exe static --bus --json
amd-smi.exe static --vbios --json
amd-smi.exe static --board --json
*```
======================================================================
FAIL: test_vf_csv_format (integration_test_host.TestHost) (command='metric')
----------------------------------------------------------------------
AssertionError: Command 'metric --csv' is not supported on the system. Run 'help' for more info.
Executed command: amd-smi.exe metric --vf=6:0  --csv

======================================================================
FAIL: test_vf_csv_format (integration_test_host.TestHost) (command='metric')
----------------------------------------------------------------------
AssertionError: Command 'metric --csv' is not supported on the system. Run 'help' for more info.
Executed command: amd-smi.exe metric --vf=7:0  --csv

----------------------------------------------------------------------
Ran 6 tests in 719.208s

FAILED (failures=24)

# Examples (Linux)

## All tests are passed
python3 integration_test_main.py
*```
amd-smi.exe static --asic --json
amd-smi.exe static --bus --json
amd-smi.exe static --vbios --json
amd-smi.exe static --board --json
amd-smi.exe static --limit --json
amd-smi.exe static --driver --json
amd-smi.exe static --ras --json
amd-smi.exe static --dfc-ucode --json
amd-smi.exe static --fb-info --json
amd-smi.exe static --num-vf --json
.
----------------------------------------------------------------------
Ran 3 tests in 18.114s

OK

## Some test are faild
*```
amd-smi.exe static --asic --json
amd-smi.exe static --bus --json
amd-smi.exe static --vbios --json
amd-smi.exe static --board --json
*```
======================================================================
FAIL: test_vf_csv_format (integration_test_host.TestHost) (command='metric')
----------------------------------------------------------------------
AssertionError: Command 'metric --csv' is not supported on the system. Run 'help' for more info.
Executed command: amd-smi.exe metric --vf=6:0  --csv

======================================================================
FAIL: test_vf_csv_format (integration_test_host.TestHost) (command='metric')
----------------------------------------------------------------------
AssertionError: Command 'metric --csv' is not supported on the system. Run 'help' for more info.
Executed command: amd-smi.exe metric --vf=7:0  --csv

----------------------------------------------------------------------
Ran 6 tests in 719.208s

FAILED (failures=24)