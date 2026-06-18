# Copyright Advanced Micro Devices, Inc.
#
# SPDX-License-Identifier: MIT


include ../make/linux/defines.mk

all:
	$(MAKE) -C monitor
	$(MAKE) -C event_monitor
	$(MAKE) -C ras_cper

.PHONY: clean
clean:
	$(MAKE) -C monitor clean
	$(MAKE) -C event_monitor clean
	$(MAKE) -C ras_cper clean
