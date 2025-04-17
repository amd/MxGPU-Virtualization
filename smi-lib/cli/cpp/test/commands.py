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

import settings

globalCommands = None

class Commands:
    commands = []
    options = []
    vf_commands = []
    vf_options = []
    csv_commands = []
    csv_options = []
    def __init__(self):
        if settings.globalSettings.is_mi300():
            import mi300_commands as commands
        elif settings.globalSettings.is_nv32():
            import nv32_commands as commands
        else:
            raise Exception("Unsupported platform")
        if settings.globalSettings.is_host_machine():
            if settings.globalSettings.is_windows():
                self.commands = commands.host_windows_commands
                self.options = commands.host_windows_options
                self.vf_commands = commands.host_windows_commands_vf
                self.vf_options = commands.host_windows_options_vf
                self.csv_commands = commands.host_windows_commands_csv if len(commands.host_windows_commands_csv) > 0 else commands.host_windows_commands
                self.csv_options = commands.host_windows_options_csv if len(commands.host_windows_options_csv) > 0 else commands.host_windows_options
            else:
                self.commands = commands.host_linux_commands
                self.options = commands.host_linux_options
                self.vf_commands = commands.host_linux_commands_vf
                self.vf_options = commands.host_linux_options_vf
                self.csv_commands = commands.host_linux_commands_csv if len(commands.host_linux_commands_csv) > 0 else commands.host_linux_commands
                self.csv_options = commands.host_linux_options_csv if len(commands.host_linux_options_csv) > 0 else commands.host_linux_options
        elif settings.globalSettings.is_baremetal_machine():
            if settings.globalSettings.is_windows():
                self.commands = commands.baremetal_windows_commands
                self.options = commands.baremetal_windows_options
            else:
                self.commands = commands.baremetal_linux_commands
                self.options = commands.baremetal_linux_options
        else:
            if settings.globalSettings.is_windows():
                self.commands = commands.guest_commands
                self.options = commands.guest_options
            else:
                self.commands = commands.guest_linux_commands
                self.options = commands.guest_linux_options
                self.csv_commands = commands.guest_linux_commands_csv if len(commands.guest_linux_commands_csv) > 0 else commands.guest_linux_commands
                self.csv_options = commands.guest_linux_options_csv if len(commands.guest_linux_options_csv) > 0 else commands.guest_linux_options
    def getCommands(self):
        return self.commands
    def getOptions(self):
        return self.options
    def getCsvCommands(self):
        return self.csv_commands
    def getCsvOptions(self):
        return self.csv_options
    def getVfCommands(self):
        return self.vf_commands
    def getVfOptions(self):
        return self.vf_options
def initGlobalCommands():
    global globalCommands
    if globalCommands is None:
        globalCommands = Commands()