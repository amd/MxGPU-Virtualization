/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_PSP_H
#define AMDGV_PSP_H
//#include <amdgv_device.h>

/* All Trusted Applications have properties buffer starting at 2048-byte offset
 * from beginning of signed binary
 */
#define PSP_TA_PROP_OFFSET   2048
#define PSP_TA_PROP_VER_NAME "amd.ta.version"

#define MAX_PF_DB_SIZE				64
#define MAX_VF_DB_SIZE				(64 * 16)
#define ATTESTATION_TABLE_COOKIE	0x143b6a37

#define regMP0_C2PMSG_64 0x16080
#define regMP0_C2PMSG_69 0x16085
#define regMP0_C2PMSG_70 0x16086
#define regMP0_C2PMSG_71 0x16087

struct amdgv_live_info_psp;
struct amdgv_live_info_fw_info;

/* Forward declaration to avoid circular dependency with amdgv_oss_wrapper.h */
typedef void *mutex_t;

/* FW Attestation Record structure*/
struct FW_ATTESTATION_RECORD {
	uint16_t attestation_fw_id_v1; /*Legacy FW Type field */
	uint16_t attestation_fw_id_v2; /* V2 FW ID field */
	uint32_t attestation_fw_id_version; /* FW Version */
	uint16_t attestation_fw_active_func_id; /*The VF ID (only in VF Attestation Table */
	uint8_t  attestation_source; /* FW source indicator */
	uint8_t  record_valid; /* Indicated whether record is a valid entry */
	uint32_t attestation_fw_ta_id; /*Ta ID (only in TA Attestation Table */
};

struct ATTESTATION_DB_HEADER {
	uint32_t attestation_db_version;
	uint32_t attestation_db_cookie;
	struct FW_ATTESTATION_RECORD pf_table[MAX_PF_DB_SIZE];
	struct FW_ATTESTATION_RECORD vf_table[MAX_VF_DB_SIZE];
};

typedef union FW_RECORD_INFO{
    uint16_t W0;
    struct{
		uint16_t fw_record_type			: 4;
		uint16_t fw_src					: 4;
		uint16_t fw_dest				: 4;
		uint16_t reserved				: 2;
		uint16_t last_entry				: 1;
		uint16_t record_valid			: 1;
    } bit0;
} FW_RECORD_INFO;

typedef struct FWMAN_ENTRY_ID {
    uint16_t FwId;
    uint16_t VfId;
} FWMAN_ENTRY_ID;

typedef union FWMAN_ID {
    FWMAN_ENTRY_ID FwEntry;
    uint32_t TaEntry;
} FWMAN_ID;

struct FWMAN_RECORD{
    FWMAN_ID FwManId;
	uint32_t FwVer;
	FW_RECORD_INFO record_info;
	uint8_t FwHash[(sizeof(uint8_t) * 3)];
};

struct FWMAN_DB_HEADER {
	uint32_t attestation_db_version;
	uint32_t attestation_db_cookie;
	struct FWMAN_RECORD pf_table[MAX_PF_DB_SIZE];
	struct FWMAN_RECORD vf_table[MAX_VF_DB_SIZE];
};

struct psp_mb_status;

enum { PSP_KM_CMD_MAX_NUM = 16 }; /*16 CMD Buffers*/
enum { PSP_KM_CMD_INVALID_INDEX = 0xFFFFFFFF };
enum { MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT = 0x00000018 };

/* TEE (Trusted Execution Environment) error codes */
enum {
	PSP_KM_TEE_SUCCESS                 = 0x00000000,
	PSP_KM_TEE_ERROR_CANCEL            = 0xFFFF0002,
	PSP_KM_TEE_ERROR_BAD_PARAMETERS    = 0xFFFF0006,
	PSP_KM_TEE_ERROR_NOT_SUPPORTED     = 0xFFFF000A,
	PSP_KM_TEE_ERROR_AMD_SMU_TIMEOUT   = 0x80000306,
	PSP_KM_TEE_ERROR_AMD_PERF_HW_BUSY  = 0x80000313,
};

struct psp_fw_image_header {
	uint32_t reserved1[24];
	uint32_t image_version; // [0x60] Off Chip Firmware Version
	uint32_t reserved2[39];
};

