/*
* Copyright 2026 Advanced Micro Devices, Inc.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
* THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
*/

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "amdgv_mcp.h"
#include "amdgv_mes.h"
#include "gc/gfx_v12_1.h"
#include "asic_reg/GC/gc_12_1_0_offset.h"
#include "asic_reg/GC/gc_12_1_0_sh_mask.h"
#include "mes_v12_api_def.h"

#define regCP_HQD_IQ_TIMER_DEFAULT		0x00000000
#define regCP_HQD_QUANTUM_DEFAULT		0x00000000
#define regCP_HQD_PQ_CONTROL_DEFAULT		0x00308509
#define regCP_MQD_CONTROL_DEFAULT		0x00000100
#define regCP_HQD_IB_CONTROL_MES_12_1_DEFAULT	0x00100000
#define regCP_HQD_PERSISTENT_STATE_DEFAULT	0x0ae06301

static const char *mes_v12_1_opcodes[] = {
	"SET_HW_RSRC",
	"SET_SCHEDULING_CONFIG",
	"ADD_QUEUE",
	"REMOVE_QUEUE",
	"PERFORM_YIELD",
	"SET_GANG_PRIORITY_LEVEL",
	"SUSPEND",
	"RESUME",
	"RESET",
	"SET_LOG_BUFFER",
	"CHANGE_GANG_PRORITY",
	"QUERY_SCHEDULER_STATUS",
	"unused",
	"SET_DEBUG_VMID",
	"MISC",
	"UPDATE_ROOT_PAGE_TABLE",
	"AMD_LOG",
	"SET_SE_MODE",
	"SET_GANG_SUBMIT",
	"SET_HW_RSRC_1",
	"INVALIDATE_TLBS",
};

static const char *mes_v12_1_misc_opcodes[] = {
	"WRITE_REG",
	"INV_GART",
	"QUERY_STATUS",
	"READ_REG",
	"WAIT_REG_MEM",
	"SET_SHADER_DEBUGGER",
	"NOTIFY_WORK_ON_UNMAPPED_QUEUE",
	"NOTIFY_TO_UNMAP_PROCESSES",
};

static const uint32_t this_block = AMDGV_GFX_BLOCK;

static int mes_v12_1_hw_fini_xcc(struct amdgv_adapter *adapt, uint32_t xcc_id);

static const char *mes_v12_1_get_op_string(union MESAPI__MISC *x_pkt)
{
	const char *op_str = NULL;

	if (x_pkt->header.opcode < sizeof(mes_v12_1_opcodes) / sizeof(mes_v12_1_opcodes[0]))
		op_str = mes_v12_1_opcodes[x_pkt->header.opcode];

	return op_str;
}

static const char *mes_v12_1_get_misc_op_string(union MESAPI__MISC *x_pkt)
{
	const char *op_str = NULL;

	if ((x_pkt->header.opcode == MES_SCH_API_MISC) &&
		(x_pkt->opcode < sizeof(mes_v12_1_misc_opcodes) / sizeof(mes_v12_1_misc_opcodes[0])))
		op_str = mes_v12_1_misc_opcodes[x_pkt->opcode];

	return op_str;
}

static void mes_v12_1_ring_set_wptr(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;

	if (ring->use_doorbell) {
		*((volatile uint64_t *)(ring->wptr_cpu_addr)) = ring->wptr;
		WDOORBELL64(ring->doorbell_index, ring->wptr);
	}
}

