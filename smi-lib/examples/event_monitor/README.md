This example reads the events from the GPU. It has 2 input arguments. The first argument is the category and the second is severity level. Depending on these 2 arguments, the output of the program will be events with the specific category and severity level. If no arguments are given, it will print all events that happened on the GPU. Category can be a number from 0 to 14 which represents AMDSMI_EVENT_CATEGORY. If the given category is higher or equal than AMDSMI_EVENT_CATEGORY__MAX (15), all events will be included but only those with the highest severity. Severity level can have five values ranging from 0 to 4 where 0 represents events with the highest severity, 1 represents events with the medium severity, 2 represents events with the lowest severity, 3 represents events with the warning severity and 4 represents events with info (all) severity.
If any other value is given as the second input argument, only events with the lowest severity level will be logged.

Dependency: `libamdsmi.so`

Prerequisite: OS dynamic library search path has to contain the path where `libamdsmi.so` is located.

Usage: `event_monitor <category> <severity>`

* Input arguments:
	* category: event category. The event category can have the following values:
		* 0 = AMDSMI_EVENT_CATEGORY_NON_USED
		* 1 = AMDSMI_EVENT_CATEGORY_DRIVER
		* 2 = AMDSMI_EVENT_CATEGORY_RESET
		* 3 = AMDSMI_EVENT_CATEGORY_SCHED
		* 4 = AMDSMI_EVENT_CATEGORY_VBIOS
		* 5 = AMDSMI_EVENT_CATEGORY_ECC
		* 6 = AMDSMI_EVENT_CATEGORY_PP
		* 7 = AMDSMI_EVENT_CATEGORY_IOV
		* 8 = AMDSMI_EVENT_CATEGORY_VF
		* 9 = AMDSMI_EVENT_CATEGORY_UCODE
		* 10 = AMDSMI_EVENT_CATEGORY_GPU
		* 11 = AMDSMI_EVENT_CATEGORY_GUARD
		* 12 = AMDSMI_EVENT_CATEGORY_GPUMON
		* 13 = AMDSMI_EVENT_CATEGORY_MMSCH
		* 14 = AMDSMI_EVENT_CATEGORY_XGMI
		* 15 <= All categories
	* severity: severity level. The severity level can have the following values:
		* 0 = highest, critical error level
		* 1 = med, significant error level
		* 2 = low, trivial error level
		* 3 = warn, warning level
		* 4 = info, info level

Usage examples:

* If `libamdsmi.so` is installed to system path like `/lib` or `/usr/lib`, the following command is sufficient: `sudo ./event_monitor 2 1`
* Otherwise the command should contain the path to `libamdsmi.so`: `sudo LD_LIBRARY_PATH=/path/to/libamdsmi/library/ ./event_monitor 2 1`