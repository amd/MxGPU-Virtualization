/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv_mailbox.h"
#include "amdgv_oss_wrapper.h"
#include "amdgv_irqmgr.h"
#include "amdgv_guard.h"
#include "amdgv_log.h"
#include "amdgv_sched_internal.h"
#include "amdgv_ecc.h"
#include "amdgv_powerplay.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

#define IH_WB_BUF_SIZE 8

static void amdgv_ih_dump_irq_info(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry, bool is_detail)
{
	int i;

	AMDGV_INFO("IRQ src_id=0x%x client_id=0x%x pas_id=0x%x pasid_src=0x%x "
		    "ring_id=0x%x vm_id=0x%x vm_id_src=0x%x timestamp=0x%x timestamp_src=0x%x\n",
		    entry->src_id, entry->client_id, entry->pas_id, entry->pasid_src,
		    entry->ring_id, entry->vm_id, entry->vm_id_src, entry->timestamp, entry->timestamp_src);

	if (is_detail) {
		for (i = 0; i < AMDGV_IH_SRC_DATA_MAX_SIZE_DW; i++) {
			AMDGV_INFO("    IRQ src_data[%d]=0x%x\n", i, entry->src_data[i]);
		}
	}
}

/**
 * amdgv_ih_ring_init - initialize the IH state
 *
 * @adapt: amdgv_device pointer
 *
 * Initializes the IH state and allocates a buffer
 * for the IH ring buffer.
 * Returns 0 for success, errors for failure.
 */
int amdgv_ih_ring_init(struct amdgv_adapter *adapt, unsigned int ring_size, bool use_bus_addr)
{
	uint32_t rb_bufsz;
	uint32_t ih_buf_size, size;

	adapt->irqmgr.ih_submission_interrupt_context = NULL;
	adapt->irqmgr.ih_submission_interrupt_handler = NULL;

	/* Translate the RB size in Bytes to be log2 based DWORDs */
	rb_bufsz = order_base_2(ring_size / 4);

	/* Align ring size */
	ring_size = (1 << rb_bufsz) * 4;
	adapt->irqmgr.ih.ring_size = ring_size;
	adapt->irqmgr.ih.ring_size_log2 = rb_bufsz;
	adapt->irqmgr.ih.ptr_mask = adapt->irqmgr.ih.ring_size - 1;
	adapt->irqmgr.ih.rptr = 0;
	adapt->irqmgr.ih.use_bus_addr = use_bus_addr;
	adapt->irqmgr.ih.irq_processing = false;

	/* The IH buffer includes IH ring and wptr/rptr read back address */
	ih_buf_size = ring_size + IH_WB_BUF_SIZE;

	if (adapt->irqmgr.ih.use_bus_addr) {
		/* Allocate IV ring and WB inside system memory */
		struct oss_dma_mem_info dma_mem_info;
		int mem_type = OSS_DMA_MEM_CACHEABLE;

		/* add 8 bytes for the rptr/wptr shadows and
		 * add them to the end of the ring allocation.
		 */
		if (oss_alloc_dma_mem(adapt->dev, ih_buf_size, mem_type, &dma_mem_info) != 0) {
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_DMA_MEM_FAIL,
					ih_buf_size);
			return AMDGV_FAILURE;
		}

		adapt->irqmgr.ih.ring = dma_mem_info.va_ptr;
		adapt->irqmgr.ih.rb_dma_addr = dma_mem_info.bus_addr;
		adapt->irqmgr.ih.dma_mem_handle = dma_mem_info.handle;
	} else {
		/* allocate IV Ring and WB inside visible framebuffer
		   if the memory manager is enabled */
		if (adapt->memmgr_pf.is_init) {
			adapt->irqmgr.ih.fb_mem = amdgv_memmgr_alloc(
				&adapt->memmgr_pf, ih_buf_size, MEM_IRQMGR_IH_RING);
			if (!adapt->irqmgr.ih.fb_mem) {
				amdgv_put_log(AMDGV_PF_IDX,
						AMDGV_LOG_DRIVER_ALLOC_FB_MEM_FAIL,
						ih_buf_size);
				return AMDGV_FAILURE;
			}
		}
	}

	adapt->irqmgr.ih.use_bh = false;
	size = sizeof(struct amdgv_iv_entry) * AMDGV_IH_QUEUE_ENTRY_NUM;
	adapt->irqmgr.ih_queue = NULL;
	adapt->irqmgr.ih_queue_rptr = 0;
	adapt->irqmgr.ih_queue_wptr = 0;
	adapt->irqmgr.ih_bh.arg1 = NULL;
	adapt->irqmgr.ih_bh.arg2 = NULL;
	adapt->irqmgr.ih_bh.fn = amdgv_ih_process_handle3;
	adapt->irqmgr.ih_bh.context = adapt;

	/* init ih_bh before call and check handle after call */
	if (oss_bh_init(&adapt->irqmgr.ih_bh) &&
	    adapt->irqmgr.ih_bh.handle != OSS_INVALID_HANDLE) {
		adapt->irqmgr.ih_queue = (struct amdgv_iv_entry *)oss_zalloc(size);
		if (!adapt->irqmgr.ih_queue) {
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL, size);
			return AMDGV_FAILURE;
		}

		adapt->irqmgr.ih.use_bh = true;
	} else
		AMDGV_WARN("Not support bottom-half or bh_init failure, revert to legacy interrupt handler\n");

	return 0;
}

