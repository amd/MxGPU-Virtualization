/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_IRQMGR_H
#define AMDGV_IRQMGR_H

#include "amdgv_api.h"

/* src_id */
#define IH_IV_SRCID_DF			   0x0 // 0
#define IH_IV_SRCID_BIF_PF_VF_MSGBUF_VALID 0x00000087 // 135
#define IH_IV_SRCID_BIF_PF_VF_MSGBUF_ACK   0x00000088 // 136
#define IH_IV_SRCID_BIF_VF_PF_MSGBUF_VALID 0x00000089 // 137
#define IH_IV_SRCID_BIF_VF_PF_MSGBUF_ACK   0x0000008a // 138
#define IH_IV_SRCID_MP0_IH_SW_INT	   	   0x000000FF	//255
#define IH_IV_SRCID_DF_MCA_INT		   	   0xFF0003aa // 1170

#define IH_IV_SRCID_RLC_GC_FED_INTERRUPT   0x80

#define IH_IV_SRCID_RLC_GC_FED_COOKIE     0xCB

#define IH_IV_CLIENTID_DF    0x17
#define IH_IV_CLIENTID_VMC   0x12
#define IH_IV_CLIENTID_UTCL2 0x1B
#define IH_IV_CLIENTID_MP0	 0x1E
#define IH_IV_CLIENTID_MP1   0x1F

/* ClientID for SOC21 */
#define IH_IV_CLIENTID_GFX   0xA
#define IH_IV_CLIENTID_RLC   0x7

struct amdgv_ih_ring {
	volatile uint32_t       *ring;
	unsigned int		 rptr;
	unsigned int		 ring_size;
	unsigned int		 ring_size_log2;
	uint64_t		 gpu_addr;
	uint32_t		 ptr_mask;
	struct amdgv_memmgr_mem *fb_mem;
	bool			 enabled;
	unsigned int		 wptr_offs;
	unsigned int		 rptr_offs;
	uint32_t		 doorbell_index;
	bool			 use_doorbell;
	bool			 use_bus_addr;
	bool			 use_bh;
	bool			 irq_processing;

	/* only used when use_bus_addr = true */
	uint64_t		 rb_dma_addr;
	void			*dma_mem_handle;
};

struct amdgv_iv_entry {
	unsigned int	client_id;
	unsigned int	src_id;
	unsigned int	ring_id;
	unsigned int	vm_id;
	unsigned int	vm_id_src;
	uint64_t	timestamp;
	unsigned int	timestamp_src;
	unsigned int	pas_id;
	unsigned int	node_id;
	unsigned int	pasid_src;
	unsigned int	src_data[AMDGV_IH_SRC_DATA_MAX_SIZE_DW];
	const uint32_t *iv_entry;
};

struct amdgv_ih_funcs {
	/* ring read/write ptr handling, called from interrupt context */
	uint32_t (*get_wptr)(struct amdgv_adapter *adapt);
	void (*decode_iv)(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry);
	void (*set_rptr)(struct amdgv_adapter *adapt);
	uint32_t (*get_rptr)(struct amdgv_adapter *adapt);

	/* process iv ring entry */
	int (*process)(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry);

	/* enable or disable IH iv ring */
	int (*enable_iv_ring)(struct amdgv_adapter *adapt, bool enable);

	/* enable or disable mailbox interrupt */
	void (*enable_mbox)(struct amdgv_adapter *adapt, bool enable);

	/* re-arm ih ring on VF */
	void (*re_arm_vf_ih_ring)(struct amdgv_adapter *adapt, uint32_t idx_vf);

	/* toggle disp_timer2 interrupt */
	int (*toggle_disp_timer2)(struct amdgv_adapter *adapt, bool enable);
	void (*handle_page_fault)(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry);

	int (*register_interrupt)(struct amdgv_adapter *adapt);
	void (*unregister_interrupt)(struct amdgv_adapter *adapt);

	void (*start_gpu_timer)(struct amdgv_adapter *adapt, uint64_t micro_seconds);
};

struct amdgv_virtualized_interrupt_info {
	uint8_t table_entry_update;
	uint32_t *msix_tab;
};