enum psp_local_memory_size {
	PSP_PRIVATE_IMAGE_SIZE = 0x100000, // allocate 1MB  framebuffer from offset 0,
	PSP_RING_SIZE	       = 0x1000,   // allocate 4K   framebuffer from offset 1MB
	PSP_CMD_PAGE_SIZE      = 0x1000,   // allocate 4K   framebuffer from offset 1MB+4K, total size is 16*4K
	PSP_FENCE_SIZE	       = 0x1000,   // allocate 4K   framebuffer from offset 1MB+4K+16*4K
};

enum psp_local_memory_alignment {
	PSP_PRIVATE_IMAGE_ALIGNMENT = 0x100000,
	PSP_RING_ALIGNMENT	    = 0x1000,
	PSP_CMD_ALIGNMENT	    = 0x1000,
	PSP_FENCE_ALIGNMENT	    = 0x1000,
};

enum psp_status {
	PSP_STATUS__SUCCESS = 0,
	PSP_STATUS__ERROR_INVALID_PARAMS,
	PSP_STATUS__ERROR_GENERIC,
	PSP_STATUS__ERROR_OUT_OF_MEMORY,
	PSP_STATUS__ERROR_UNSUPPORTED_FEATURE,
	PSP_STATUS__ERROR_TIMEOUT,
	PSP_STATUS__ERROR_BUSY
};

/**
 * PTL (Peak TOPS Limiter) Data Format Types
 * Used to specify which data types should be optimized for peak performance
 */
enum psp_gfx_format_type {
	GFX_FTYPE_I8          = 0x00000000,
	GFX_FTYPE_F16         = 0x00000001,
	GFX_FTYPE_BF16        = 0x00000002,
	GFX_FTYPE_F32         = 0x00000003,
	GFX_FTYPE_F64         = 0x00000004,
	GFX_FTYPE_F8          = 0x00000005,
	GFX_FTYPE_VECTOR      = 0x00000006,
	GFX_FTYPE_INVALID     = 0xFFFFFFFF,
};

struct psp_gfx_cmd_req_perf_hw {
	uint32_t req;
	uint32_t ptl_state;
	uint32_t pref_format1;
	uint32_t pref_format2;
};

struct psp_gfx_cmd_resp_perf_hw {
	uint32_t resp;
	uint32_t ptl_state;
	uint32_t pref_format1;
	uint32_t pref_format2;
};

enum psp_ring_type {
	PSP_RING_TYPE__KM = 2,
};

enum psp_ih_reg {
	GC_MC_SYSTEM_APERTURE_HI  = 0x5,
	GC_MC_SYSTEM_APERTURE_LO  = 0x6,
	MM_MC_SYSTEM_APERTURE_HI  = 0x7,
	MM_MC_SYSTEM_APERTURE_LO  = 0x8,
	GC_MC_VM_FB_LOCATION_TOP  = 0x9,
	GC_MC_VM_FB_LOCATION_BASE = 0xA,
	MM_MC_VM_FB_LOCATION_TOP  = 0xB,
	MM_MC_VM_FB_LOCATION_BASE = 0xC,
	HDP_NONSURFACE_BASE_HI	  = 0xD,
	HDP_NONSURFACE_BASE	  = 0xE,

	// 2nd MMHUB instance
	GC_MC_VM_XGMI_GPUIOV_ENABLE = 23,
	MM_MC_VM_XGMI_GPUIOV_ENABLE = 24,

	// golden setting registers
	MM_MC_VM_MX_L1_TLB_CNTL = 25,

	// VM_IOMMU_CONTROL_REGISTER.IOMMUEN update by PSP
	VM_IOMMU_CONTROL_WA = 26,

	RCC_CONFIG_MEMSIZE = 27,
	RCC_CONFIG_RESERVED = 28,
};

struct psp_local_memory {
	uint32_t size;	    // in bytes
	uint32_t alignment; // in bytes
	union {
		struct {
			uint64_t mc_address;
			void	*virtual_address;
		};
		struct amdgv_memmgr_mem *mem;
	};
};

struct psp_cmd_km_buf {
	struct psp_local_memory cmd_mem; /* CMD buffer allocation handles */
	uint32_t		fence_value; /* Fence associated with CMD submission */
	bool			used; /* CMD buffer usage flag */
};