static uint64_t mes_v12_1_ring_get_rptr(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;

	if (ring->rptr_cpu_addr == NULL) {
		AMDGV_ERROR("Failed to get RPTR pointer (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		return 0;
	}
	return (*((volatile uint64_t *)(ring->rptr_cpu_addr)));
}

static uint64_t mes_v12_1_ring_get_wptr(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;

	if (ring->wptr_cpu_addr == NULL) {
		AMDGV_ERROR("Failed to get WPTR pointer (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		return 0;
	}
	return (*((volatile uint64_t *)(ring->wptr_cpu_addr)));
}

static int mes_v12_1_submit_pkt_and_poll_completion(struct amdgv_mes *mes,
				uint32_t xcc_id, uint32_t pipe, void *pkt, uint32_t size,
				uint32_t offset)
{
	union MESAPI__QUERY_MES_STATUS mes_status_pkt;
	struct amdgv_adapter *adapt = mes->adapt;
	uint32_t status_offs;
	uint64_t status_gpu_addr;
	volatile uint64_t *status_ptr;
	struct MES_API_STATUS *api_status;
	union MESAPI__MISC *x_pkt = pkt;
	signed long timeout_us = 600 * 1000; // 600ms
	signed long elapsed;
	const char *op_str, *misc_op_str;
	struct amdgv_ring *ring = &mes->ring[AMDGV_MES_INST(xcc_id, pipe)];
	uint32_t seq;

	if (x_pkt->header.opcode >= MES_SCH_API_MAX) {
		AMDGV_ERROR("Invalid MES API opcode (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		return AMDGV_FAILURE;
	}

	if (!ring->ring_obj || !ring->fence_drv.initialized) {
		AMDGV_ERROR("MES ring not ready (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		return AMDGV_FAILURE;
	}

	if (amdgv_wb_memory_get(adapt, &status_offs)) {
		AMDGV_ERROR("MES submit single: wb alloc failed\n");
		return AMDGV_FAILURE;
	}

	status_gpu_addr = adapt->wb.gpu_addr + (uint64_t)(status_offs * 4);
	status_ptr = (volatile uint64_t *)&adapt->wb.wb[status_offs];
	*status_ptr = 0;

	oss_spin_lock(ring->fence_drv.lock);
	if (amdgv_ring_alloc(ring, (size + sizeof(mes_status_pkt)) / 4)) {
		AMDGV_ERROR("MES ring full (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		amdgv_wb_memory_free(adapt, status_offs);
		oss_spin_unlock(ring->fence_drv.lock);
		return AMDGV_FAILURE;
	}

	seq = ++ring->fence_drv.sync_seq;

	api_status = (struct MES_API_STATUS *)((char *)pkt + offset);
	api_status->api_completion_fence_addr = status_gpu_addr;
	api_status->api_completion_fence_value = 1;

	amdgv_ring_write_multiple(ring, (uint32_t *)pkt, size / 4);

	oss_memset(&mes_status_pkt, 0, sizeof(mes_status_pkt));
	mes_status_pkt.header.type = MES_API_TYPE_SCHEDULER;
	mes_status_pkt.header.opcode = MES_SCH_API_QUERY_SCHEDULER_STATUS;
	mes_status_pkt.header.dwsize = API_FRAME_SIZE_IN_DWORDS;
	mes_status_pkt.api_status.api_completion_fence_addr = ring->fence_drv.gpu_addr;
	mes_status_pkt.api_status.api_completion_fence_value = seq;

	amdgv_ring_write_multiple(ring, (uint32_t *)&mes_status_pkt, sizeof(mes_status_pkt) / 4);

	amdgv_ring_commit(ring);

	oss_spin_unlock(ring->fence_drv.lock);

	op_str = mes_v12_1_get_op_string(x_pkt);
	misc_op_str = mes_v12_1_get_misc_op_string(x_pkt);

	if (op_str == NULL)
		op_str = "UNKNOWN";

	if (misc_op_str == NULL)
		misc_op_str = "UNKNOWN";

	AMDGV_INFO("MES Submission on ring: me=%d, pipe=%d, queue=%d, seq=%d, op_str=%s, misc_op_str=%s\n", ring->me, ring->pipe, ring->queue, seq, op_str, misc_op_str);

	elapsed = amdgv_fence_wait_polling(ring, seq, timeout_us);

	if (elapsed < 1 || !*status_ptr) {
		AMDGV_ERROR("MES submission timeout (me=%d, pipe=%d, op_str=%s, misc_op_str=%s)\n", ring->me, ring->pipe, op_str, misc_op_str);
		amdgv_wb_memory_free(adapt, status_offs);
		return AMDGV_FAILURE;
	}
	amdgv_wb_memory_free(adapt, status_offs);

	return 0;
}

static int mes_v12_1_misc_op(struct amdgv_mes *mes, struct mes_misc_op_input *input)
{
	return 0;
}

static int mes_v12_1_set_hw_resources(struct amdgv_mes *mes,
				int pipe, int xcc_id)
{
	union MESAPI_SET_HW_RESOURCES pkt;
	struct amdgv_adapter *adapt = mes->adapt;
	int i;
	int inst = AMDGV_MES_INST(xcc_id, pipe);

	oss_memset(&pkt, 0, sizeof(pkt));

	pkt.header.type   = MES_API_TYPE_SCHEDULER;
	pkt.header.opcode = MES_SCH_API_SET_HW_RSRC;
	pkt.header.dwsize = API_FRAME_SIZE_IN_DWORDS;

	pkt.vmid_mask_mmhub = mes->vmid_mask_mmhub;
	pkt.vmid_mask_gfxhub = mes->vmid_mask_gfxhub;
	pkt.gds_size = 0;
	pkt.paging_vmid = 0;

	for (i = 0; i < AMDGV_MES_MAX_COMPUTE_PIPES; i++)
		pkt.compute_hqd_mask[i] = mes->compute_hqd_mask[i];
	for (i = 0; i < AMDGV_MES_MAX_GFX_PIPES; i++)
		pkt.gfx_hqd_mask[i] = mes->gfx_hqd_mask[i];
	for (i = 0; i < AMDGV_MES_MAX_SDMA_PIPES; i++)
		pkt.sdma_hqd_mask[i] = mes->sdma_hqd_mask[i];
	for (i = 0; i < AMDGV_MES_PRIORITY_NUM_LEVELS; i++)
		pkt.aggregated_doorbells[i] = mes->aggregated_doorbells[i];

	pkt.g_sch_ctx_gpu_mc_ptr = mes->sch_ctx_gpu_addr[inst];
	pkt.query_status_fence_gpu_mc_ptr = mes->query_status_fence_gpu_addr[inst];

	for (i = 0; i < AMDGV_MES_NUM_REG_SEGMENTS; i++) {
		if (i < adapt->reg_num_segs[GC_HWIP][xcc_id])
			pkt.gc_base[i]     = adapt->reg_offset[GC_HWIP][xcc_id][i];
		if (i < adapt->reg_num_segs[MMHUB_HWIP][xcc_id])
			pkt.mmhub_base[i]  = adapt->reg_offset[MMHUB_HWIP][xcc_id][i];
		if (i < adapt->reg_num_segs[OSSSYS_HWIP][xcc_id])
			pkt.osssys_base[i] = adapt->reg_offset[OSSSYS_HWIP][xcc_id][i];
		AMDGV_INFO("MES KIQ SET_HW_RSRC, gc_base[%d]=0x%x, mmhub_base[%d]=0x%x, osssys_base[%d]=0x%x\n", i, pkt.gc_base[i], i, pkt.mmhub_base[i], i, pkt.osssys_base[i]);
	}

	pkt.disable_reset                      = 1;
	pkt.disable_mes_log                    = 1;
	pkt.use_different_vmid_compute         = 1;
	pkt.enable_reg_active_poll             = 1;
	pkt.enable_level_process_quantum_check = 1;
	pkt.oversubscription_timer     = 50;
	pkt.unmapped_doorbell_handling = 1;

	return mes_v12_1_submit_pkt_and_poll_completion(mes, xcc_id, pipe, &pkt,
				sizeof(pkt), offsetof(union MESAPI_SET_HW_RESOURCES, api_status));
}

static int mes_v12_1_set_hw_resources_1(struct amdgv_mes *mes,
	int pipe, int xcc_id)
{
	union MESAPI_SET_HW_RESOURCES_1 pkt;
	oss_memset(&pkt, 0, sizeof(pkt));

	pkt.header.type = MES_API_TYPE_SCHEDULER;
	pkt.header.opcode = MES_SCH_API_SET_HW_RSRC_1;
	pkt.header.dwsize = API_FRAME_SIZE_IN_DWORDS;
	pkt.mes_kiq_unmap_timeout = 100;

	return mes_v12_1_submit_pkt_and_poll_completion(mes, xcc_id, pipe, &pkt,
				sizeof(pkt), offsetof(union MESAPI_SET_HW_RESOURCES_1, api_status));
}

#if 0
static int mes_v12_1_query_sched_status(struct amdgv_mes *mes,
	int pipe, int xcc_id)
{
	union MESAPI__QUERY_MES_STATUS pkt;

	oss_memset(&pkt, 0, sizeof(pkt));

	pkt.header.type = MES_API_TYPE_SCHEDULER;
	pkt.header.opcode = MES_SCH_API_QUERY_SCHEDULER_STATUS;
	pkt.header.dwsize = API_FRAME_SIZE_IN_DWORDS;

	return mes_v12_1_submit_pkt_and_poll_completion(mes, xcc_id, pipe,
			&pkt, sizeof(pkt),
			offsetof(union MESAPI__QUERY_MES_STATUS, api_status));
}
#endif

static int mes_v12_1_ring_test(struct amdgv_ring *ring)
{
	union MESAPI__QUERY_MES_STATUS mes_status_pkt;
	union MESAPI__QUERY_MES_STATUS mes_fence_pkt;
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t status_offs;
	uint64_t status_gpu_addr;
	volatile uint64_t *status_ptr;
	struct MES_API_STATUS *api_status;
	uint32_t seq;
	signed long elapsed;
	signed long timeout_us = 600 * 1000; // 600ms

	if (amdgv_wb_memory_get(adapt, &status_offs)) {
		AMDGV_ERROR("MES submit single: wb alloc failed\n");
		return AMDGV_FAILURE;
	}

	oss_memset(&mes_status_pkt, 0, sizeof(mes_status_pkt));

	mes_status_pkt.header.type = MES_API_TYPE_SCHEDULER;
	mes_status_pkt.header.opcode = MES_SCH_API_QUERY_SCHEDULER_STATUS;
	mes_status_pkt.header.dwsize = API_FRAME_SIZE_IN_DWORDS;

	status_gpu_addr = adapt->wb.gpu_addr + (uint64_t)(status_offs * 4);
	status_ptr = (volatile uint64_t *)&adapt->wb.wb[status_offs];
	*status_ptr = 0;

	oss_spin_lock(ring->fence_drv.lock);
	if (amdgv_ring_alloc(ring, (sizeof(mes_status_pkt) + sizeof(mes_fence_pkt)) / 4)) {
		AMDGV_ERROR("MES ring full (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		amdgv_wb_memory_free(adapt, status_offs);
		oss_spin_unlock(ring->fence_drv.lock);
		return AMDGV_FAILURE;
	}

	seq = ++ring->fence_drv.sync_seq;

	api_status = (struct MES_API_STATUS *)&mes_status_pkt.api_status;
	api_status->api_completion_fence_addr = status_gpu_addr;
	api_status->api_completion_fence_value = 1;

	amdgv_ring_write_multiple(ring, (uint32_t *)&mes_status_pkt, sizeof(mes_status_pkt) / 4);

	oss_memset(&mes_fence_pkt, 0, sizeof(mes_fence_pkt));
	mes_fence_pkt.header.type = MES_API_TYPE_SCHEDULER;
	mes_fence_pkt.header.opcode = MES_SCH_API_QUERY_SCHEDULER_STATUS;
	mes_fence_pkt.header.dwsize = API_FRAME_SIZE_IN_DWORDS;
	mes_fence_pkt.api_status.api_completion_fence_addr = ring->fence_drv.gpu_addr;
	mes_fence_pkt.api_status.api_completion_fence_value = seq;

	amdgv_ring_write_multiple(ring, (uint32_t *)&mes_fence_pkt, sizeof(mes_fence_pkt) / 4);

	amdgv_ring_commit(ring);

	oss_spin_unlock(ring->fence_drv.lock);

	elapsed = amdgv_fence_wait_polling(ring, seq, timeout_us);

	if (elapsed < 1 || !*status_ptr) {
		AMDGV_ERROR("MES submission timeout (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		amdgv_wb_memory_free(adapt, status_offs);
		return AMDGV_FAILURE;
	}

	amdgv_wb_memory_free(adapt, status_offs);

	return 0;

}

static int mes_v12_1_map_legacy_queue(struct amdgv_mes *mes,
				struct mes_map_legacy_queue_input *input)
{
	union MESAPI__ADD_QUEUE mes_add_queue_pkt;

	oss_memset(&mes_add_queue_pkt, 0, sizeof(mes_add_queue_pkt));

	mes_add_queue_pkt.header.type = MES_API_TYPE_SCHEDULER;
	mes_add_queue_pkt.header.opcode = MES_SCH_API_ADD_QUEUE;
	mes_add_queue_pkt.header.dwsize = API_FRAME_SIZE_IN_DWORDS;

	mes_add_queue_pkt.pipe_id = input->pipe_id;
	mes_add_queue_pkt.queue_id = input->queue_id;
	mes_add_queue_pkt.doorbell_offset = input->doorbell_offset;
	mes_add_queue_pkt.mqd_addr = input->mqd_addr;
	mes_add_queue_pkt.wptr_addr = input->wptr_addr;
	mes_add_queue_pkt.queue_type = input->queue_type;
	mes_add_queue_pkt.map_legacy_kq = 1;

	return mes_v12_1_submit_pkt_and_poll_completion(mes, input->xcc_id,
				AMDGV_MES_KIQ_PIPE, &mes_add_queue_pkt, sizeof(mes_add_queue_pkt),
				offsetof(union MESAPI__ADD_QUEUE, api_status));
}

static int mes_v12_1_unmap_legacy_queue(struct amdgv_mes *mes,
				struct mes_unmap_legacy_queue_input *input)
{
	union MESAPI__REMOVE_QUEUE mes_remove_queue_pkt;

	oss_memset(&mes_remove_queue_pkt, 0, sizeof(mes_remove_queue_pkt));

	mes_remove_queue_pkt.header.type = MES_API_TYPE_SCHEDULER;
	mes_remove_queue_pkt.header.opcode = MES_SCH_API_REMOVE_QUEUE;
	mes_remove_queue_pkt.header.dwsize = API_FRAME_SIZE_IN_DWORDS;

	mes_remove_queue_pkt.doorbell_offset = input->doorbell_offset;
	mes_remove_queue_pkt.gang_context_addr = 0;

	mes_remove_queue_pkt.pipe_id = input->pipe_id;
	mes_remove_queue_pkt.queue_id = input->queue_id;

	mes_remove_queue_pkt.unmap_legacy_queue = 1;
	mes_remove_queue_pkt.queue_type = input->queue_type;

	if (input->action == RESET_QUEUES)
		mes_remove_queue_pkt.remove_queue_after_reset = 1;

	return mes_v12_1_submit_pkt_and_poll_completion(mes,
			input->xcc_id, AMDGV_MES_KIQ_PIPE,
			&mes_remove_queue_pkt, sizeof(mes_remove_queue_pkt),
			offsetof(union MESAPI__REMOVE_QUEUE, api_status));
}

static int mes_v12_1_reset_hw_queue(struct amdgv_mes *mes,
				struct mes_reset_queue_input *input)
{
	return 0;
}

static const struct amdgv_ring_funcs mes_v12_1_ring_funcs = {
	.type = AMDGV_RING_TYPE_MES,
	.align_mask = 1,
	.nop = 0,
	.support_64bit_ptrs = true,
	.get_rptr = mes_v12_1_ring_get_rptr,
	.get_wptr = mes_v12_1_ring_get_wptr,
	.set_wptr = mes_v12_1_ring_set_wptr,
	.insert_nop = amdgv_ring_insert_nop,
	.test_ring = mes_v12_1_ring_test,
};

/*
	On PF, There is none user service queue scenario and compute queues
	are mapped to MES as legacy kernel queues.
*/
static struct amdgv_mes_funcs mes_v12_1_ops_funcs = {
	.map_legacy_queue = mes_v12_1_map_legacy_queue,
	.unmap_legacy_queue = mes_v12_1_unmap_legacy_queue,
	.misc_op = mes_v12_1_misc_op,
	.reset_hw_queue = mes_v12_1_reset_hw_queue,
};

static int mes_v12_1_mqd_sw_init(struct amdgv_adapter *adapt,
									enum amdgv_mes_pipe pipe,
									uint32_t xcc_id)
{
	int mqd_size = sizeof(struct v12_1_mes_mqd);
	int inst = AMDGV_MES_INST(xcc_id, pipe);
	struct amdgv_ring *ring = &adapt->mes.ring[inst];
	uint32_t mem_id = (pipe == AMDGV_MES_SCHED_PIPE) ?
					MEM_MES_MQD : MEM_KIQ_MQD;

	if (ring->mqd_obj)
		return 0;

	ring->mqd_obj = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, mqd_size, PAGE_SIZE, mem_id);
	if (!ring->mqd_obj) {
		AMDGV_ERROR("Failed to allocate MQD memory (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mes_v12_1_mqd_init(struct amdgv_adapter *adapt,
								enum amdgv_mes_pipe pipe,
								uint32_t xcc_id)
{
	struct amdgv_ring *ring = &adapt->mes.ring[AMDGV_MES_INST(xcc_id, pipe)];
	struct v12_1_mes_mqd *mqd;
	uint64_t hqd_gpu_addr, wb_gpu_addr;
	uint32_t tmp;

	if (ring->mqd_obj == NULL) {
		AMDGV_ERROR("MQD object is not allocated (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		return AMDGV_FAILURE;
	}

	ring->mqd_gpu_addr = amdgv_memmgr_get_gpu_addr(ring->mqd_obj);
	ring->mqd_ptr = amdgv_memmgr_get_cpu_addr(ring->mqd_obj);
	mqd = (struct v12_1_mes_mqd *)ring->mqd_ptr;

	if (mqd == NULL) {
		AMDGV_ERROR("Failed to get MQD pointer (me=%d, pipe=%d)\n", ring->me, ring->pipe);
		return AMDGV_FAILURE;
	}

	oss_memset(mqd, 0, sizeof(struct v12_1_mes_mqd));

	mqd->header = 0xC0310800;
	mqd->compute_pipelinestat_enable = 0x00000001;
	mqd->compute_static_thread_mgmt_se0 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se1 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se2 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se3 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se4 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se5 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se6 = 0xffffffff;
	mqd->compute_static_thread_mgmt_se7 = 0xffffffff;
	mqd->compute_misc_reserved = 0x00000007;
	mqd->cp_hqd_ib_control = regCP_HQD_IB_CONTROL_MES_12_1_DEFAULT;
	mqd->cp_hqd_iq_timer = regCP_HQD_IQ_TIMER_DEFAULT;
	mqd->cp_hqd_quantum = regCP_HQD_QUANTUM_DEFAULT;
	mqd->cp_hqd_queue_priority = 0x00000000;
	mqd->cp_hqd_hq_status0 = 0x40000000;

	/* disable the queue if it's active */
	mqd->cp_hqd_dequeue_request = 0;
	mqd->cp_hqd_pq_rptr = 0;
	mqd->cp_hqd_pq_wptr_lo = 0;
	mqd->cp_hqd_pq_wptr_hi = 0;

	/* set the pointer to the MQD */
	mqd->cp_mqd_base_addr_lo = lower_32_bits(ring->mqd_gpu_addr & 0xfffffffc);
	mqd->cp_mqd_base_addr_hi = upper_32_bits(ring->mqd_gpu_addr);

	/* set MQD vmid to 0 */
	tmp = regCP_MQD_CONTROL_DEFAULT;
	tmp = REG_SET_FIELD(tmp, CP_MQD_CONTROL, VMID, 0);
	mqd->cp_mqd_control = tmp;
	/* set the pointer to the HQD, this is similar CP_RB0_BASE/_HI */
	hqd_gpu_addr = ring->gpu_addr >> 8;
	mqd->cp_hqd_pq_base_lo = hqd_gpu_addr;
	mqd->cp_hqd_pq_base_hi = upper_32_bits(hqd_gpu_addr);

	tmp = regCP_HQD_PQ_CONTROL_DEFAULT;
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, QUEUE_SIZE, ring->log2_ring_size - 1);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, RPTR_BLOCK_SIZE, (order_base_2(AMDGV_GPU_PAGE_SIZE / 4) - 1));
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, UNORD_DISPATCH, 0);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, TUNNEL_DISPATCH, 0);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, NO_UPDATE_RPTR, 1);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, PRIV_STATE, 1);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_CONTROL, KMD_QUEUE, 1);
	mqd->cp_hqd_pq_control = tmp;

	/* set the wb address whether it's enabled or not */
	wb_gpu_addr = ring->rptr_gpu_addr;
	mqd->cp_hqd_pq_rptr_report_addr_lo = wb_gpu_addr & 0xfffffffc;
	mqd->cp_hqd_pq_rptr_report_addr_hi =
		upper_32_bits(wb_gpu_addr) & 0xffff;

	/* only used if CP_PQ_WPTR_POLL_CNTL.CP_PQ_WPTR_POLL_CNTL__EN_MASK=1 */
	wb_gpu_addr = ring->wptr_gpu_addr;
	mqd->cp_hqd_pq_wptr_poll_addr_lo = wb_gpu_addr & 0xfffffffc ;
	mqd->cp_hqd_pq_wptr_poll_addr_hi = upper_32_bits(wb_gpu_addr) & 0xffff;

	/* enable the doorbell if requested */
	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_DOORBELL_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
			DOORBELL_OFFSET, ring->doorbell_index);
	/* mes_kiq_init.md step 13: DOORBELL_MODE=0 */
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
			DOORBELL_HIT, 0);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
			DOORBELL_EN, 1);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL,
			DOORBELL_SOURCE, 0);

	mqd->cp_hqd_pq_doorbell_control = tmp;

	/* set the vmid for the queue */
	mqd->cp_hqd_vmid = 0;

	tmp = regCP_HQD_PERSISTENT_STATE_DEFAULT;
	tmp = REG_SET_FIELD(tmp, CP_HQD_PERSISTENT_STATE, PRELOAD_SIZE, 0x63);
	mqd->cp_hqd_persistent_state = tmp;

	mqd->cp_hqd_gfx_control = 1 << 15;
	mqd->cp_hqd_active = 1;

	return 0;
}

