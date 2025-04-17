#!/usr/bin/env python3

#
# Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

from enum import IntEnum
from . import amdsmi_wrapper


class AmdSmiRetCode(IntEnum):
    SUCCESS = amdsmi_wrapper.AMDSMI_STATUS_SUCCESS
    STATUS_INVAL = amdsmi_wrapper.AMDSMI_STATUS_INVAL
    STATUS_NOT_SUPPORTED = amdsmi_wrapper.AMDSMI_STATUS_NOT_SUPPORTED
    STATUS_NOT_YET_IMPLEMENTED = amdsmi_wrapper.AMDSMI_STATUS_NOT_YET_IMPLEMENTED
    STATUS_FAIL_LOAD_MODULE = amdsmi_wrapper.AMDSMI_STATUS_FAIL_LOAD_MODULE
    STATUS_FAIL_LOAD_SYMBOL = amdsmi_wrapper.AMDSMI_STATUS_FAIL_LOAD_SYMBOL
    STATUS_DRM_ERROR = amdsmi_wrapper.AMDSMI_STATUS_DRM_ERROR
    STATUS_API_FAILED = amdsmi_wrapper.AMDSMI_STATUS_API_FAILED
    STATUS_TIMEOUT = amdsmi_wrapper.AMDSMI_STATUS_TIMEOUT
    STATUS_RETRY = amdsmi_wrapper.AMDSMI_STATUS_RETRY
    STATUS_NO_PERM = amdsmi_wrapper.AMDSMI_STATUS_NO_PERM
    STATUS_INTERRUPT = amdsmi_wrapper.AMDSMI_STATUS_INTERRUPT
    STATUS_IO = amdsmi_wrapper.AMDSMI_STATUS_IO
    STATUS_ADDRESS_FAULT = amdsmi_wrapper.AMDSMI_STATUS_ADDRESS_FAULT
    STATUS_FILE_ERROR = amdsmi_wrapper.AMDSMI_STATUS_FILE_ERROR
    STATUS_OUT_OF_RESOURCES = amdsmi_wrapper.AMDSMI_STATUS_OUT_OF_RESOURCES
    STATUS_INTERNAL_EXCEPTION = amdsmi_wrapper.AMDSMI_STATUS_INTERNAL_EXCEPTION
    STATUS_INPUT_OUT_OF_BOUNDS = amdsmi_wrapper.AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS
    STATUS_INIT_ERROR = amdsmi_wrapper.AMDSMI_STATUS_INIT_ERROR
    STATUS_REFCOUNT_OVERFLOW = amdsmi_wrapper.AMDSMI_STATUS_REFCOUNT_OVERFLOW
    STATUS_BUSY = amdsmi_wrapper.AMDSMI_STATUS_BUSY
    STATUS_NOT_FOUND = amdsmi_wrapper.AMDSMI_STATUS_NOT_FOUND
    STATUS_NOT_INIT = amdsmi_wrapper.AMDSMI_STATUS_NOT_INIT
    STATUS_NO_SLOT = amdsmi_wrapper.AMDSMI_STATUS_NO_SLOT
    STATUS_DRIVER_NOT_LOADED = amdsmi_wrapper.AMDSMI_STATUS_DRIVER_NOT_LOADED
    STATUS_NO_DATA = amdsmi_wrapper.AMDSMI_STATUS_NO_DATA
    STATUS_INSUFFICIENT_SIZE = amdsmi_wrapper.AMDSMI_STATUS_INSUFFICIENT_SIZE
    STATUS_UNEXPECTED_SIZE = amdsmi_wrapper.AMDSMI_STATUS_UNEXPECTED_SIZE
    STATUS_UNEXPECTED_DATA = amdsmi_wrapper.AMDSMI_STATUS_UNEXPECTED_DATA
    STATUS_NON_AMD_CPU = amdsmi_wrapper.AMDSMI_STATUS_NON_AMD_CPU
    STATUS_NO_ENERGY_DRV = amdsmi_wrapper.AMDSMI_STATUS_NO_ENERGY_DRV
    STATUS_NO_MSR_DRV = amdsmi_wrapper.AMDSMI_STATUS_NO_MSR_DRV
    STATUS_NO_HSMP_DRV = amdsmi_wrapper.AMDSMI_STATUS_NO_HSMP_DRV
    STATUS_NO_HSMP_SUP = amdsmi_wrapper.AMDSMI_STATUS_NO_HSMP_SUP
    STATUS_NO_HSMP_MSG_SUP = amdsmi_wrapper.AMDSMI_STATUS_NO_HSMP_MSG_SUP
    STATUS_HSMP_TIMEOUT = amdsmi_wrapper.AMDSMI_STATUS_HSMP_TIMEOUT
    STATUS_NO_DRV = amdsmi_wrapper.AMDSMI_STATUS_NO_DRV
    STATUS_FILE_NOT_FOUND = amdsmi_wrapper.AMDSMI_STATUS_FILE_NOT_FOUND
    STATUS_ARG_PTR_NULL = amdsmi_wrapper.AMDSMI_STATUS_ARG_PTR_NULL
    STATUS_AMDGPU_RESTART_ERR = amdsmi_wrapper.AMDSMI_STATUS_AMDGPU_RESTART_ERR
    STATUS_SETTING_UNAVAILABLE = amdsmi_wrapper.AMDSMI_STATUS_SETTING_UNAVAILABLE
    STATUS_MAP_ERROR = amdsmi_wrapper.AMDSMI_STATUS_MAP_ERROR
    STATUS_UNKNOWN_ERROR = amdsmi_wrapper.AMDSMI_STATUS_UNKNOWN_ERROR