struct psp_ring {
	enum psp_ring_type	ring_type;
	struct psp_local_memory ring_mem;
	uint32_t		ring_rptr;
	uint32_t		ring_wptr;
};

struct psp_cmd_km_context {
	struct psp_cmd_km_buf	km_cmd_buf_pool[PSP_KM_CMD_MAX_NUM];
	struct psp_local_memory km_fence_mem_handle;
	uint32_t		next_avail_cmd_buf_index;
	uint32_t		km_fence_count;
	mutex_t lock;
};

struct psp_xgmi_context {
	struct psp_local_memory shared_buffer;
	bool			xgmi_initialized;
	uint32_t		xgmi_session_id;
	bool			supports_extended_data;
};

struct psp_asd_context {
	uint32_t asd_session_id;
};

struct psp_ras_context {
	bool			ras_initialized;
	uint32_t		ras_session_id;
	struct psp_local_memory shared_buffer;
	void *ras_shared_buffer;
	bool            set_init_flag;
	unsigned char *ras_bin_buf;
	uint32_t 		ras_bin_size;
	uint32_t ta_version;
};

struct psp_vbflash_context {
	struct psp_local_memory	shared_buffer;
	char *vbflash_tmp_buf;
	uint32_t vbflash_image_size;
	bool vbflash_done;
};

/** PSP runtime DB **/
#define PSP_RUNTIME_DB_SIZE_IN_BYTES		0x10000
#define PSP_RUNTIME_DB_OFFSET			0x100000
#define PSP_RUNTIME_DB_COOKIE_ID		0x0ed5
#define PSP_RUNTIME_DB_VER_1			0x0100
#define PSP_RUNTIME_DB_DIAG_ENTRY_MAX_COUNT	0x40

enum psp_runtime_entry_type {
	PSP_RUNTIME_ENTRY_TYPE_INVALID		= 0x0,
	PSP_RUNTIME_ENTRY_TYPE_TEST		= 0x1,
	PSP_RUNTIME_ENTRY_TYPE_MGPU_COMMON	= 0x2,  /* Common mGPU runtime data */
	PSP_RUNTIME_ENTRY_TYPE_MGPU_WAFL	= 0x3,  /* WAFL runtime data */
	PSP_RUNTIME_ENTRY_TYPE_MGPU_XGMI	= 0x4,  /* XGMI runtime data */
	PSP_RUNTIME_ENTRY_TYPE_BOOT_CONFIG	= 0x5,  /* Boot Config runtime data */
	PSP_RUNTIME_ENTRY_TYPE_PPTABLE_ERR_STATUS = 0x6, /* SCPM validation data */
};

/* PSP runtime DB header */
struct psp_runtime_data_header {
	/* determine the existence of runtime db */
	uint16_t cookie;
	/* version of runtime db */
	uint16_t version;
};

/* PSP runtime DB entry */
struct psp_runtime_entry {
	/* type of runtime db entry */
	uint32_t entry_type;
	/* offset of entry in bytes */
	uint16_t offset;
	/* size of entry in bytes */
	uint16_t size;
};


/* PSP runtime DB directory */
struct psp_runtime_data_directory {
	/* number of valid entries */
	uint16_t			entry_count;
	/* db entries*/
	struct psp_runtime_entry	entry_list[PSP_RUNTIME_DB_DIAG_ENTRY_MAX_COUNT];
};

/* PSP run time DB SCPM authentication defines */
enum psp_runtime_scpm_authentication {
	SCPM_DISABLE                     = 0x0,
	SCPM_ENABLE                      = 0x1,
	SCPM_ENABLE_WITH_SCPM_ERR        = 0x2,
};

enum psp_gfx_tee_version {
       GFX_TEE_VERSION_2   = 2,
       GFX_TEE_VERSION_3   = 3,
};

struct psp_runtime_scpm_entry {
	/* scpm error bits */
	/* DWORD 0 ~ 0 */
	uint32_t scpm_identity_token_authenticate_err               :  1;
	/*above bit is VBIOS identity token table error*/
	uint32_t scpm_authenticate_pptable_err                      :  1;
	uint32_t scpm_non_pptable_err                               :  1;
	/*above bits are VBIOS pptable error*/
	uint32_t scpm_non_board_err                                 :  1;
	/*above bit is BCM/VBL donot patch board table error*/
	uint32_t scpm_non_fuse_err                                  :  1;
	/*above bit is fuse donot set error*/
	uint32_t scpm_pad_bits                                      : 25;
	uint32_t scpm_enable_bits                                   :  2;
	/* value 0x00 is SCPM disable
	value 0x01 is SCPM enable
	value 0x10 is SCPM enable with error status
	*/
};