static int mes_v12_1_queue_init_register(struct amdgv_ring *ring, uint32_t xcc_id)
{
	struct amdgv_adapter *adapt = ring->adapt;
	struct v12_1_mes_mqd *mqd = (struct v12_1_mes_mqd *)ring->mqd_ptr;
	uint32_t tmp;

	oss_mutex_lock(adapt->srbm_mutex);
	gfx_v12_1_grbm_select(adapt, ring->me, ring->pipe, ring->queue, 0, GET_INST(GC, xcc_id));
	/* set the vmid for the queue */
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_VMID, 0);

	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_DOORBELL_CONTROL);
	tmp = REG_SET_FIELD(tmp, CP_HQD_PQ_DOORBELL_CONTROL, DOORBELL_EN, 0);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_DOORBELL_CONTROL, tmp);

	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MQD_CONTROL,
			mqd->cp_mqd_control);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MQD_BASE_ADDR,
			mqd->cp_mqd_base_addr_lo);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MQD_BASE_ADDR_HI,
			mqd->cp_mqd_base_addr_hi);

		/* set the pointer to the HQD, this is similar CP_RB0_BASE/_HI */
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_BASE,
			mqd->cp_hqd_pq_base_lo);

	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_BASE_HI,
			mqd->cp_hqd_pq_base_hi);

	/* set the wb address whether it's enabled or not */
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_RPTR_REPORT_ADDR,
			mqd->cp_hqd_pq_rptr_report_addr_lo);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_RPTR_REPORT_ADDR_HI,
			mqd->cp_hqd_pq_rptr_report_addr_hi);

	/* set up the HQD, this is similar to CP_RB0_CNTL */
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_CONTROL,
			mqd->cp_hqd_pq_control);

	/* only used if CP_PQ_WPTR_POLL_CNTL.CP_PQ_WPTR_POLL_CNTL__EN_MASK=1 */
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_POLL_ADDR,
			mqd->cp_hqd_pq_wptr_poll_addr_lo);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_WPTR_POLL_ADDR_HI,
			mqd->cp_hqd_pq_wptr_poll_addr_hi);

	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PQ_DOORBELL_CONTROL,
				mqd->cp_hqd_pq_doorbell_control);

	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_PERSISTENT_STATE,
				mqd->cp_hqd_persistent_state);

	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_PQ_STATUS);
	tmp = REG_SET_FIELD(tmp, CP_PQ_STATUS, DOORBELL_UPDATED_EN, 1);
	tmp = REG_SET_FIELD(tmp, CP_PQ_STATUS, DOORBELL_ENABLE, 1);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_PQ_STATUS, tmp);

	/* activate the queue */
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_ACTIVE,
			mqd->cp_hqd_active);

	gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, GET_INST(GC, xcc_id));
	oss_mutex_unlock(adapt->srbm_mutex);

	return 0;
}