class AmdSmiException(Exception):
    '''Base amdsmi exception class'''
    pass


class AmdSmiLibraryException(AmdSmiException):
    def __init__(self, err_code):
        err_code = abs(err_code)
        super().__init__(err_code)
        self.err_code = err_code
        self.set_err_info()

    def __str__(self):
        return "An error occured with code: {err_code}({err_info})".format(
            err_code=self.err_code, err_info=self.err_info)

    def get_error_code(self):
        return self.err_code

    def set_err_info(self):
        switch = {
            AmdSmiRetCode.STATUS_INVAL: "AMDSMI_STATUS_INVAL - Invalid parameters",
            AmdSmiRetCode.STATUS_NOT_SUPPORTED: "AMDSMI_STATUS_NOT_SUPPORTED - Command not supported",
            AmdSmiRetCode.STATUS_NOT_YET_IMPLEMENTED: "AMDSMI_STATUS_NOT_YET_IMPLEMENTED - Not implemented yet",
            AmdSmiRetCode.STATUS_FAIL_LOAD_MODULE: "AMDSMI_STATUS_FAIL_LOAD_MODULE - Fail to load lib",
            AmdSmiRetCode.STATUS_FAIL_LOAD_SYMBOL: "AMDSMI_STATUS_FAIL_LOAD_SYMBOL - Fail to load symbol",
            AmdSmiRetCode.STATUS_DRM_ERROR: "AMDSMI_STATUS_DRM_ERROR - Error when call libdrm",
            AmdSmiRetCode.STATUS_API_FAILED: "AMDSMI_STATUS_API_FAILED - API call failed",
            AmdSmiRetCode.STATUS_TIMEOUT: "AMDSMI_STATUS_TIMEOUT - Timeout in API call",
            AmdSmiRetCode.STATUS_RETRY: "AMDSMI_STATUS_RETRY - Retry operation",
            AmdSmiRetCode.STATUS_NO_PERM: "AMDSMI_STATUS_NO_PERM - Permission Denied",
            AmdSmiRetCode.STATUS_INTERRUPT: "AMDSMI_STATUS_INTERRUPT - An interrupt occurred during execution of function",
            AmdSmiRetCode.STATUS_IO: "AMDSMI_STATUS_IO - I/O Error",
            AmdSmiRetCode.STATUS_ADDRESS_FAULT: "AMDSMI_STATUS_ADDRESS_FAULT - Bad address",
            AmdSmiRetCode.STATUS_FILE_ERROR: "AMDSMI_STATUS_FILE_ERROR - Problem accessing a file",
            AmdSmiRetCode.STATUS_OUT_OF_RESOURCES: "AMDSMI_STATUS_OUT_OF_RESOURCES - Not enough memory",
            AmdSmiRetCode.STATUS_INTERNAL_EXCEPTION: "AMDSMI_STATUS_INTERNAL_EXCEPTION - An internal exception was caught",
            AmdSmiRetCode.STATUS_INPUT_OUT_OF_BOUNDS: "AMDSMI_STATUS_INPUT_OUT_OF_BOUNDS - The provided input is out of allowable or safe range",
            AmdSmiRetCode.STATUS_INIT_ERROR: "AMDSMI_STATUS_INIT_ERROR - An error occurred when initializing internal data structures",
            AmdSmiRetCode.STATUS_REFCOUNT_OVERFLOW: "AMDSMI_STATUS_REFCOUNT_OVERFLOW - An internal reference counter exceeded INT32_MAX",
            AmdSmiRetCode.STATUS_BUSY: "AMDSMI_STATUS_BUSY - Processor busy",
            AmdSmiRetCode.STATUS_NOT_FOUND: "AMDSMI_STATUS_NOT_FOUND - Processor not found",
            AmdSmiRetCode.STATUS_NOT_INIT: "AMDSMI_STATUS_NOT_INIT - Processor not initialized",
            AmdSmiRetCode.STATUS_NO_SLOT: "AMDSMI_STATUS_NO_SLOT - No more free slot",
            AmdSmiRetCode.STATUS_DRIVER_NOT_LOADED: "AMDSMI_STATUS_DRIVER_NOT_LOADED - Processor driver not loaded",
            AmdSmiRetCode.STATUS_NO_DATA: "AMDSMI_STATUS_NO_DATA - No data was found for a given input",
            AmdSmiRetCode.STATUS_INSUFFICIENT_SIZE: "AMDSMI_STATUS_INSUFFICIENT_SIZE - Not enough resources were available for the operation",
            AmdSmiRetCode.STATUS_UNEXPECTED_SIZE: "AMDSMI_STATUS_UNEXPECTED_SIZE - An unexpected amount of data was read",
            AmdSmiRetCode.STATUS_UNEXPECTED_DATA: "AMDSMI_STATUS_UNEXPECTED_DATA - The data read or provided to function is not what was expected",
            AmdSmiRetCode.STATUS_NON_AMD_CPU: "AMDSMI_STATUS_NON_AMD_CPU - System has different cpu than AMD",
            AmdSmiRetCode.STATUS_NO_ENERGY_DRV: "AMDSMI_STATUS_NO_ENERGY_DRV - Energy driver not found",
            AmdSmiRetCode.STATUS_NO_MSR_DRV: "AMDSMI_STATUS_NO_MSR_DRV - MSR driver not found",
            AmdSmiRetCode.STATUS_NO_HSMP_DRV: "AMDSMI_STATUS_NO_HSMP_DRV - HSMP driver not found",
            AmdSmiRetCode.STATUS_NO_HSMP_SUP: "AMDSMI_STATUS_NO_HSMP_SUP - HSMP not supported",
            AmdSmiRetCode.STATUS_NO_HSMP_MSG_SUP: "AMDSMI_STATUS_NO_HSMP_MSG_SUP - HSMP message/feature not supported",
            AmdSmiRetCode.STATUS_HSMP_TIMEOUT: "AMDSMI_STATUS_HSMP_TIMEOUT - HSMP message is timedout",
            AmdSmiRetCode.STATUS_NO_DRV: "AMDSMI_STATUS_NO_DRV - No Energy and HSMP driver present",
            AmdSmiRetCode.STATUS_FILE_NOT_FOUND: "AMDSMI_STATUS_FILE_NOT_FOUND - File or directory not found",
            AmdSmiRetCode.STATUS_ARG_PTR_NULL: "AMDSMI_STATUS_ARG_PTR_NULL - Parsed argument is invalid",
            AmdSmiRetCode.STATUS_AMDGPU_RESTART_ERR: "AMDSMI_STATUS_AMDGPU_RESTART_ERR - AMDGPU restart failed",
            AmdSmiRetCode.STATUS_SETTING_UNAVAILABLE: "AMDSMI_STATUS_SETTING_UNAVAILABLE - Setting is not available",
            AmdSmiRetCode.STATUS_MAP_ERROR: "AMDSMI_STATUS_MAP_ERROR - The internal library error did not map to a status code",
        }

        self.err_info = switch.get(self.err_code, "AMDSMI_STATUS_UNKNOWN_ERROR - An unknown error occurred")