int amdgv_ih_ring_set(struct amdgv_adapter *adapt)
{
	if (!adapt->irqmgr.ih.use_bus_addr) {
		if (adapt->irqmgr.ih.fb_mem) {
			adapt->irqmgr.ih.gpu_addr =
				amdgv_memmgr_get_gpu_addr(adapt->irqmgr.ih.fb_mem);
			adapt->irqmgr.ih.ring =
				amdgv_memmgr_get_cpu_addr(adapt->irqmgr.ih.fb_mem);
		} else {
			/* If the memory manager is not used place IH ring at 0 FB */
			adapt->irqmgr.ih.gpu_addr = adapt->mc_fb_loc_addr;
			if (adapt->xgmi.phy_nodes_num > 1)
				adapt->irqmgr.ih.gpu_addr += adapt->xgmi.phy_node_id *
							     adapt->xgmi.node_segment_size;

			adapt->irqmgr.ih.ring = adapt->fb;
		}
	}

	if (!adapt->irqmgr.ih.ring) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_IOV_IH_RING_NOT_ALLOCATED, 0);
		return AMDGV_FAILURE;
	}
	if (!amdgv_in_live_update_seq())
		oss_memset((void *)adapt->irqmgr.ih.ring, 0, adapt->irqmgr.ih.ring_size + 8);

	adapt->irqmgr.ih.wptr_offs = (adapt->irqmgr.ih.ring_size / 4) + 0;
	adapt->irqmgr.ih.rptr_offs = (adapt->irqmgr.ih.ring_size / 4) + 1;

	return 0;
}

/**
 * amdgv_ih_ring_fini - tear down the IH state
 *
 * @adapt: amdgv_device pointer
 *
 * Tears down the IH state and frees buffer
 * used for the IH ring buffer.
 */
void amdgv_ih_ring_fini(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.ih_bh.handle != OSS_INVALID_HANDLE) {
		/* free handle */
		oss_bh_fini(&adapt->irqmgr.ih_bh);
		/* re-init ih_bh */
		oss_memset(&adapt->irqmgr.ih_bh, 0, sizeof(struct oss_bh_info));
	}

	if (adapt->irqmgr.ih_queue) {
		oss_free(adapt->irqmgr.ih_queue);
		adapt->irqmgr.ih_queue = NULL;
	}

	adapt->irqmgr.ih_queue_rptr = 0;
	adapt->irqmgr.ih_queue_wptr = 0;

	if (adapt->irqmgr.ih.use_bus_addr) {
		if (adapt->irqmgr.ih.ring) {
			oss_free_dma_mem(adapt->irqmgr.ih.dma_mem_handle);
			adapt->irqmgr.ih.ring = NULL;
		}
	} else {
		if (adapt->irqmgr.ih.fb_mem) {
			amdgv_memmgr_free(adapt->irqmgr.ih.fb_mem);
			adapt->irqmgr.ih.fb_mem = NULL;
		}
		adapt->irqmgr.ih.gpu_addr = 0;
		adapt->irqmgr.ih.ring = NULL;
	}
}