static int mes_v12_1_queue_init(struct amdgv_adapter *adapt,
				enum amdgv_mes_pipe pipe, uint32_t xcc_id)
{
	struct amdgv_ring *ring = &adapt->mes.ring[AMDGV_MES_INST(xcc_id, pipe)];
	int r = 0;

	r = mes_v12_1_mqd_init(adapt, pipe, xcc_id);
	if (r)
		return r;

	r = mes_v12_1_queue_init_register(ring, xcc_id);

	if (r)
		return r;

	oss_mutex_lock(adapt->srbm_mutex);
	gfx_v12_1_grbm_select(adapt, 3, pipe, 0, 0, GET_INST(GC, xcc_id));
	if (pipe == AMDGV_MES_KIQ_PIPE)
		adapt->mes.kiq_version = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_GP3_LO);
	gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, GET_INST(GC, xcc_id));
	oss_mutex_unlock(adapt->srbm_mutex);

	return 0;
}

static int mes_v12_1_ring_init(struct amdgv_adapter *adapt, uint32_t xcc_id, uint32_t pipe)
{
	struct amdgv_ring *ring;
	int inst = AMDGV_MES_INST(xcc_id, pipe);
	uint32_t mem_id;
	uint32_t frame_dword_size = AMDGV_MES_RING_FRAME_SIZE;
	uint32_t frame_number = AMDGV_MES_RING_FRAME_NUMBER;

	if (inst >= AMDGV_MAX_MES_INST_PIPES)
		return AMDGV_FAILURE;

	ring = &adapt->mes.ring[inst];

	adapt->mes.funcs = &mes_v12_1_ops_funcs;
	ring->funcs = &mes_v12_1_ring_funcs;
	ring->xcc_id = xcc_id;
	ring->me = 3;
	ring->pipe = pipe;
	ring->queue = 0;

	ring->ring_obj = NULL;
	ring->use_doorbell = true;
	ring->no_scheduler = true;
	ring->is_mes_queue = true;

	if (pipe == AMDGV_MES_KIQ_PIPE)
		ring->doorbell_index = (adapt->doorbell_index.mes_ring1 +
				xcc_id * adapt->doorbell_index.xcc_doorbell_range) << 1;
	else
		ring->doorbell_index = (adapt->doorbell_index.mes_ring0 +
				xcc_id * adapt->doorbell_index.xcc_doorbell_range) << 1;

	mem_id = (pipe == AMDGV_MES_KIQ_PIPE) ? MEM_KIQ_RING : MEM_MES_RING;

	return amdgv_ring_init(adapt, ring, frame_dword_size, frame_number, AMDGV_RING_PRIO_DEFAULT, NULL, mem_id);
}