class AmdSmiRetryException(AmdSmiLibraryException):
    def __init__(self):
        super().__init__(AmdSmiRetCode.STATUS_RETRY)


class AmdSmiTimeoutException(AmdSmiLibraryException):
    def __init__(self):
        super().__init__(AmdSmiRetCode.STATUS_TIMEOUT)


class AmdSmiParameterException(AmdSmiException):
    def __init__(self, receivedValue, expectedType, msg=None):
        super().__init__(msg)
        self.actualType = type(receivedValue)
        self.expectedType = expectedType
        self.set_err_msg()
        if msg is not None:
            self.err_msg = msg

    def set_err_msg(self):
        self.err_msg = "Invalid parameter:\n" + "Actual type: {actualType}\n".format(
            actualType=self.actualType) + "Expected type: {expectedType}".format(
                expectedType=self.expectedType)

    def __str__(self):
        return self.err_msg


class AmdSmiKeyException(AmdSmiException):
    def __init__(self, key):
        super().__init__()
        self.key = key
        self.set_err_msg()

    def set_err_msg(self):
        self.err_msg = "Key " + self.key + " is missing from dictionary"

    def __str__(self):
        return self.err_msg


class AmdSmiBdfFormatException(AmdSmiException):
    def __init__(self, bdf):
        super().__init__()
        self.bdf = bdf

    def __str__(self):
        return ("Wrong BDF format: {}. \n" +
                "BDF string should be: <domain>:<bus>:<device>.<function>\n" +
                " or <bus>:<device>.<function> in hexcode format.\n" +
                "Where:\n\t<domain> is 4 hex digits long from 0000-FFFF interval\n" +
                "\t<bus> is 2 hex digits long from 00-FF interval\n" +
                "\t<device> is 2 hex digits long from 00-1F interval\n" +
                "\t<function> is 1 hex digit long from 0-7 interval"
                ).format(self.bdf)