enum psp_migration_manifest_data_type {
	PSP_MIGRATION_INVALID = 0,
	PSP_MIGRATION_EXPORT_STATIC_DATA = 1,
	PSP_MIGRATION_EXPORT_DYNAMIC_DATA,
	/* static adata import */
	PSP_MIGRATION_IMPORT_STATIC_DATA,
	/* dynamic data import and final step */
	PSP_MIGRATION_IMPORT_DYNAMIC_DATA,
};

#define MAX_PSP_NUM		4	/* number of PSPs in MI300 etc*/

struct psp_context {
	struct psp_local_memory private_fw_memory;
	struct psp_ring km_ring[MAX_PSP_NUM];
	struct psp_cmd_km_context km_cmd_context;
	struct psp_cmd_km *psp_cmd_km_mem;
	struct psp_local_memory	  tmr_context;
	uint32_t		  allocated_tmr_size;
	struct psp_xgmi_context	  xgmi_context;
	struct psp_asd_context	  asd_context;
	struct psp_ras_context	  ras_context;
	struct psp_vbflash_context	  vbflash_context;

	void *attestation_db_cpu_addr;
	uint64_t attestation_db_gpu_addr;

	uint8_t	  fw_num;
	uint32_t *fw_info;

	enum psp_runtime_scpm_authentication scpm_status;

	struct dfc_fw *dfc_fw;
	uint32_t vf_relay_wtr_ptr[AMDGV_MAX_VF_NUM];
	/* Upper bound for vf_relay_wtr_ptr; 0 will cause all VF relay requests to be dropped. */
	uint32_t vf_relay_wtr_ptr_max;
	enum psp_gfx_tee_version tee_version;
	bool skip_ta_fw_version;
	mutex_t psp_gfx_cmd_lock;

	bool (*fw_id_support)(uint32_t fw_id);
	enum psp_status (*program_register)(struct amdgv_adapter *adapt, uint32_t idx_vf,
						uint32_t reg_value, uint32_t reg_value_hi,
						enum psp_ih_reg reg_id);
	uint32_t (*get_wptr)(struct amdgv_adapter *adapt);
	void (*set_wptr)(struct amdgv_adapter *adapt, uint32_t value);
	bool (*need_switch_to_pf)(struct amdgv_adapter *adapt);
	bool (*ras_need_switch_to_pf)(struct amdgv_adapter *adapt);
	enum psp_status (*vfgate_support)(struct amdgv_adapter *adapt);
	enum psp_status (*set_mb_int)(struct amdgv_adapter *adapt, uint32_t idx_vf,
					  bool enable);
	enum psp_status (*get_mb_int_status)(struct amdgv_adapter *adapt, uint32_t idx_vf,
					  struct psp_mb_status *mb_status);
	enum psp_status (*load_keydb)(struct amdgv_adapter *adapt, const unsigned char *fw_image,
					  uint32_t fw_image_size);
	enum psp_status (*load_spl)(struct amdgv_adapter *adapt, const unsigned char *fw_image,
					uint32_t fw_image_size);
	enum psp_status (*load_sysdrv)(struct amdgv_adapter *adapt, const unsigned char *fw_image,
					   uint32_t fw_image_size);
	enum psp_status (*load_rasdrv)(struct amdgv_adapter *adapt, const unsigned char *fw_image,
					   uint32_t fw_image_size);
	enum psp_status (*load_sosdrv)(struct amdgv_adapter *adapt, const unsigned char *fw_image,
					   uint32_t fw_image_size);
	enum psp_status (*clear_vf_fw)(struct amdgv_adapter *adapt,
					  uint32_t idx_vf);
	enum psp_status (*load_psp_ucode)(struct amdgv_adapter *adapt, const unsigned char *fw_image,
					   uint32_t fw_image_size, uint32_t fw_id);
	enum psp_status (*fw_attestation_support)(struct amdgv_adapter *adapt);
	enum psp_status (*get_fw_attestation_info)(struct amdgv_adapter *adapt, uint32_t vf_id);
	enum psp_status (*parse_psp_info)(struct amdgv_adapter *adapt);
	uint32_t idx;
	enum psp_status (*update_spirom)(struct amdgv_adapter *adapt);
	enum psp_status (*vbflash_status)(struct amdgv_adapter *adapt, uint32_t *status);