static int mes_v12_1_enable(struct amdgv_adapter *adapt,
				bool enable, uint32_t xcc_id)
{
	uint64_t uc_start_addr = 0;
	uint32_t ucode_id,pipe, data = 0;
	int r = 0;

	if (enable) {
		data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_CNTL);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_INVALIDATE_ICACHE, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE0_RESET, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE1_RESET, 1);
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_CNTL, data);

		if (!adapt->ucode.get_ucode_start_addr)
			return AMDGV_FAILURE;

		oss_mutex_lock(adapt->srbm_mutex);
		for (pipe = 0; pipe < AMDGV_MAX_MES_PIPES; pipe++) {
			ucode_id = (pipe == AMDGV_MES_KIQ_PIPE) ? AMDGV_FIRMWARE_ID__MES_THREAD1 : AMDGV_FIRMWARE_ID__CP_MES;
			r = adapt->ucode.get_ucode_start_addr(adapt, ucode_id, &uc_start_addr);

			if (r || !uc_start_addr) {
				AMDGV_ERROR("Failed to get ucode start address for firmware id %d\n", ucode_id);
				gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, GET_INST(GC, xcc_id));
				oss_mutex_unlock(adapt->srbm_mutex);
				return AMDGV_FAILURE;
			}
			gfx_v12_1_grbm_select(adapt, 3, pipe, 0, 0, GET_INST(GC, xcc_id));

			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_PRGRM_CNTR_START, lower_32_bits(uc_start_addr));
			WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_PRGRM_CNTR_START_HI, upper_32_bits(uc_start_addr));

			oss_udelay(100);
		}
		gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, GET_INST(GC, xcc_id));
		oss_mutex_unlock(adapt->srbm_mutex);

		data = REG_SET_FIELD(0, CP_MES_CNTL, MES_PIPE0_ACTIVE, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE1_ACTIVE, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_STEP, 0);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_HALT, 0);
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_CNTL, data);

		oss_msleep(100);

	} else {
		data = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_CNTL);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE0_ACTIVE, 0);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE1_ACTIVE, 0);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE0_RESET, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_PIPE1_RESET, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_INVALIDATE_ICACHE, 1);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_STEP, 0);
		data = REG_SET_FIELD(data, CP_MES_CNTL, MES_HALT, 1);
		WREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_CNTL, data);

		oss_msleep(100);
	}

	return 0;
}

