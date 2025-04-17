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
from utilities import *
from helpers import *
import commands as cmd
import sys

__unittest = True
class Test(unittest.TestCase):
    def __init__(self, *args, **kwargs):
        super(Test, self).__init__(*args, **kwargs)
        self.commands = cmd.globalCommands.getCommands()
        self.options = cmd.globalCommands.getOptions()

    def setUp(self):
        self.longMessage = False

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_human(self):
        self._commands_tester(OutputFormat.HUMAN, self.commands, self.options)

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_csv(self):
        self._commands_tester(OutputFormat.CSV, self.commands, self.options)

    @unittest.skipIf("--save" in sys.argv,"not saving part")
    def test_json(self):
        self._commands_tester(OutputFormat.JSON, self.commands, self.options)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_human_and_saving(self):
        self._create_folder("HUMAN")
        self._commands_tester(OutputFormat.HUMAN, self.commands, self.options, True)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_csv_and_saving(self):
        self._create_folder("CSV")
        self._commands_tester(OutputFormat.CSV, self.commands, self.options, True)

    @unittest.skipIf("--save" not in sys.argv,"saving part")
    def test_json_and_saving(self):
        self._create_folder("JSON")
        self._commands_tester(OutputFormat.JSON, self.commands, self.options, True)

    def test_general(self):
        self._create_folder("general")
        tool = settings.globalSettings.get_tool_binary()
        file = ""

        command = "help"
        if "--save" in sys.argv:
            file =  "--file=" + self.file_path + "/" + command + ".txt"
        self._check_error_output([tool, command, OutputFormat.HUMAN, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

        command = "version"
        if "--save" in sys.argv:
            file =  "--file=" + self.file_path + "/" + command + ".txt"
        self._check_error_output([tool, command, OutputFormat.HUMAN, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])
        self._check_error_output([tool, command, OutputFormat.JSON, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])
        self._check_error_output([tool, command, OutputFormat.CSV, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

    def _create_folder(self, name):
        self.file_folder = output_folder_name + "/" + name + "/"
        self.file_path = pwd + "/" + self.file_folder
        if not os.path.exists(self.file_path):
            os.makedirs(self.file_path)

    def _check_error(self, stdout, stderr, args):
        err = ErrorType.NO_ERROR
        errs = stderr.decode("utf-8") if stderr.decode("utf-8") != "" else ""
        outs = stdout

        if re.search('^[a-zA-Z]*Error', errs):
            return ErrorType.UNKNOWN_ERROR, errs

        erroroccured = "An error occured with code:"
        newlist = [i for i, item in enumerate(args) if re.search('\A--file=', item) ]
        isFile = True if len(newlist) > 0 else False
        error_message = ""
        errorMessage = outs.decode("utf-8") if outs.decode("utf-8") != "" else ""
        if isFile:
            file_index = newlist[0]
            file_string = args[file_index]
            filename = file_string[7:]
            if os.path.exists(filename):
                file = open(filename, 'r')
                file_message = file.read()
                if file_message != "":
                    errorMessage = file_message
        isCSV = OutputFormat.CSV in args
        isJSON = OutputFormat.JSON in args
        if isCSV:
            error_msg =""
            error_in_err = True if ("error_code" in errs or re.search('^error$', errs)) else False
            error_in_out = True if ("error_code" in errorMessage or re.search('^error$', errorMessage)) else False
            if error_in_err or error_in_out:
                error_msg = errs if error_in_err else errorMessage
                err = int(error_msg.split(",")[2])
                error_message = error_msg.split(",")[1].split("\n")[1]
            elif errorMessage == "" and errs != "":
                start = errs.rfind(erroroccured) + len(erroroccured)
                end = errs.rfind("(")
                if end != -1:
                    err = int(errs[start:end])
                else:
                    error_message = "An unknown error occurred"
                    err = ErrorType.UNKNOWN_ERROR
            elif errorMessage == "":
                error_message = "An unknown error occurred"
                err = ErrorType.UNKNOWN_ERROR
        elif isJSON:
            error_msg =""
            error_in_err = True if ("error_code" in errs or re.search('^error$', errs)) else False
            error_in_out = True if ("error_code" in errorMessage or re.search('^error$', errorMessage)) else False
            if error_in_err or error_in_out:
                error_msg = errs if error_in_err else errorMessage
                if error_msg[-2:] == "\r\n":
                    error_msg = error_msg[:-2]
                elif error_msg[-1] == "\n":
                    error_msg = error_msg[:-1]
                error_msg = error_msg[1:-1].split(",")
                err = int(error_msg[1].split(": ")[1])
                error_message = error_msg[0].split(": ")[1]
            elif errorMessage == "" and errs != "":
                start = errs.rfind(erroroccured) + len(erroroccured)
                end = errs.rfind("(")
                if end != -1:
                    err = int(errs[start:end])
                else:
                    error_message = "An unknown error occurred"
                    err = ErrorType.UNKNOWN_ERROR
            elif errorMessage == "":
                error_message = "An unknown error occurred"
                err = ErrorType.UNKNOWN_ERROR
        else:
            error_msg =""
            error_in_err = True if ("Error code" in errs or re.search('^Error$', errs)) else False
            error_in_out = True if ("Error code" in errorMessage or re.search('^Error$', errorMessage)) else False
            if error_in_err or error_in_out:
                error_msg = errs if error_in_err else errorMessage
                err = int(error_msg[error_msg.find(
                    "Error code: ") + len("Error code: "):])
                error_message = error_msg.partition(
                    "Error code: ")[0]

            elif errorMessage == "" and errs != "":
                start = errs.rfind(erroroccured) + len(erroroccured)
                end = errs.rfind("(")
                if end != -1:
                    err = int(errs[start:end])
                else:
                    error_message = "An unknown error occurred"
                    err = ErrorType.UNKNOWN_ERROR
            elif errorMessage == "":
                error_message = "An unknown error occurred"
                err = ErrorType.UNKNOWN_ERROR
        return err, error_message

    def assertIn(self, val1, val2, msg=None):
        if val1 not in val2:
            raise self.failureException(msg)

    def _check_error_output(self, args, checkvalue):
        main_command = args[1] if len(args) > 1 else tool_name

        executed_command =""
        for arg in args:
            executed_command += arg + " "

        with self.subTest(command=main_command):
            result = subprocess.Popen(executed_command,
                stdout=subprocess.PIPE, stdin=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)

            if main_command == "event":
                result.stdin.write(b"q\n")
                result.stdin.flush()

            stdout, stderrs = result.communicate()
            err, error_message = self._check_error(stdout, stderrs, args)
            command = tool_name + executed_command.split(tool_name)[1]
            print(command)
            msg=""
            if err != ErrorType.NO_ERROR and err != ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM:
                msg += error_message + "\nExecuted command: " + command
            else:
                msg+= "Command should throw an error" + "\nExecuted command: " + command
            self.assertIn(err, checkvalue, msg)

    def _gpu_option_tester(self, gpu_values, outputformat, commands, commands_options, isFile, gpu_value = None):
        file = ""
        tool = settings.globalSettings.get_tool_binary()
        gpu_str = ""

        for j in range(len(gpu_values)):
            gpu = "--gpu=" + str(gpu_values[j])
            for i in range(len(commands)):
                if isFile:
                    if gpu_value == GpuVfValue.BDF:
                        gpu_str = get_formated_string_from_bdf(gpu)
                    else:
                        gpu_str = gpu
                    file = "--file=" + self.file_path + str(i) + "_" + str(commands[i]) + gpu_str + outputformat + ".txt"
                self._check_error_output([tool, commands[i], gpu, "", outputformat, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

                for option in commands_options[i]:
                    if option == "--help":
                        continue
                    if isFile:
                        file = "--file=" + self.file_path + str(i) + "_" + str(commands[i]) + gpu_str + str(
                            option) + outputformat + ".txt"

                    self._check_error_output([tool, commands[i], gpu, option, outputformat, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

    def _commands_tester(self, outputformat, commands, commands_options, isFile=False):
        file = ""
        tool = settings.globalSettings.get_tool_binary()

        # Testing commands that should work on platform
        for i in range(len(commands)):
            if isFile:
                file = "--file=" + self.file_path + str(commands[i]) + outputformat + ".txt"
            self._check_error_output([tool, commands[i], outputformat, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])
            for option in commands_options[i]:
                if isFile:
                    file = "--file=" + self.file_path + str(i) + "_" + str(commands[i]) + str(option) + outputformat + ".txt"
                self._check_error_output([tool, commands[i], option, outputformat, file], [ErrorType.NO_ERROR, ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM])

    def _wrong_commands_tester(self, outputformat, commands_fail, commands_options_fail, isFile=False):
        file = ""
        tool = settings.globalSettings.get_tool_binary()

        # Testing commands that should not work on platform
        for i in range(len(commands_fail)):
            for option in commands_options_fail[i]:
                if isFile:
                    file = "--file=" + self.file_path + str(i) + "_" + str(commands_fail[i]) + str(option) + outputformat + ".txt"
                self._check_error_output([tool, commands_fail[i], option, outputformat, file], [ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM, ErrorType.PARAMETER_NOT_SUPPORTED_ON_SYSTEM])

    def _wrong_gpu_option_tester(self, gpu_values, outputformat, commands_fail, commands_options_fail, isFile, gpu_value = None):
        file = ""
        tool = settings.globalSettings.get_tool_binary()
        gpu_str = ""
        for j in range(len(gpu_values)):
            gpu = "--gpu=" + str(gpu_values[j])
            for i in range(len(commands_fail)):
                for option in commands_options_fail[i]:
                    if isFile:
                        if gpu_value == GpuVfValue.BDF:
                            gpu_str = get_formated_string_from_bdf(gpu)
                        else:
                            gpu_str = str(gpu[6:])
                        file = "--file=" + self.file_path + str(i) + "_" + str(commands_fail[i]) + str(option) + outputformat + gpu_str + ".txt"

                    self._check_error_output([tool, commands_fail[i], gpu, option, outputformat, file],
                                        [ErrorType.COMMAND_NOT_SUPPORTED_ON_SYSTEM, ErrorType.PARAMETER_NOT_SUPPORTED_ON_SYSTEM])