int amdgv_ih_iv_ring_entry_process(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	const struct amdgv_pp_funcs *pp_func = adapt->pp.pp_funcs;
	int ret = 0, handled = 0;
	uint32_t idx_vf = 0;
	uint32_t hw_sched_id;
	uint32_t event = 0, msg_data[MAILBOX_DATA_LEN_4] = { 0 };
	enum amdgv_sched_event_id sched_event;
	union amdgv_sched_event_data event_data;
	oss_memset(&event_data, 0, sizeof(union amdgv_sched_event_data));

	switch (entry->client_id) {
	case IH_IV_CLIENTID_MP1:
		if (pp_func && pp_func->handle_smu_irq) {
			ret = pp_func->handle_smu_irq(adapt, entry);
			handled = 1;
		}
		break;
	default:
		break;
	}

	if (handled || ret)
		goto err_out;

	switch (entry->src_id) {
	case IH_IV_SRCID_BIF_VF_PF_MSGBUF_VALID:
		AMDGV_DEBUG("VF_PF_MSGBUF_VALID received\n");
		idx_vf = entry->src_data[0];
		if (idx_vf < adapt->num_vf) {
			if (amdgv_guard_add_active_event(adapt, idx_vf,
							 AMDGV_GUARD_EVENT_ALL_INT) ==
			    AMDGV_EVENT_OVERFLOW)
				break;

			/* receive and ack VF message */
			amdgv_mailbox_receive_msg(adapt, idx_vf, msg_data, MAILBOX_DATA_LEN_4,
						  true);
			event = msg_data[0];

			AMDGV_DEBUG("Received %s request from %s\n",
				    amdgv_mailbox_rcv_idh_to_name(event),
				    amdgv_idx_to_str(idx_vf));

			sched_event = amdgv_mailbox_get_valid_vf_event(adapt, event);
			if (sched_event == AMDGV_EVENT_INVALID_EVENT) {
				amdgv_put_log(idx_vf, AMDGV_LOG_IOV_MB_INVALID_VF_EVENT, (uint64_t)event);
				break;
			}

			if (sched_event == AMDGV_EVENT_SCHED_PSP_VF_CMD_RELAY) {
				/* Drop request if VF relay value is out of range or if VF relay is unsupported */
				if (!adapt->psp.vf_relay_wtr_ptr_max ||
				    msg_data[1] >= adapt->psp.vf_relay_wtr_ptr_max) {
					amdgv_put_log(idx_vf, AMDGV_LOG_IOV_PSP_VF_CMD_RELAY_OUT_OF_RANGE, AMDGV_LOG_DATA_32_32(msg_data[1], adapt->psp.vf_relay_wtr_ptr_max));
					break;
				}
				adapt->psp.vf_relay_wtr_ptr[idx_vf] = msg_data[1];
			}

			if (sched_event == AMDGV_EVENT_TEXT_MESSAGE) {
				char *buf = (char *)&msg_data[2];
				int j;

				/* guarantee the buffer terminated with '\0' */
				buf[7] = '\0';
				for (j = 0; j < 7; j++) {
					if (buf[j] == '\0')
						break;
					// replace non-printable characters
					if (buf[j] < ' ' || buf[j] > '~')
						buf[j] = '.';
				}

				AMDGV_INFO("%s driver message: %s\n", amdgv_idx_to_str(idx_vf),
					   buf);

			} else if (sched_event == AMDGV_EVENT_LOG_VF_ERROR) {
				uint32_t data1;
				uint64_t data2;
				uint16_t code;

				data1 = msg_data[1];
				code = AMDGV_LOG_CODE_FROM_MAILBOX(data1);

				if (!amdgv_log_is_valid_vf_code(code))
					break;

				data2 = msg_data[3];
				data2 <<= 32;
				data2 |= msg_data[2];

				amdgv_put_log(idx_vf, code, data2);
			/* else if in a row since sometimes we need to break from switch */
			} else if (sched_event == AMDGV_EVENT_READY_TO_RESET) {
				adapt->array_vf[idx_vf].ready_to_reset = true;
				for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
					if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode == AMDGV_SCHED_HYBRID_LIQUID_MODE)
						amdgv_gpuiov_ctx_empty_intr_control(adapt, hw_sched_id, false);
				}
			} else if (sched_event == AMDGV_EVENT_SCHED_VF_REQ_RAS_CPER_DUMP) {
				event_data.cper_vf.rptr = ((uint64_t)msg_data[1] << 32) |
							   msg_data[2];
				amdgv_sched_queue_event_ex(adapt, idx_vf, sched_event,
							   AMDGV_SCHED_BLOCK_ALL, event_data);
			} else if (sched_event == AMDGV_EVENT_SCHED_VF_REQ_RAS_BAD_PAGES) {
				amdgv_sched_queue_event_ex(adapt, idx_vf, sched_event,
							   AMDGV_SCHED_BLOCK_ALL, event_data);
			} else if (sched_event == AMDGV_EVENT_SCHED_VF_RAS_REMOTE_CMD) {
				if (amdgv_guard_add_active_event(adapt, idx_vf,
					AMDGV_GUARD_EVENT_RAS_REMOTE_CMD) == AMDGV_EVENT_OVERFLOW)
					break;

				event_data.remote_ras.gpa_addr = ((uint64_t)msg_data[2] << 32) | msg_data[1];
				event_data.remote_ras.gpa_size = msg_data[3];
				amdgv_sched_queue_event_ex(adapt, idx_vf, sched_event,
								AMDGV_SCHED_BLOCK_ALL, event_data);
			} else if (sched_event == AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION) {
				event_data.poison.consumption.block = msg_data[1];
				amdgv_put_log(idx_vf, AMDGV_LOG_ECC_POISON_CONSUMPTION,
						AMDGV_LOG_DATA_32_32(idx_vf, msg_data[1]));
				amdgv_sched_queue_event_ex(adapt, idx_vf, sched_event,
							AMDGV_SCHED_BLOCK_ALL, event_data);
			} else if (sched_event == AMDGV_EVENT_REQ_GPU_INIT_DATA) {
				uint32_t guest_crit_region_ver = msg_data[2];

				adapt->array_vf[idx_vf].xchg.guest_gpu_init_flags = msg_data[1];

				if (guest_crit_region_ver >= GPU_CRIT_REGION_MAX) {
					amdgv_put_log(idx_vf, AMDGV_LOG_IOV_UNSUPPORTED_CRIT_REGION_VER, AMDGV_LOG_DATA_32_32(guest_crit_region_ver, GPU_CRIT_REGION_MAX - 1));
					guest_crit_region_ver = GPU_CRIT_REGION_MAX - 1;
				}

				/* CRIT_REGION_VERSION enums start with 0, however the
                 * crit_region_caps expect bit 0 to be V1, need to shift
                 * all bits by 1.
				 * Guest sends the highest supported crit region version.
				 * Set caps bits for that version and all lower versions,
				 * since supporting VN implies supporting V1..VN-1. */
				adapt->array_vf[idx_vf].xchg.guest_crit_region_caps =
					(guest_crit_region_ver == 0 || guest_crit_region_ver > 31) ?  BIT(0) :
					(BIT(guest_crit_region_ver) - 1);
				amdgv_sched_queue_event_ex(adapt, idx_vf, sched_event,
							AMDGV_SCHED_BLOCK_ALL, event_data);
			} else if (sched_event == AMDGV_EVENT_SCHED_VF_REQ_RAS_CHK_CRITI_REGION) {
				event_data.chk_criti.addr = ((uint64_t)msg_data[1] << 32) |
							    msg_data[2];
				amdgv_sched_queue_event_ex(adapt, idx_vf, sched_event,
							   AMDGV_SCHED_BLOCK_ALL, event_data);
			} else if (sched_event == AMDGV_EVENT_VF_REQ_PTL_UPDATE) {
				/* Parse PTL request: req_code, ptl_state, packed formats */
				event_data.ptl.req_code = msg_data[1];
				event_data.ptl.ptl_state = msg_data[2];
				event_data.ptl.pref_format1 = AMD_SRIOV_PTL_UNPACK_FMT1(msg_data[3]);
				event_data.ptl.pref_format2 = AMD_SRIOV_PTL_UNPACK_FMT2(msg_data[3]);
				amdgv_sched_queue_event_ex(adapt, idx_vf, sched_event,
							   AMDGV_SCHED_BLOCK_ALL, event_data);
			} else if (sched_event == AMDGV_EVENT_SCHED_VF_REQ_GPU_INIT_XCHG_REGION) {
				event_data.vf_xchg_region.gpa_base = ((uint64_t)msg_data[1] << 32) |
								      msg_data[2];
				event_data.vf_xchg_region.size = msg_data[3];
				amdgv_sched_queue_event_ex(adapt, idx_vf, sched_event,
							   AMDGV_SCHED_BLOCK_ALL, event_data);
			} else {
				/* queue event to scheduler */
				amdgv_sched_queue_event(adapt, idx_vf, sched_event,
							AMDGV_SCHED_BLOCK_ALL);

				for_each_id(hw_sched_id, amdgv_sched_get_hw_sched_mask_by_vf(adapt, idx_vf)) {
					if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode == AMDGV_SCHED_HYBRID_LIQUID_MODE)
						amdgv_gpuiov_ctx_empty_intr_control(adapt, hw_sched_id, false);
				}
			}
		}
		break;

	case IH_IV_SRCID_BIF_PF_VF_MSGBUF_ACK:
		AMDGV_DEBUG("PF_VF_MSGBUF_ACK received\n");

		idx_vf = entry->src_data[0];
		if (idx_vf < adapt->num_vf) {
			if (amdgv_guard_add_active_event(adapt, idx_vf,
							 AMDGV_GUARD_EVENT_ALL_INT) ==
			    AMDGV_EVENT_OVERFLOW)
				break;
			/* clear msg buffer to VF */
			amdgv_mailbox_clear_valid_msg(adapt, idx_vf);
		}
		break;

	case IH_IV_SRCID_DF_MCA_INT:
		idx_vf = entry->src_data[0];
		AMDGV_INFO("ECC Uncorrectable Error Interrupt Received\n");
		amdgv_ih_dump_irq_info(adapt, entry, false);
		amdgv_sched_queue_event(adapt, idx_vf, AMDGV_EVENT_SCHED_RAS_UMC,
					AMDGV_SCHED_BLOCK_ALL);
		break;

	case IH_IV_SRCID_DF:
		/* RAS DF INT process */
		if (entry->client_id == IH_IV_CLIENTID_DF) {
			if (amdgv_uniras_enabled(adapt)) {
				amdgv_ras_mgr_handle_controller_interrupt(adapt, entry);
			} else {
				if (amdgv_ras_is_poison_mode_supported(adapt)) {
					amdgv_sched_queue_event_ex(adapt, AMDGV_PF_IDX,
								AMDGV_EVENT_SCHED_RAS_POISON_CREATION,
								AMDGV_SCHED_BLOCK_ALL, event_data);
				} else {
					idx_vf = entry->src_data[0];
					AMDGV_INFO("ECC RAS Error Interrupt Received\n");
					amdgv_ih_dump_irq_info(adapt, entry, false);
					amdgv_sched_queue_event(adapt, idx_vf,
								AMDGV_EVENT_SCHED_RAS_UMC,
								AMDGV_SCHED_BLOCK_ALL);
				}
			}
		} else if ((entry->client_id == IH_IV_CLIENTID_UTCL2 ||
			    entry->client_id == IH_IV_CLIENTID_VMC ||
			    entry->client_id == IH_IV_CLIENTID_GFX) &&
			   adapt->irqmgr.ih_funcs->handle_page_fault) {
			adapt->irqmgr.ih_funcs->handle_page_fault(adapt, entry);
		} else {
			AMDGV_ERROR("Unknown/Unhandled DF Interrupt Received\n");
			amdgv_ih_dump_irq_info(adapt, entry, true);
		}

		break;

	case IH_IV_SRCID_RLC_GC_FED_INTERRUPT:
		if (entry->client_id == IH_IV_CLIENTID_GFX) {
			event_data.fed_data.src = AMDGV_FED_SRC_GC_RLC;
			event_data.fed_data.data.u32_all = entry->src_data[0];
			AMDGV_WARN("RAS GC poison consumption interrupt received\n");
			amdgv_sched_queue_event_ex(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_RAS_FED,
						AMDGV_SCHED_BLOCK_ALL, event_data);
		} else {
			amdgv_ih_dump_irq_info(adapt, entry, false);
		}
		break;

	case IH_IV_SRCID_MP0_IH_SW_INT:
		AMDGV_INFO("MP0 IH SW INT received\n");
		if (entry->client_id == IH_IV_CLIENTID_MP0) {
			amdgv_ih_dump_irq_info(adapt, entry, false);
			if (adapt->psp.handle_irq)
				adapt->psp.handle_irq(adapt, entry);
			else
				AMDGV_ERROR("PSP: handle_irq is not supported\n");
		} else {
			AMDGV_ERROR("Unknown/Unhandled MP0 IH SW INT Received\n");
			amdgv_ih_dump_irq_info(adapt, entry, true);
		}
		break;

	case IH_IV_SRCID_RLC_GC_FED_COOKIE:
		if (entry->client_id == IH_IV_CLIENTID_RLC) {
			/*
			 * IH_COOKIE_7 = entry->src_data[3] = ContextID[127:96]
			 * When route_to_pf==1 (always true when host receives
			 * this), IH inserts {VF,2'b0,VFID[4:0]} into
			 * ContextID[127:120].
			 */
			uint32_t ih_cookie_7 = entry->src_data[3];
			bool is_vf = (ih_cookie_7 >> 7) & 0x1;
			uint8_t src_vfid = ih_cookie_7 & 0x1F;

			if (!is_vf || src_vfid >= adapt->num_vf) {
				AMDGV_ERROR("Invalid VF received from RLC poison IH cookie: is_vf=%d vfid=%u\n",
					is_vf, src_vfid);
				amdgv_ih_dump_irq_info(adapt, entry, true);
				break;
			}

			idx_vf = src_vfid;
			event_data.poison.consumption.block = AMDGV_RAS_BLOCK__GFX;

			amdgv_put_log(idx_vf, AMDGV_LOG_ECC_POISON_CONSUMPTION,
					AMDGV_LOG_DATA_32_32(idx_vf, AMDGV_RAS_BLOCK__GFX));

			amdgv_sched_queue_event_ex(adapt, idx_vf,
						AMDGV_EVENT_SCHED_RAS_POISON_CONSUMPTION,
						AMDGV_SCHED_BLOCK_ALL, event_data);
		} else {
			AMDGV_ERROR("Unexpected client_id 0x%x for RLC poison IH cookie\n",
				    entry->client_id);
			amdgv_ih_dump_irq_info(adapt, entry, true);
		}
		break;

	default:
		/* Unhandled hardware-delivered src_id - developer diagnostic only, not a
		 * SHIM-visible error. Keep as a raw DEBUG paired with the IH dump. */
		AMDGV_DEBUG("Unhandled IH src_id 0x%x\n", entry->src_id);
		amdgv_ih_dump_irq_info(adapt, entry, true);
		break;
	}

