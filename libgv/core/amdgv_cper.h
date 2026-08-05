/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_CPER_H
#define AMDGV_CPER_H

#include "amd_cper.h"
#include "amdgv_list.h"

#define CPER_MAX_COUNT			(adapt->cper.max_count)

#define HDR_LEN				(sizeof(struct cper_hdr))
#define SEC_DESC_LEN			(sizeof(struct cper_sec_desc))

#define BOOT_SEC_LEN			(sizeof(struct cper_sec_crashdump_boot))
#define FATAL_SEC_LEN			(sizeof(struct cper_sec_crashdump_fatal))
#define NONSTD_SEC_LEN			(sizeof(struct cper_sec_nonstd_err))

#define SEC_DESC_OFFSET(idx)		(HDR_LEN + (SEC_DESC_LEN * idx))

#define BOOT_SEC_OFFSET(count, idx)	(HDR_LEN + (SEC_DESC_LEN * count) + (BOOT_SEC_LEN * idx))
#define FATAL_SEC_OFFSET(count, idx)	(HDR_LEN + (SEC_DESC_LEN * count) + (FATAL_SEC_LEN * idx))
#define NONSTD_SEC_OFFSET(count, idx)	(HDR_LEN + (SEC_DESC_LEN * count) + (NONSTD_SEC_LEN * idx))

/* Move rptr forward to the first still-cached CPER entry.
 *
 * wptr and rptr are uint64_t. Clamp rptr to wptr first so that, for an
 * out-of-window rptr > wptr, neither (wptr - rptr) nor (wptr - CPER_MAX_COUNT)
 * underflows (CWE-191). Callers must also clamp rptr (see amdgv_cper.c) as the
 * underflowing subtractions around this macro live in the caller.
 */
#define CPER_MOVE_TO_FIRST_VALID(rptr) \
	(((rptr) > adapt->cper.wptr) ? adapt->cper.wptr : \
	 (((adapt->cper.wptr - (rptr)) > CPER_MAX_COUNT) ? \
	  adapt->cper.wptr - CPER_MAX_COUNT : (rptr)))

enum amdgv_cper_type {
	AMDGV_CPER_TYPE_RUNTIME,
	AMDGV_CPER_TYPE_FATAL,
	AMDGV_CPER_TYPE_BOOT,
	AMDGV_CPER_TYPE_BP_THR,
	AMDGV_CPER_TYPE_MAX,
};

struct amdgv_cper {
	bool enabled;
	atomic_t next_uid;
	uint64_t max_count;
	/* Lifetime CPERs generated */
	uint64_t count;

	uint64_t wptr;
	void *ring[AMDGV_CPER_MAX_ALLOWED_COUNT];
};

void amdgv_cper_entry_fill_hdr(struct amdgv_adapter *adapt,
			       struct cper_hdr *hdr,
			       enum amdgv_cper_type type,
			       enum cper_error_severity sev);
int amdgv_cper_entry_fill_fatal_section(struct amdgv_adapter *adapt,
					struct cper_hdr *hdr,
					uint32_t idx,
					struct cper_sec_crashdump_reg_data reg_data);
int amdgv_cper_entry_fill_boot_section(struct amdgv_adapter *adapt,
				       struct cper_hdr *hdr,
				       uint32_t idx,
				       uint64_t *msg,
				       uint32_t msg_count);
int amdgv_cper_entry_fill_runtime_section(struct amdgv_adapter *adapt,
					  struct cper_hdr *hdr,
					  uint32_t idx,
					  enum cper_error_severity sev,
					  uint32_t *reg_dump,
					  uint32_t reg_count);
int amdgv_cper_entry_fill_bad_page_thr_section(struct amdgv_adapter *adapt,
					       struct cper_hdr *hdr,
					       uint32_t section_idx,
					       uint32_t oam_id);

int amdgv_cper_commit_entry(struct amdgv_adapter *adapt,
			    struct cper_hdr *hdr);

struct cper_hdr *amdgv_cper_alloc_entry(struct amdgv_adapter *adapt,
					enum amdgv_cper_type type,
					uint16_t section_count);

int amdgv_cper_sw_init(struct amdgv_adapter *adapt);
int amdgv_cper_sw_fini(struct amdgv_adapter *adapt);

int amdgv_cper_get_count(struct amdgv_adapter *adapt,
			 uint64_t rptr, uint64_t *wptr,
			 uint64_t *avail_count,
			 uint64_t *size);
int amdgv_cper_get_entries(struct amdgv_adapter *adapt, uint64_t rptr,
			   void *buf, uint64_t buf_size,
			   uint64_t *write_count,
			   uint64_t *overflow_count,
			   uint64_t *left_size);


struct cper_hdr *amdgv_cper_get_ring_entry(struct amdgv_adapter *adapt, uint64_t rptr);
enum amdgv_cper_type amdgv_cper_get_type(struct amdgv_adapter *adapt, struct cper_hdr *hdr, uint32_t idx);

void amdgv_cper_get_crashdump(struct amdgv_adapter *adapt, struct cper_hdr *hdr, uint32_t idx,
			      struct cper_sec_crashdump_reg_data **reg_data);
void amdgv_cper_get_runtime_reg_dump(struct amdgv_adapter *adapt, struct cper_hdr *hdr, uint32_t idx,
				     uint32_t **reg_dump);

int amdgv_cper_patch_to_vf(struct amdgv_adapter *adapt, uint32_t idx_vf,
			   struct cper_hdr *hdr,
			   bool *allowed_sections, uint32_t allowed_count,
			   uint64_t *fb_offset,
			   uint32_t *checksum);
int amdgv_cper_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_cper *cper);
int amdgv_cper_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_cper *cper);
#endif
