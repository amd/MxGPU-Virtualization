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

class TestGuestLinux(integration_test.Test):

    def setUp(self):
        self.longMessage = False
        self.tool = settings.globalSettings.get_tool_binary()
        self.commands = cmd.globalCommands.getCommands()
        self.options = cmd.globalCommands.getOptions()

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_csv(self):
        self._commands_tester(OutputFormat.CSV, self.csv_commands, self.csv_options)

    @unittest.skipIf("--save" not in sys.argv,"not saving part")
    def test_csv_and_saving(self):
        self._create_folder("CSV")
        self._commands_tester(OutputFormat.CSV, self.csv_commands, self.csv_options, True)

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_gpu_human(self):
        self._gpu_pids_names_tester(OutputFormat.HUMAN)

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_gpu_csv(self):
        self._gpu_pids_names_tester(OutputFormat.CSV)

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_gpu_json(self):
        self._gpu_pids_names_tester(OutputFormat.JSON)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_gpu_human_and_saving(self):
        self._create_folder("HUMAN")
        self._gpu_pids_names_tester(OutputFormat.HUMAN, True)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_gpu_csv_and_saving(self):
        self._create_folder("CSV")
        self._gpu_pids_names_tester(OutputFormat.CSV, True)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_gpu_json_and_saving(self):
        self._create_folder("JSON")
        self._gpu_pids_names_tester(OutputFormat.JSON, True)

    def test_general(self):
        tool = settings.globalSettings.get_tool_binary()
        self._create_folder("general")
        path = self.file_path

        command = "set"
        file =  "--file=" + path + "/" + command + ".txt"
        self._check_error_output([tool, command, "--help", OutputFormat.HUMAN, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

        command = "reset"
        file =  "--file=" + path + "/" + command + ".txt"
        self._check_error_output([tool, command, "--help", OutputFormat.HUMAN, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

        command = "list"
        file =  "--file=" + path + "/" + command + ".txt"
        self._check_error_output([tool, command, "--help", OutputFormat.HUMAN, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

    def _gpu_pids_names_tester(self, outputformat, isFile=False):
        file = ""
        pf_bdf_list, pf_index_list, pf_uuid_list = get_gpu_vf_info()

        # Testing commands with parameter --gpu=BDF
        self._gpu_option_tester(pf_bdf_list, outputformat, self.commands, self.options, isFile, GpuVfValue.BDF)

        # Testing commands with parameter --gpu=UUID
        self._gpu_option_tester(pf_uuid_list, outputformat, self.commands, self.options, isFile, GpuVfValue.UUID)

        # Testing commands with parameter --gpu=index
        self._gpu_option_tester(pf_index_list, outputformat, self.commands, self.options, isFile, GpuVfValue.INDEX)

        pids, names = get_process_info()

        # Testing process command with --pid and --name
        for i in range(len(pids)):
            process = "--pid=" + str(pids[i])
            if isFile:
                file = "--file=" + self.file_path + str(i) + "_" + str(process) + outputformat + ".txt"

            self._check_error_output([tool, "process", process, "", outputformat, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

        for i in range(len(names)):
            process = "--name=" + str(names[i])
            if isFile:
                file = "--file=" + self.file_path + str(i) + "_" + str(process) + outputformat + ".txt"

            self._check_error_output([tool, "process", process, "", outputformat, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])


