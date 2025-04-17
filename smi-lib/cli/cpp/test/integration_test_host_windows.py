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
import unittest
import integration_test
from utilities import *
from helpers import *
import commands as cmd
import sys

__unittest = True
class TestHostWindows(integration_test.Test):
    def __init__(self, *args, **kwargs):
        super(TestHostWindows, self).__init__(*args, **kwargs)
        self.commands_vf = cmd.globalCommands.getVfCommands()
        self.options_vf = cmd.globalCommands.getVfOptions()
        self.csv_commands = cmd.globalCommands.getCsvCommands()
        self.csv_options = cmd.globalCommands.getCsvOptions()

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_csv(self):
        self._commands_tester(OutputFormat.CSV, self.csv_commands, self.csv_options)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_csv_and_saving(self):
        self._create_folder("CSV")
        self._commands_tester(OutputFormat.CSV, self.csv_commands, self.csv_options, True)

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_gpu_vf_human(self):
        self._gpu_vf_tester(OutputFormat.HUMAN)

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_gpu_vf_csv(self):
        self._gpu_vf_tester(OutputFormat.CSV, self.csv_commands, self.csv_options)

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_gpu_vf_json(self):
        self._gpu_vf_tester(OutputFormat.JSON)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_gpu_vf_human_and_saving(self):
        self._create_folder("HUMAN")
        self._gpu_vf_tester(OutputFormat.HUMAN, True)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_gpu_vf_csv_and_saving(self):
        self._create_folder("CSV")
        self._gpu_vf_tester(OutputFormat.CSV, self.csv_commands, self.csv_options, True)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_gpu_vf_json_and_saving(self):
        self._create_folder("JSON")
        self._gpu_vf_tester(OutputFormat.JSON, True)

    def _vf_option_tester(self, vf, outputformat, isFile, vf_value = None):
        file = ""
        tool = settings.globalSettings.get_tool_binary()
        vf_str = ""
        if isFile:
            if vf_value == GpuVfValue.BDF:
                vf_str = get_formated_string_from_bdf(vf)
            else:
                vf_str = vf

        for i in range(len(self.commands_vf)):
            if isFile:
                file = "--file=" + self.file_path + str(i) + "_" + str(self.commands_vf[i]) + vf_str + outputformat + ".txt"

            self._check_error_output([tool, self.commands_vf[i], vf, "", outputformat, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

            for option in self.options_vf[i]:
                if isFile:
                    file = "--file=" + self.file_path + str(i) + "_" + str(self.commands_vf[i]) + str(option) + outputformat + vf_str + ".txt"

                self._check_error_output([tool, self.commands_vf[i], vf, option, outputformat, file],
                                      [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

    def _gpu_vf_tester(self, outputformat, commands = None, options = None, isFile=False):
        if commands == None or options == None:
            commands = self.commands
            options = self.options
        # Testing commands that should not work on platform
        pf_bdf_list, pf_index_list, pf_uuid_list, vf_bdf_list, vf_index_list, vf_uuid_list = get_gpu_vf_info()

        # Testing commands with parameter --gpu=BDF
        self._gpu_option_tester(pf_bdf_list, outputformat, commands, options, isFile, GpuVfValue.BDF)

        # Testing commands with parameter --vf=BDF
        for j in range(len(vf_bdf_list)):
            vf = "--vf=" + str(vf_bdf_list[j])
            self._vf_option_tester(vf, outputformat, isFile, GpuVfValue.BDF)

        # Testing commands with parameter --gpu=UUID
        self._gpu_option_tester(pf_uuid_list, outputformat, commands, options, isFile, GpuVfValue.UUID)

        # Testing commands with parameter --vf=UUID
        for j in range(len(vf_uuid_list)):
            vf = "--vf=" + str(vf_uuid_list[j])
            self._vf_option_tester(vf, outputformat, isFile, GpuVfValue.UUID)

        # Testing commands with parameter --gpu=index
        self._gpu_option_tester(pf_index_list, outputformat, commands, options, isFile, GpuVfValue.INDEX)

        # Testing commands with parameter --vf=index
        for dv in pf_index_list:
            for j in range(len(vf_index_list[dv])):
                vf = "--vf=" + str(dv) + ":" + str(vf_index_list[dv][j])
                self._vf_option_tester(vf, outputformat, isFile, GpuVfValue.INDEX)