err_out:
	/* Add IRQ entry to the diagnosis data trace log */
	AMDGV_DIAG_DATA_TRACE_LOG_IRQ(entry->client_id, entry->src_id, entry->vm_id, event, idx_vf);

	return ret;
}

int amdgv_irqmgr_iv_ring_enable(struct amdgv_adapter *adapt, bool enable)
{
	return adapt->irqmgr.ih_funcs->enable_iv_ring(adapt, enable);
}

int amdgv_irqmgr_mbox_enable(struct amdgv_adapter *adapt, bool enable)
{
	adapt->irqmgr.ih_funcs->enable_mbox(adapt, enable);

	return 0;
}

void amdgv_irqmgr_register_submission_interrupt_handler(struct amdgv_adapter *adapt, void *handler, void *context)
{
	adapt->irqmgr.ih_submission_interrupt_context = context;
	adapt->irqmgr.ih_submission_interrupt_handler = handler;
}

void amdgv_irqmgr_register_gpu_timer_handler(struct amdgv_adapter *adapt, void *handler, void *context)
{
	adapt->irqmgr.ih_gpu_timer_context = context;
	adapt->irqmgr.ih_gpu_timer_handler = handler;
}

void amdgv_irqmgr_decode_iv(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry)
{
	/* wptr/rptr are in bytes! */
	uint32_t ring_index = adapt->irqmgr.ih.rptr >> 2;
	uint32_t dw[8];

	dw[0] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 0]);
	dw[1] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 1]);
	dw[2] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 2]);
	dw[3] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 3]);
	dw[4] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 4]);
	dw[5] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 5]);
	dw[6] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 6]);
	dw[7] = le32_to_cpu(adapt->irqmgr.ih.ring[ring_index + 7]);

	entry->client_id = dw[0] & 0xff;
	entry->src_id = (dw[0] >> 8) & 0xff;
	entry->ring_id = (dw[0] >> 16) & 0xff;
	entry->vm_id = (dw[0] >> 24) & 0xf;
	entry->vm_id_src = (dw[0] >> 31);
	entry->timestamp = dw[1] | ((uint64_t)(dw[2] & 0xffff) << 32);
	entry->timestamp_src = dw[2] >> 31;
	entry->pas_id = dw[3] & 0xffff;
	entry->node_id = (dw[3] >> 16) & 0xff;
	entry->pasid_src = dw[3] >> 31;
	entry->src_data[0] = dw[4];
	entry->src_data[1] = dw[5];
	entry->src_data[2] = dw[6];
	entry->src_data[3] = dw[7];

	/* wptr/rptr are in bytes! */
	adapt->irqmgr.ih.rptr += 32;
}

