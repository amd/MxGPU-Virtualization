/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef MI300_FRU_H
#define MI300_FRU_H

/* SMU I2C interface */
#define I2C_ADDR_SIZE	(0x2)
#define I2C_RM_DEV_ADDR (0x58 << 1)

/*
 * 80 bytes is the minimum recommended size for FRU Product Info section (v1.3)
 * Assume, largest field is maximum 80 bytes.
 */
#define I2C_CMD_BUFFER_SIZE 80

/* FRU product information area */
#define MI300_FRU_PRODUCT_INFO_HEADER	       (0x0004)
#define MI300_FRU_PRODUCT_INFO_SKIP_ADDR      (0x3)
#define MI300_FRU_PRODUCT_INFO_SIZE_MASK      (0x3F)
#define MI300_FRU_PRODUCT_INFO_ASSET_TAG_SIZE (0x3)

/* FRU multirecord area */
#define MI300_FRU_MULTIRECORD_HEADER	      (0x0005)
#define MI300_FRU_MULTIRECORD_ENTRY_SIZE     (0x5)
#define MI300_FRU_MULTIRECORD_ENTRY_END_MASK (0x80)

/* FRU AMD OEM record in multirecord area */
#define MI300_FRU_AMD_OEM_RECORD_ID		  (0xC0)

/* FRU AMD register offset from register base */
#define MI300_FRU_AMD_REG_PASSWORD_OFFSET	   (0x0002)
#define MI300_FRU_AMD_REG_PASSWORD_SIZE	   (0x8)
#define MI300_FRU_AMD_REG_PASSWORD_CONFIRM_OFFSET (0x000A)
#define MI300_FRU_AMD_REG_FUNCTION_AREA_OFFSET	   (0x001B)

/* FRU function area */
#define MI300_FRU_FUNCTION_AREA_END_MASK    (0x80)
#define MI300_FRU_FUNCTION_AREA_TYPE_MASK   (0x3F)
#define MI300_FRU_FUNCTION_AREA_LENGTH_SIZE (0x2)

/* FRU sigout in function area */
#define MI300_FRU_SIGOUT_FUNCTION_AREA_TYPE (0x13)
#define MI300_FRU_SIGOUT_ECC_KEYWORD	     "RAS EEPROM Error Injection"

#pragma pack(1)
/* FRU uses little endian, make sure below structs are correct */
struct mi300_fru_multirecord_header {
	uint8_t record_type_id;
	uint8_t record_eol_format_version;
	uint8_t record_length;
	uint8_t record_checksum;
	uint8_t header_checksum;
};

struct mi300_fru_rm_version {
	uint16_t version : 10;
	uint8_t release_stage : 2;
	uint8_t reserved : 4;
};

/* OEM record ID 0xC0 (mr_c0) */
struct mi300_fru_multirecord_c0 {
	struct mi300_fru_multirecord_header header;
	uint8_t manufacturer_id[3];
	struct mi300_fru_rm_version specification_version;
	struct mi300_fru_rm_version firmware_version;
	struct mi300_fru_rm_version hardware_version;
	uint16_t amd_registers_address_offset;
};

struct mi300_fru_sigout_header {
	uint8_t	 function_area_type;
	uint8_t	 function_area_revision;
	uint8_t	 function_area_id;
	uint16_t sub_area_length;
	uint8_t	 reserved_1;
	uint16_t settings_private_offset;
	uint16_t settings_public_static_offset;
	uint16_t settings_public_dynamic_offset;
	uint16_t log_offset;
	uint16_t end_of_section;
	uint8_t	 reserved_2[7];
	uint8_t	 checksum;
};

struct mi300_fru_sigout_static_content {
	uint16_t description_length;
	uint8_t	 id;
	uint8_t	 setting;
	char	 description[32];
	uint8_t	 location;
	uint8_t	 log_sensor_id;
	uint16_t end_of_section;
	uint8_t	 reserved[7];
	uint8_t	 checksum;
};

struct mi300_fru_sigout_dynamic_content {
	uint16_t setting_length;
	uint8_t	 id;
	uint8_t	 enable_live;
	uint8_t	 value_live;
	uint8_t	 duty_cycle_live;
	uint8_t	 enable_flash;
	uint8_t	 value_flash;
	uint8_t	 duty_cycle_flash;
	uint16_t end_of_section;
	uint8_t	 reserved[5];
};
#pragma pack()

/* order of FRU product info, may vary in future */
enum mi300_fru_product_order {
	FRU_PRODUCT_MANUFACTURER = 0,
	FRU_PRODUCT_NAME,
	FRU_PRODUCT_MODEL_NUM,
	FRU_PRODUCT_VERSION,
	FRU_PRODUCT_SERIAL,
	FRU_PRODUCT_ASSET_TAG,
	FRU_PRODUCT_FRU_ID,

	FRU_PRODUCT_MAX
};

int mi300_fru_get_product_info(struct amdgv_adapter *adapt);

#endif