static int mes_v12_1_wait_for_hqd_active_pipe_pending(struct amdgv_adapter *adapt, int xcc_id)
{
	int wait_ret;

	wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET_NAME(GC, GET_INST(GC, xcc_id), regCP_MES_GP3_HI),
					   0xffffff, 0,
					   AMDGV_TIMEOUT(TIMEOUT_SMU_REG),
					   AMDGV_WAIT_CHECK_NE, AMDGV_WAIT_FLAG_AUTO);
	if (wait_ret)
		return AMDGV_FAILURE;

	return 0;
}

static int mes_v12_1_mes_pipes_status_check(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	int wait_ret;
	uint32_t mes_cntl, hqd_active, inst_ptr;
	uint32_t gp3_lo, gp3_hi;

	oss_mutex_lock(adapt->srbm_mutex);
	gfx_v12_1_grbm_select(adapt, 3, 1, 0, 0, GET_INST(GC, xcc_id));

	wait_ret = mes_v12_1_wait_for_hqd_active_pipe_pending(adapt, xcc_id);
	if (wait_ret) // retry once
		wait_ret = mes_v12_1_wait_for_hqd_active_pipe_pending(adapt, xcc_id);

	if (wait_ret) {
		AMDGV_ERROR("Failed to wait for MES.KIQ HQD active pipe pending for XCC %d\n", xcc_id);
		gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, GET_INST(GC, xcc_id));
		oss_mutex_unlock(adapt->srbm_mutex);
		return AMDGV_FAILURE;
	}

	mes_cntl = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_CNTL);
	mes_cntl = REG_GET_FIELD(mes_cntl, CP_MES_CNTL, MES_PIPE1_ACTIVE);

	hqd_active = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_HQD_ACTIVE) & 1;
	gp3_lo = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_GP3_LO);
	gp3_hi = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_GP3_HI);

	inst_ptr = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regCP_MES_INSTR_PNTR);

	gfx_v12_1_grbm_select(adapt, 0, 0, 0, 0, GET_INST(GC, xcc_id));
	oss_mutex_unlock(adapt->srbm_mutex);

	AMDGV_INFO("MES.KIQ status XCC %d: MES_PIPE1_ACTIVE=%u, CP_HQD_ACTIVE=%u, gp3_hi=%x, gp3_lo=%x, mes_pc=0x%x\n",
		xcc_id, mes_cntl, hqd_active, gp3_hi, gp3_lo, inst_ptr << 2);

	if (!mes_cntl) {
		AMDGV_ERROR("MES.KIQ pipe 1 not active on XCC %d (CP_MES_CNTL.MES_PIPE1_ACTIVE=0)\n", xcc_id);
		return AMDGV_FAILURE;
	}

	if (!hqd_active) {
		AMDGV_ERROR("MES.KIQ HQD not active on XCC %d (CP_HQD_ACTIVE=0)\n", xcc_id);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mes_v12_1_hw_fini_xcc(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	// place holder of MES.sched pipe fini
	return 0;
}

static int mes_v12_1_xcc_hw_init(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	// place holder of MES.sched pipe initialization
	return 0;
}

static int mes_v12_1_kiq_setting(struct amdgv_ring *ring, uint32_t xcc_id)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t tmp = 0;

	tmp = RREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS);
	tmp &= 0xffffff00;
	tmp |= (ring->me << 5) | (ring->pipe << 3) | (ring->queue);
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS, tmp);
	tmp |= 0x80;
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS, tmp);

	return 0;
}

