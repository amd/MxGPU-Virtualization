#

## Integration tests on Windows

### Requirements on Windows

Binary should be in the root folder( C:\Windows\System32\) or in the same folder as integration_test_main.py, under the name amd-smi.exe.

### Saving test results

Add --save in command line in order to save test results in result folder

### Run on Windows

``shell python integration_test_main.py``
or
``shell python integration_test_main.py --save``

## Integration tests on Linux

### Requirements on Linux

Binary should be in the root folder( /usr/bin/) or in the same folder as integration_test_main.py, under the name amd-smi.

### Run on Linux

``shell sudo python3 integration_test_main.py``
or
``shell sudo python3 integration_test_main.py --save``

## Usage

The tests will run each supported command with options and test whether the tool will execute it with or without problems. On standard output, it will print every executed command and show every failed test. At the end, it will show how many tests were passed, failed and skipped.

## Examples (Windows)

### All tests passed on Windows

``shell python integration_test_main.py``
*
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
*

----------------------------------------------------------------------
Ran 3 tests in 18.114s

OK

### Some tests failed on Windows

*
    amd-smi.exe static --asic --json
    amd-smi.exe static --bus --json
    amd-smi.exe static --vbios --json
    amd-smi.exe static --board --json
*

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

## Examples (Linux)

### All tests passed on Linux

``shell python3 integration_test_main.py``
*
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
*

----------------------------------------------------------------------
Ran 3 tests in 18.114s

OK

### Some tests failed on Linux

*
    amd-smi.exe static --asic --json
    amd-smi.exe static --bus --json
    amd-smi.exe static --vbios --json
    amd-smi.exe static --board --json
*

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
