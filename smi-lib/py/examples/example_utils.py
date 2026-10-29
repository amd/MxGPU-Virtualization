# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT
#

from amdsmi import AmdSmiLibraryException, AmdSmiRetCode


def prompt_for_input(prompt, default_value, lower_bound=0, upper_bound=None,
                     valid_values=None, error_message=None):
    def is_valid(value):
        return (
            value >= lower_bound
            and (upper_bound is None or value <= upper_bound)
            and (valid_values is None or value in valid_values)
        )

    if not is_valid(default_value):
        raise ValueError("Default input value must be valid")

    if error_message is None:
        if valid_values is not None:
            error_message = "  Input must be one of: {}".format(
                ", ".join(str(value) for value in valid_values))
        elif upper_bound is not None:
            error_message = "  Input must be between {} and {}".format(
                lower_bound, upper_bound)
        else:
            error_message = "  Input must be at least {}".format(lower_bound)

    while True:
        input_value = input("{}[default: {}] ".format(
            prompt, default_value)).strip()
        if input_value == "":
            return default_value

        try:
            value = int(input_value)
        except ValueError:
            value = None

        if value is not None and is_valid(value):
            return value

        print(error_message)


def prompt_confirmation(prompt):
    while True:
        confirmation = input("{} (YES/no) ".format(prompt)).strip().lower()
        if confirmation == "yes":
            return True
        if confirmation == "no":
            return False
        print("  Enter yes or no")


def print_optional_api_error(api_name, error):
    if error.err_code == AmdSmiRetCode.STATUS_NOT_SUPPORTED:
        print("  {} is not supported on this system".format(api_name))
    elif error.err_code == AmdSmiRetCode.STATUS_DRIVER_NOT_LOADED:
        print("  {} requires the appropriate device driver".format(api_name))
    elif error.err_code == AmdSmiRetCode.STATUS_NO_PERM:
        print("  {} requires permission to access the AMD SMI driver device".format(
            api_name))
        print("  From the smi-lib repository root, run:")
        print("  sudo python3 py/examples/<example>.py")
    elif error.err_code == AmdSmiRetCode.STATUS_NOT_FOUND:
        print("  {} found no matching processors".format(api_name))
    elif error.err_code == AmdSmiRetCode.STATUS_NO_DATA:
        print("  {} returned no data".format(api_name))
    elif error.err_code == AmdSmiRetCode.STATUS_INVAL:
        print("  {} received invalid parameters".format(api_name))
    else:
        print("  {} failed: {}".format(api_name, error))


def get_optional_api(api_name, api, *args):
    try:
        return api(*args)
    except AmdSmiLibraryException as e:
        print_optional_api_error(api_name, e)
        return None


def set_optional_api(api_name, api, *args):
    # Setters return nothing on success, so report the status code instead.
    try:
        api(*args)
    except AmdSmiLibraryException as e:
        print_optional_api_error(api_name, e)
        try:
            return AmdSmiRetCode(e.err_code)
        except ValueError:
            return AmdSmiRetCode.STATUS_UNKNOWN_ERROR
    return AmdSmiRetCode.SUCCESS