/* iv ring interrupt handle */
static enum oss_irq_return _amdgv_ih_process_handle(struct amdgv_adapter *adapt)
{
	struct amdgv_iv_entry entry;
	uint32_t wptr;
	uint32_t ring_index;

	if (!adapt->irqmgr.ih.enabled)
		return OSS_IRQ_NONE;
	oss_spin_lock(adapt->irqmgr.ih_event_lock);
	adapt->irqmgr.ih.irq_processing = true;
	wptr = adapt->irqmgr.ih_funcs->get_wptr(adapt);

	while (wptr != adapt->irqmgr.ih.rptr) {
		AMDGV_DEBUG("rptr 0x%x, wptr 0x%x\n", adapt->irqmgr.ih.rptr, wptr);

		oss_memory_fence();

		while (adapt->irqmgr.ih.rptr != wptr) {
			/* If interrupt handler gets disabled,
			 * stop processing req and exit.
			 */
			if (!adapt->irqmgr.ih.enabled)
				goto finish;

			ring_index = adapt->irqmgr.ih.rptr >> 2;

			entry.iv_entry = (const uint32_t *)&adapt->irqmgr.ih.ring[ring_index];

			adapt->irqmgr.ih_funcs->decode_iv(adapt, &entry);
			adapt->irqmgr.ih.rptr &= adapt->irqmgr.ih.ptr_mask;

			/* queue to bottom-half to handle if it supports bh */
			if (adapt->irqmgr.ih.use_bh) {
				if (((adapt->irqmgr.ih_queue_wptr + 1) %
				 AMDGV_IH_QUEUE_ENTRY_NUM) == adapt->irqmgr.ih_queue_rptr)
					amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_IOV_IH_QUEUE_FULL, 0);
				else {
					/* ih entry has been decoded */
					entry.iv_entry = NULL;
					oss_memcpy(
					 &adapt->irqmgr.ih_queue[adapt->irqmgr.ih_queue_wptr],
					 &entry,
					 sizeof(struct amdgv_iv_entry));
					/* no spinlock needs since on host only one thread */
					adapt->irqmgr.ih_queue_wptr =
					 ((adapt->irqmgr.ih_queue_wptr + 1) %
					   AMDGV_IH_QUEUE_ENTRY_NUM);
					/* can change ih_bh.arg 1&2 here if needs */
					oss_bh_queue(&adapt->irqmgr.ih_bh);
				}
			} else
				adapt->irqmgr.ih_funcs->process(adapt, &entry);
		}

