/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include "amdgv_nbio.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;


int amdgv_nbio_ih_doorbell_range(struct amdgv_adapter *adapt, bool use_doorbell,
				 int doorbell_index)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->ih_doorbell_range) {
		adapt->nbio.funcs->ih_doorbell_range(adapt, use_doorbell, doorbell_index);
	} else {
		AMDGV_ERROR("Unable to set ih doorbell range");
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_nbio_enable_doorbell_aperture(struct amdgv_adapter *adapt, bool enable)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->enable_doorbell_aperture) {
		adapt->nbio.funcs->enable_doorbell_aperture(adapt, enable);
	} else {
		AMDGV_ERROR("Unable to enable doorbell aperture");
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_nbio_enable_ih_interrupt(struct amdgv_adapter *adapt, bool use_bus_addr,
				   uint32_t ih_base_addr)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->enable_ih_interrupt) {
		adapt->nbio.funcs->enable_ih_interrupt(adapt, use_bus_addr, ih_base_addr);
	} else {
		AMDGV_ERROR("Unable to enable ih interrupt");
		return AMDGV_FAILURE;
	}

	return 0;
}

uint32_t amdgv_nbio_get_memsize(struct amdgv_adapter *adapt)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->get_memsize)
		return adapt->nbio.funcs->get_memsize(adapt);

	AMDGV_ERROR("Unable to get memsize");
	return 0;
}

int amdgv_nbio_gc_doorbell_init(struct amdgv_adapter *adapt)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->gc_doorbell_init) {
		adapt->nbio.funcs->gc_doorbell_init(adapt);
	} else {
		AMDGV_ERROR("GC doorbell init not implemented.\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_nbio_sdma_doorbell_range(struct amdgv_adapter *adapt, int instance, bool use_doorbell,
				   int doorbell_index, int doorbell_size)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->sdma_doorbell_range) {
		adapt->nbio.funcs->sdma_doorbell_range(adapt, instance, use_doorbell,
						       doorbell_index, doorbell_size);
	} else {
		AMDGV_ERROR("Unable to set sdma doorbell range");
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_nbio_get_nps_mode(struct amdgv_adapter *adapt,
			    enum amdgv_memory_partition_mode *memory_partition_mode)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->get_nps_mode) {
		return adapt->nbio.funcs->get_nps_mode(adapt, memory_partition_mode);
	} else {
		AMDGV_ERROR("Unable to get nps mode\n");
		return AMDGV_FAILURE;
	}
}

bool amdgv_nbio_is_partition_mode_supported(struct amdgv_adapter *adapt,
		enum amdgv_memory_partition_mode memory_partition_mode,
		enum amdgv_accelerator_partition_mode accelerator_partition_mode)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->is_partition_mode_supported) {
		return adapt->nbio.funcs->is_partition_mode_supported(adapt, memory_partition_mode,
								      accelerator_partition_mode);
	} else {
		AMDGV_ERROR("Unable verify partition mode combination\n");
		return false;
	}
}

int amdgv_nbio_get_supported_memory_partition_mode(struct amdgv_adapter *adapt,
						   enum amdgv_memory_partition_mode *supported_nps,
						   int* supported_nps_count)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->get_supported_memory_partition_mode) {
		return adapt->nbio.funcs->get_supported_memory_partition_mode(adapt, supported_nps,
									      supported_nps_count);
	} else {
		AMDGV_ERROR("Unable get supported memory partition modes\n");
		return AMDGV_FAILURE;
	}
}
enum amdgv_accelerator_partition_mode
amdgv_nbio_get_accel_partition_mode(struct amdgv_adapter *adapt)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->get_accel_partition_mode) {
		return adapt->nbio.funcs->get_accel_partition_mode(adapt);
	} else {
		AMDGV_ERROR("Unable get accel partition mode\n");
		return AMDGV_ACCELERATOR_PARTITION_MODE_UNKNOWN;
	}
}

enum amdgv_accelerator_partition_mode
amdgv_nbio_get_default_accel_partition_mode(struct amdgv_adapter *adapt,
					    enum amdgv_memory_partition_mode memory_partition_mode)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->get_default_accel_partition_mode) {
		return adapt->nbio.funcs->get_default_accel_partition_mode(adapt,
									   memory_partition_mode);
	} else {
		AMDGV_ERROR("Unable get default accel partition mode\n");
		return AMDGV_ACCELERATOR_PARTITION_MODE_UNKNOWN;
	}
}

int amdgv_nbio_get_asic_nps_caps(struct amdgv_adapter *adapt,
				 const struct amdgv_nps_compute_combination **combination)
{
	if (adapt->nbio.funcs && adapt->nbio.funcs->get_asic_nps_caps) {
		return adapt->nbio.funcs->get_asic_nps_caps(adapt, combination);
	} else {
		AMDGV_ERROR("Unable get asic NPS capabilities\n");
		return AMDGV_ACCELERATOR_PARTITION_MODE_UNKNOWN;
	}
}