static int mes_v12_1_kiq_hw_init(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	struct amdgv_ring *ring;
	int inst = AMDGV_MES_INST(xcc_id, AMDGV_MES_KIQ_PIPE);
	int r = 0;

	ring = &adapt->mes.ring[inst];

	r = amdgv_ring_init_set(adapt, ring);
	if (r)
		return r;

	r = mes_v12_1_kiq_setting(ring, xcc_id);
	if (r)
		return r;

	r = mes_v12_1_enable(adapt, true, xcc_id);
	if (r)
		goto failure;

	r = mes_v12_1_queue_init(adapt, AMDGV_MES_KIQ_PIPE, xcc_id);
	if (r)
		goto failure;

	r = mes_v12_1_mes_pipes_status_check(adapt, xcc_id);
	if (r)
		goto failure;

	r = mes_v12_1_set_hw_resources(&adapt->mes, AMDGV_MES_KIQ_PIPE, xcc_id);
	if (r)
		goto failure;

	r = mes_v12_1_set_hw_resources_1(&adapt->mes, AMDGV_MES_KIQ_PIPE, xcc_id);
	if (r)
		goto failure;

	if (adapt->mes.enable_legacy_queue_map) {
		r = mes_v12_1_xcc_hw_init(adapt, xcc_id);
		if (r)
			goto failure;
	}

	return r;

failure:
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS, 0);
	mes_v12_1_enable(adapt, false, xcc_id);
	return r;
}