		adapt->irqmgr.ih_funcs->set_rptr(adapt);

		/* make sure wptr hasn't changed while processing */
		wptr = adapt->irqmgr.ih_funcs->get_wptr(adapt);
	}
finish:
	adapt->irqmgr.ih.irq_processing = false;
	oss_spin_unlock(adapt->irqmgr.ih_event_lock);

	return OSS_IRQ_HANDLED;
}

/* oss ih interrupt callback type 1 */
enum oss_irq_return amdgv_ih_process_handle(void *context)
{
	return _amdgv_ih_process_handle((struct amdgv_adapter *)context);
}

/* oss ih interrupt callback type 2 */
enum oss_irq_return amdgv_ih_process_handle2(void *context,
					     struct oss_interrupt_cb_info *cb_info)
{
	int i;
	struct amdgv_iv_entry entry;
	struct amdgv_adapter *adapt;

	adapt = context;
	oss_spin_lock(adapt->irqmgr.ih_event_lock);
	/* convert interrupt callback info to iv entry */
	entry.client_id = cb_info->client_id;
	entry.src_id = cb_info->src_id;
	entry.ring_id = cb_info->ring_id;
	entry.vm_id = cb_info->vm_id;
	entry.vm_id_src = cb_info->vm_id_src;
	entry.timestamp = cb_info->timestamp;
	entry.timestamp_src = cb_info->timestamp_src;
	entry.pas_id = cb_info->pas_id;
	entry.pasid_src = cb_info->pasid_src;
	entry.iv_entry = NULL;

	for (i = 0; i < AMDGV_IH_SRC_DATA_MAX_SIZE_DW; i++)
		entry.src_data[i] = cb_info->src_data[i];

	adapt->irqmgr.ih_funcs->process((struct amdgv_adapter *)context, &entry);
	oss_spin_unlock(adapt->irqmgr.ih_event_lock);
	return OSS_IRQ_HANDLED;
}

/* oss ih interrupt callback type 3 */
void amdgv_ih_process_handle3(void *handle, void *context, void *arg1, void *arg2)
{
	struct amdgv_iv_entry *entry;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;
	/* if needs arg1&arg2, we can query adapt->irqmgr.ih_bh */
	if (adapt) {
		oss_spin_lock(adapt->irqmgr.ih_handler3_lock);
		while (adapt->irqmgr.ih_queue_rptr != adapt->irqmgr.ih_queue_wptr) {
			entry = &adapt->irqmgr.ih_queue[adapt->irqmgr.ih_queue_rptr];
			adapt->irqmgr.ih_funcs->process((struct amdgv_adapter *)context, entry);
			adapt->irqmgr.ih_queue_rptr =
			(adapt->irqmgr.ih_queue_rptr + 1) % AMDGV_IH_QUEUE_ENTRY_NUM;
		}
		oss_spin_unlock(adapt->irqmgr.ih_handler3_lock);
	}
}

/* oss ih interrupt callback type 2 */
enum oss_irq_return amdgv_hv_event_handle(void *context)
{
	struct amdgv_adapter *adapt;

	adapt = context;
	return adapt->irqmgr.hv_event_process(adapt);
}

enum oss_irq_return amdgv_ras_fatal_error_handle(void *context)
{
	struct amdgv_adapter *adapt = context;

	if (adapt->status == AMDGV_STATUS_HW_INIT) {
		if (amdgv_uniras_enabled(adapt)) {
			amdgv_ras_mgr_handle_fatal_interrupt(adapt, NULL);
		} else {
			if (adapt->nbio.ras) {
				if (adapt->nbio.ras->handle_ras_controller_intr_no_bifring)
					adapt->nbio.ras->handle_ras_controller_intr_no_bifring(adapt);
				if (adapt->nbio.ras->handle_ras_err_event_athub_intr_no_bifring)
					adapt->nbio.ras->handle_ras_err_event_athub_intr_no_bifring(adapt);
			}
		}
	}

	return OSS_IRQ_HANDLED;
}

