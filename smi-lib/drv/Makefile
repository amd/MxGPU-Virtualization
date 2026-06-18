# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


SMI_CORE_LOCAL = smi_drv_handshake.o smi_drv_cmd.o smi_drv_core.o smi_drv_utils.o
SMI_LINUX_LOCAL = smi_drv.o smi_drv_event.o

SMI_COMPILE_FILES = $(addprefix core/,$(SMI_CORE_LOCAL))
SMI_COMPILE_FILES += $(addprefix linux/,$(SMI_LINUX_LOCAL))