	int (*psp_ring_destroy)(struct amdgv_adapter *adapt);
	enum psp_status (*psp_program_guest_mc_settings)(struct amdgv_adapter *adapt,
							 uint32_t idx_vf);
	enum psp_status (*vf_relay)(struct amdgv_adapter *adapt, uint32_t vf_id);
	enum psp_status (*load_asd_fw_to_mem)(struct amdgv_adapter *adapt,
		struct psp_local_memory *asd_bin_mem, uint32_t *size);
	enum psp_status (*copy_vf_chiplet_regs)(struct amdgv_adapter *adapt,
							uint32_t idx_vf);
	enum psp_status(*dfc_check_guest_version)(struct amdgv_adapter *adapter,
		char *driver_version);
	enum psp_status(*tmr_init)(struct amdgv_adapter *adapt, uint32_t allocated_tmr_size);
	void (*get_xgmi_fw_info)(struct amdgv_adapter *adapt, unsigned const char **fw_image,
							uint32_t *fw_image_size);
	enum psp_status (*transfer_manifest_data)(struct amdgv_adapter *adapt,
						uint32_t idx_vf, uint64_t data_addr, uint32_t size,
						enum psp_migration_manifest_data_type type);
	enum psp_status (*get_migration_info)(struct amdgv_adapter *adapt);
	enum psp_status (*migration_rlc_autoload)(struct amdgv_adapter *adapt, uint32_t idx_vf);
	int (*enable_interrupt)(struct amdgv_adapter *adapt, bool enable);
	int (*handle_irq)(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry);
	bool (*vf_cp_migration_is_supported)(struct amdgv_adapter *adapt, uint32_t idx_vf);
};

/* Single property buffer stored in the APP_PROP_BUF structure.
 * The uint8_t type of the Size implies that single property can be
 * max 256 bytes including property name and value.
 */
struct app_property {
	uint8_t Size;	   // total size of property in bytes
	uint8_t Type;	   // property type
	uint8_t NameSize;  // size of property name in bytes
			   // including zero terminator
	uint8_t ValueSize; // size of property value in bytes
	uint8_t Name[1];   // variable size property name
			   // The property value is following the name
};

// Structure containing multiple property buffers of variable size.
//
struct app_prop_buff {
	uint32_t	    Size;	 // total size of buffer in bytes
	uint32_t	    PropCount;	 // number of properties in buffer
	uint32_t	    Reserved[2]; //
	struct app_property Property[1]; // first property in structure
};

enum psp_status amdgv_psp_get_migration_data_size(struct amdgv_adapter *adapt,
				uint32_t pkg_type, uint32_t *data_size);
enum psp_status amdgv_psp_check_migration_data_sizes(struct amdgv_adapter *adapt);

enum amdgv_live_info_status amdgv_psp_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_psp *psp_info);
enum amdgv_live_info_status amdgv_psp_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_psp *psp_info);
enum amdgv_live_info_status amdgv_psp_fw_info_export_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_fw_info *fw_info);
enum amdgv_live_info_status amdgv_psp_fw_info_import_live_data(struct amdgv_adapter *adapt, struct amdgv_live_info_fw_info *fw_info);
enum psp_status amdgv_psp_read_ip_discovery(struct amdgv_adapter *adapt,
						struct amdgv_memmgr_mem *ip_mem);
#define amdgv_psp_transfer_manifest_data(adapt, idx_vf, data_addr, size, type) \
		((adapt->psp.transfer_manifest_data) ? \
		 adapt->psp.transfer_manifest_data(adapt, idx_vf, data_addr, size, type) : \
		 PSP_STATUS__ERROR_UNSUPPORTED_FEATURE)

/*
 * psp_ring_type_to_gpuv_psp_ring_type
 * @ring_type:	ring type
 *
*/
enum gpuv_psp_ring_type psp_ring_type_to_gpuv_psp_ring_type(enum psp_ring_type ring_type);

#endif