int amdgv_irqmgr_toggle_interrupt(struct amdgv_adapter *adapt, bool enable)
{
	int ret = 0;
	uint32_t idx_vf;
	struct amdgv_wait_for_cb_context cb_context = { 0 };

	if (enable && adapt->irqmgr.ih_funcs->re_arm_vf_ih_ring) {
		// Re-arm ih ring on active vf
		for (idx_vf = 0; idx_vf < adapt->max_num_vf; idx_vf++) {
			if (is_active_vf(idx_vf)) {
				adapt->irqmgr.ih_funcs->re_arm_vf_ih_ring(adapt, idx_vf);
			}
		}
	}

	// Toggle SMU DISP_TIMER2
	if (adapt->irqmgr.ih_funcs->toggle_disp_timer2) {
		adapt->irqmgr.ih_funcs->toggle_disp_timer2(adapt, enable);
	}

	// Register mailbox interrupt with OS
	if (enable) {
		AMDGV_DEBUG("Register interrupt.\n");
		adapt->irqmgr.ih.enabled = true;
		ret = amdgv_irqmgr_register_interrupt(adapt);
	} else {
		AMDGV_DEBUG("Unregister interrupt.\n");
		adapt->irqmgr.ih.enabled = false;

		cb_context.ctx = (void *)adapt;
		cb_context.type = AMDGV_WAIT_FOR_IRQ_HANDLER;

		/* make sure irq handler exit */
		ret = amdgv_wait_for(adapt, amdgv_wait_for_irq_handler, &cb_context, ~0, 0);

		ret = amdgv_irqmgr_unregister_interrupt(adapt);
	}

	return ret;
}

void amdgv_irqmgr_dump_temperature(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct amdgv_gpumon_temp temp;

	oss_memset(&temp, 0, sizeof(temp));
	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_all_temperature) {
		ret = adapt->gpumon.funcs->get_all_temperature(adapt, &temp);
		if (!ret) {
			AMDGV_INFO("Dump all temperature checkpoints:\n");
			AMDGV_INFO("	temp_edge|asic = %d\n", temp.temp_edge);
			AMDGV_INFO("	temp_mem = %d\n", temp.temp_mem);
			AMDGV_INFO("	temp_hotspot = %d\n", temp.temp_hotspot);
			AMDGV_INFO("	temp_plx = %d\n", temp.temp_plx);
			AMDGV_INFO("	temp_multi, Max temp = %u | CTF temp = %u\n",
				   temp.temp_multi & 0x1ff, 0x1ff & (temp.temp_multi >> 9));
		}
	}
}

int amdgv_irqmgr_register_interrupt(struct amdgv_adapter *adapt)
{
	struct oss_intr_regrt_entry *intr_entries;
	struct oss_intr_regrt_info *intr_regrt_info;

	/* Use specific callback func if exists */
	if (adapt->irqmgr.ih_funcs->register_interrupt)
		return adapt->irqmgr.ih_funcs->register_interrupt(adapt);

	intr_regrt_info = oss_malloc(sizeof(struct oss_intr_regrt_info));
	if (intr_regrt_info == NULL) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct oss_intr_regrt_info));
		return AMDGV_FAILURE;
	}

	intr_entries = oss_malloc(sizeof(struct oss_intr_regrt_entry) * 4);
	if (intr_entries == NULL) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
				sizeof(struct oss_intr_regrt_entry) * 4);
		oss_free(intr_regrt_info);
		return AMDGV_FAILURE;
	}

	/* register MSI vector 0 interrupt handler */
	intr_entries[0].idx_msi_vector = 0;

	if (!adapt->irqmgr.disable_parse_ih) {
		intr_entries[0].intr_cb_type = OSS_INTR_CB_REGULAR;
		intr_entries[0].int_cb.interrupt_cb = amdgv_ih_process_handle;
		intr_entries[0].context = (void *)adapt;
	} else {
		/* interrupt callback parameters contains decoded IH entry */
		intr_entries[0].intr_cb_type = OSS_INTR_CB_DECODED;
		intr_entries[0].int_cb.interrupt_cb2 = amdgv_ih_process_handle2;
		intr_entries[0].context = (void *)adapt;
	}

	/* register MSI vector 1 interrupt handler */
	intr_entries[1].idx_msi_vector = 1;
	intr_entries[1].intr_cb_type = OSS_INTR_CB_REGULAR;
	intr_entries[1].int_cb.interrupt_cb = amdgv_hv_event_handle;
	intr_entries[1].context = (void *)adapt;

	/* register MSI vector 2 interrupt handler */
	intr_entries[2].idx_msi_vector = 2;
	intr_entries[2].intr_cb_type = OSS_INTR_CB_REGULAR;
	intr_entries[2].int_cb.interrupt_cb = amdgv_hv_event_handle;
	intr_entries[2].context = (void *)adapt;

	/* register MSI vector 3 interrupt handler */
	intr_entries[3].idx_msi_vector = 3;
	intr_entries[3].intr_cb_type = OSS_INTR_CB_REGULAR;
	intr_entries[3].int_cb.interrupt_cb = amdgv_ras_fatal_error_handle;
	intr_entries[3].context = (void *)adapt;

	intr_regrt_info->intr_type = OSS_INTR_TYPE_MSIX;
	intr_regrt_info->num_msi_vectors = 4;
	intr_regrt_info->num_intr_entry = 4;
	intr_regrt_info->intr_entries = intr_entries;

	/* register interrupt handler to OS */
	if (oss_register_interrupt(adapt->dev, intr_regrt_info) != 0) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_REGISTER_INTERRUPT_FAIL, 0);
		goto fail;
	}

	adapt->irqmgr.intr_regrt_info = intr_regrt_info;
	return 0;