static int mes_v12_1_kiq_hw_fini(struct amdgv_adapter *adapt, uint32_t xcc_id)
{
	WREG32_SOC15(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS, 0);

	return mes_v12_1_enable(adapt, false, xcc_id);
}

static int mes_v12_1_sw_fini(struct amdgv_adapter *adapt)
{
	int pipe, xcc_id, num_xcc = adapt->mcp.gfx.num_xcc;
	int r;
	struct amdgv_ring *ring;

	for (xcc_id = 0; xcc_id < num_xcc; xcc_id++) {
		for (pipe = 0; pipe < AMDGV_MAX_MES_PIPES; pipe++) {
			int inst = AMDGV_MES_INST(xcc_id, pipe);

			if (pipe == AMDGV_MES_SCHED_PIPE)
				continue;

			ring = &adapt->mes.ring[inst];

			amdgv_ring_fini(ring);

			if (ring->mqd_obj) {
				amdgv_memmgr_free(ring->mqd_obj);
				ring->mqd_obj = NULL;
			}
		}
	}

	r = amdgv_mes_fini(adapt);
	if (r)
		AMDGV_ERROR("Failed to fini MES\n");

	return r;
}

static int mes_v12_1_sw_init(struct amdgv_adapter *adapt)
{
	uint32_t xcc_id, pipe, num_xcc = adapt->mcp.gfx.num_xcc;
	int r;

	adapt->mes.kiq_hw_init = &mes_v12_1_kiq_hw_init;
	adapt->mes.kiq_hw_fini = &mes_v12_1_kiq_hw_fini;
	adapt->mes.enable_legacy_queue_map = true;
	adapt->mes.enable_coop_mode = false;

	r = amdgv_mes_init(adapt);
	if (r)
		return r;

	for (xcc_id = 0; xcc_id < num_xcc; xcc_id++) {
		for (pipe = 0; pipe < AMDGV_MAX_MES_PIPES; pipe++) {
			if (pipe == AMDGV_MES_SCHED_PIPE)
				continue;

			r = mes_v12_1_mqd_sw_init(adapt, pipe, xcc_id);
			if (r)
				goto error;

			r = mes_v12_1_ring_init(adapt, xcc_id, pipe);
			if (r)
				goto error;
		}
	}

	return 0;

error:
	mes_v12_1_sw_fini(adapt);

	return r;
}

static int mes_v12_1_hw_init(struct amdgv_adapter *adapt)
{
	int r;
	uint32_t xcc_id, num_xcc = adapt->mcp.gfx.num_xcc;

	for (xcc_id = 0; xcc_id < num_xcc; xcc_id++) {
		r = mes_v12_1_xcc_hw_init(adapt, xcc_id);
		if (r)
			return r;
	}

	return 0;
}

static int mes_v12_1_hw_fini(struct amdgv_adapter *adapt)
{
	int r = 0;
	uint32_t xcc_id, num_xcc = adapt->mcp.gfx.num_xcc;

	for (xcc_id = 0; xcc_id < num_xcc; xcc_id++) {
		if (mes_v12_1_hw_fini_xcc(adapt, xcc_id)) {
			AMDGV_ERROR("Failed to fini MES on xcc %d\n", xcc_id);
			r = AMDGV_FAILURE;
		}
	}

	return r;
}

struct amdgv_init_func mes_v12_1_func = {
	.name = "mes_v12_1_func",
	.sw_init = mes_v12_1_sw_init,
	.sw_fini = mes_v12_1_sw_fini,
	.hw_init = mes_v12_1_hw_init,
	.hw_fini = mes_v12_1_hw_fini,
};
