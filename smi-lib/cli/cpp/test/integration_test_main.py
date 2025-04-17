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
from integration_test_host_windows import *
from integration_test_host_linux import *
from integration_test_guest_windows import *
from integration_test_baremetal_linux import *
from integration_test_baremetal_windows import *
from integration_test_guest_linux import *
from utilities import *
import commands

import unittest
import settings
import shutil

if __name__ == '__main__':
    try:
        settings.initGlobalSettings()
        settings.globalSettings.init_asic_detection()

        commands.initGlobalCommands()

        # create folder to save test results
        pwd = os.getcwd()
        directory = pwd + "/" + output_folder_name
        if os.path.exists(directory):
            shutil.rmtree(directory)

        loader = unittest.TestLoader()

        if settings.globalSettings.is_host_machine():
            if settings.globalSettings.is_windows():
                suite = loader.loadTestsFromTestCase(TestHostWindows)
            else:
                suite = loader.loadTestsFromTestCase(TestHostLinux)
        elif settings.globalSettings.is_baremetal_machine():
            if settings.globalSettings.is_windows():
                suite = loader.loadTestsFromTestCase(TestBaremetalWindows)
            else:
                suite = loader.loadTestsFromTestCase(TestBaremetalLinux)
        else:
            if settings.globalSettings.is_windows():
                suite = loader.loadTestsFromTestCase(TestGuestWindows)
            else:
                suite = loader.loadTestsFromTestCase(TestGuestLinux)

        big_suite = unittest.TestSuite(suite)
        runner = unittest.TextTestRunner()
        results = runner.run(big_suite)
    except Exception as error:
        print(error)
    except:
        print("An exception occurred")