fail:
	/* free interrupt entries */
	oss_free(intr_regrt_info->intr_entries);

	/* free interrupt registration info */
	oss_free(intr_regrt_info);

	return AMDGV_FAILURE;
}

int amdgv_irqmgr_unregister_interrupt(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.ih_funcs->unregister_interrupt) {
		adapt->irqmgr.ih_funcs->unregister_interrupt(adapt);
		return 0;
	}

	if (adapt->irqmgr.intr_regrt_info == NULL)
		return 0;

	/* remove interrupt handler from OS */
	oss_unregister_interrupt(adapt->dev, adapt->irqmgr.intr_regrt_info);

	/* free interrupt entries */
	oss_free(adapt->irqmgr.intr_regrt_info->intr_entries);

	/* free interrupt registration info */
	oss_free(adapt->irqmgr.intr_regrt_info);

	adapt->irqmgr.intr_regrt_info = NULL;
	return 0;
}

int amdgv_irqmgr_disable(struct amdgv_adapter *adapt)
{
	int ret = 0;
	struct amdgv_wait_for_cb_context cb_context = { 0 };

	adapt->irqmgr.ih.enabled = false;
	adapt->irqmgr.ih.rptr = 0;

	cb_context.ctx = adapt;
	cb_context.type = AMDGV_WAIT_FOR_IRQ_HANDLER;

	/* make sure irq handler exit */
	ret = amdgv_wait_for(adapt, amdgv_wait_for_irq_handler, &cb_context, ~0, 0);

	return ret;
}

void amdgv_irqmgr_enable_hw_interrupt(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.enable_hw_interrupt)
		adapt->irqmgr.enable_hw_interrupt(adapt);
}

void amdgv_irqmgr_disable_hw_interrupt(struct amdgv_adapter *adapt)
{
	if (adapt->irqmgr.disable_hw_interrupt)
		adapt->irqmgr.disable_hw_interrupt(adapt);
}

int amdgv_irqmgr_sw_init(struct amdgv_adapter *adapt)
{
	bool alloc_ring_in_sysmem = adapt->flags & AMDGV_FLAG_ALLOC_IV_RING_IN_SYSMEM;

	/* todo: check level 0 later */
	adapt->irqmgr.hv_event_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_LOWEST_RANK);
	if (adapt->irqmgr.hv_event_lock == OSS_INVALID_HANDLE) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		return AMDGV_FAILURE;
	}

	adapt->irqmgr.ih_event_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_LOWEST_RANK);
	if (adapt->irqmgr.ih_event_lock == OSS_INVALID_HANDLE) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		return AMDGV_FAILURE;
	}

	adapt->irqmgr.ih_handler3_lock = oss_spin_lock_init(AMDGV_SPIN_LOCK_LOWEST_RANK);
	if (adapt->irqmgr.ih_handler3_lock == OSS_INVALID_HANDLE) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_CREATE_SPIN_LOCK_FAIL, 0);
		return AMDGV_FAILURE;
	}

	/* Shim drv will parse IH and pass down decoded IH entry */
	if (adapt->flags & AMDGV_FLAG_DISABLE_PARSE_IH)
		adapt->irqmgr.disable_parse_ih = true;

	else {
		/* if flag is set allocates ih ring in sysmem, by default allocates in fb */
		if (amdgv_ih_ring_init(adapt, 1 << 16, alloc_ring_in_sysmem) != 0) {
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_IOV_INIT_IV_RING_FAIL, 0);
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

int amdgv_irqmgr_sw_fini(struct amdgv_adapter *adapt)
{
	if (!adapt->irqmgr.disable_parse_ih)
		amdgv_ih_ring_fini(adapt);

	if (adapt->irqmgr.hv_event_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->irqmgr.hv_event_lock);
		adapt->irqmgr.hv_event_lock = OSS_INVALID_HANDLE;
	}
	oss_spin_lock_fini(adapt->irqmgr.ih_handler3_lock);

	if (adapt->irqmgr.ih_event_lock != OSS_INVALID_HANDLE) {
		oss_spin_lock_fini(adapt->irqmgr.ih_event_lock);
		adapt->irqmgr.ih_event_lock = OSS_INVALID_HANDLE;
	}

	return 0;
}

int amdgv_irqmgr_write_virtualized_interrupt(struct amdgv_adapter *adapt, uint32_t idx_vf, uint32_t idx_table, uint64_t message_address, 
			uint32_t message_data, uint32_t vector_control, bool is_direct_write)
{
	if (adapt->irqmgr.write_virtualized_interrupt)
		adapt->irqmgr.write_virtualized_interrupt(adapt, idx_vf, idx_table, message_address, message_data, vector_control, is_direct_write);
	return 0;
}

void amdgv_irqmgr_update_virtualized_interrupt(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	if (adapt->irqmgr.update_virtualized_interrupt)
		adapt->irqmgr.update_virtualized_interrupt(adapt, idx_vf);
}

int amdgv_irqmgr_vf_disp_timer2_control(struct amdgv_adapter *adapt, uint32_t idx_vf, bool enable)
{
	if (adapt->irqmgr.vf_disp_timer2_control)
		return adapt->irqmgr.vf_disp_timer2_control(adapt, idx_vf, enable);

	return 0;
}