struct amdgv_irqmgr {
	bool			    disable_parse_ih;
	struct oss_intr_regrt_info *intr_regrt_info;

	struct amdgv_ih_ring	     ih;
	const struct amdgv_ih_funcs *ih_funcs;

	spin_lock_t hv_event_lock;
	spin_lock_t ih_event_lock;
	spin_lock_t ih_handler3_lock;

	int (*hv_event_process)(struct amdgv_adapter *adapt);

	void *ih_submission_interrupt_context;
	void (*ih_submission_interrupt_handler)(void *context);

	void *ih_gpu_timer_context;
	void (*ih_gpu_timer_handler)(void *context);

	int (*disable_hw_interrupt)(struct amdgv_adapter *adapt);
	int (*enable_hw_interrupt)(struct amdgv_adapter *adapt);

	struct oss_bh_info ih_bh; /* ih bottom_half handle */
	uint32_t ih_queue_rptr; /* rptr ih queue */
	uint32_t ih_queue_wptr; /* wptr ih queue */
	struct amdgv_iv_entry *ih_queue; /* ih queue, default is 256 entries */

	void (*write_virtualized_interrupt)(struct amdgv_adapter *adapt, uint32_t vfIndex, uint32_t tableIndex, uint64_t messageAddress, uint32_t messageData, uint32_t vectorControl, bool is_direct_write);
	void (*update_virtualized_interrupt)(struct amdgv_adapter *adapt, uint32_t vfIndex);

	struct amdgv_virtualized_interrupt_info virtualized_interrupt_info_db[AMDGV_MAX_VF_NUM];
};

int amdgv_ih_ring_init(struct amdgv_adapter *adapt, unsigned ring_size, bool use_bus_addr);
int amdgv_ih_ring_set(struct amdgv_adapter *adapt);

void amdgv_ih_ring_fini(struct amdgv_adapter *adapt);

int amdgv_ih_iv_ring_entry_process(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry);

enum oss_irq_return amdgv_hv_event_handle(void *context);

void amdgv_ih_process_handle3(void *handle, void *context, void *arg1, void *arg2);
enum oss_irq_return amdgv_ih_process_handle2(void *context,
					     struct oss_interrupt_cb_info *cb_info);
enum oss_irq_return amdgv_ih_process_handle(void *context);
enum oss_irq_return amdgv_ras_fatal_error_handle(void *context);

int amdgv_irqmgr_iv_ring_enable(struct amdgv_adapter *adapt, bool enable);

int amdgv_irqmgr_mbox_enable(struct amdgv_adapter *adapt, bool enable);

int amdgv_irqmgr_toggle_interrupt(struct amdgv_adapter *adapt, bool enable);

void amdgv_irqmgr_dump_temperature(struct amdgv_adapter *adapt);

int amdgv_irqmgr_register_interrupt(struct amdgv_adapter *adapt);
int amdgv_irqmgr_unregister_interrupt(struct amdgv_adapter *adapt);

int amdgv_irqmgr_disable(struct amdgv_adapter *adapt);
int amdgv_irqmgr_sw_init(struct amdgv_adapter *adapt);
int amdgv_irqmgr_sw_fini(struct amdgv_adapter *adapt);

void amdgv_irqmgr_register_submission_interrupt_handler(struct amdgv_adapter *adapt, void *handler, void *context);
void amdgv_irqmgr_register_gpu_timer_handler(struct amdgv_adapter *adapt, void *handler, void *context);

void amdgv_irqmgr_enable_hw_interrupt(struct amdgv_adapter *adapt);
void amdgv_irqmgr_disable_hw_interrupt(struct amdgv_adapter *adapt);

void amdgv_irqmgr_decode_iv(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry);

int amdgv_irqmgr_write_virtualized_interrupt(struct amdgv_adapter *adapt, uint32_t vfIndex, uint32_t tableIndex,
		uint64_t messageAddress, uint32_t messageData, uint32_t vectorControl, bool is_direct_write);
void amdgv_irqmgr_update_virtualized_interrupt(struct amdgv_adapter *adapt, uint32_t vfIndex);

#endif
