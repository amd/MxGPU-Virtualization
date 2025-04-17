/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __SMI_DRV_UTILS_H__
#define __SMI_DRV_UTILS_H__

#include <smi_drv_oss.h>
#include "smi_drv_types.h"
#include "smi_drv_core.h"
#include "smi_drv_core_api.h"
#include "amdgv_gpumon.h"

/* Bad page timestamp format
 * yy[31:27] mm[26:23] day[22:17] hh[16:12] mm[11:6] ss[5:0]
 */
#define EEPROM_TIMESTAMP_MINUTE  6
#define EEPROM_TIMESTAMP_HOUR    12
#define EEPROM_TIMESTAMP_DAY     17
#define EEPROM_TIMESTAMP_MONTH   23
#define EEPROM_TIMESTAMP_YEAR    27
#define IS_LEAP_YEAR(x) ((x % 4 == 0 && x % 100 != 0) || x % 400 == 0)

/**
 * @brief Converts an EEPROM timestamp to UTC format.
 *
 * This function takes a timestamp from an EEPROM and converts it to a UTC timestamp.
 * The EEPROM timestamp is expected to be in a specific format where different parts
 * of the timestamp (year, month, day, hour, minute, second) are stored in specific
 * bit positions.
 *
 * @param eeprom_timestamp The timestamp from the EEPROM.
 * @return The corresponding UTC timestamp.
 *
 */
uint64_t smi_eeprom_to_utc_format(uint64_t eeprom_timestamp);

/**
 * @brief Generates a time string representing the given microseconds.
 *
 * This function takes a time value in microseconds and generates a formatted time string
 * representing the hours, minutes, seconds, and milliseconds. The formatted time string
 * is stored in the provided buffer.
 *
 * @param[out] buf Pointer to the buffer where the generated time string will be stored.
 * @param[in] microsec The time value in microseconds.
 *
 * @return None
 *
 * @note The generated time string format is "HH:MM:SS.MSC", where:
 *       - HH represents hours (00-23)
 *       - MM represents minutes (00-59)
 *       - SS represents seconds (00-59)
 *       - MSC represents milliseconds (000-999)
 *
 *       The buffer pointed to by 'buf' must have enough space to accommodate the generated
 *       time string. The maximum length of the time string, including the null terminator,
 *       is defined by SMI_MAX_DATE_LENGTH.
 *
 *       This function utilizes integer division and modulo operations to calculate hours,
 *       minutes, seconds, and milliseconds from the provided microseconds value.
 *       It then formats the time components into the specified format and stores the result
 *       in the buffer.
 */
void smi_generate_time_string(char *buf, uint64_t microsec);

/**
 * @brief Maps an enum representing a LibGV ucode/firmware ID to an enum representing an SMI ucode/firmware ID.
 *
 * This function takes an enum value representing a LibGV ucode/firmware ID and maps it to an
 * equivalent enum value representing an SMI ucode/firmware ID. The mapping is performed based on
 * predefined set of rules. If the provided LibGV ucode/firmware ID does not match any of the
 * predefined values, it is mapped to SMI_FW_ID__MAX.
 *
 * @param[in] ucode_id The enum value representing LibGV ucode/firmware ID.
 * @return The enum value representing the corresponding mapped SMI ucode/firmware ID.
 *
 * @note This function assumes that the enum values for both enums (smi_fw_block and amdgv_firmware_id)
 * are compatible and represent similar concepts.
 */
enum smi_fw_block smi_ucode_amdgv_to_smi(enum amdgv_firmware_id ucode_id);

/**
 * @brief Maps an enum representing an SMI framebuffer sharing mode to an enum representing a LibGV framebuffer sharing mode.
 *
 * This function takes an enum value representing an SMI framebuffer sharing mode and maps it to an
 * equivalent enum value representing a LibGV framebuffer sharing mode. The mapping is performed based on
 * predefined set of rules. If the provided SMI framebuffer sharing mode does not match any of the
 * predefined values, it is mapped to SMI_XGMI_FB_SHARING_MODE_UNKNOWN.
 *
 * @param[in] mode The enum value representing SMI framebuffer sharing mode.
 * @return The enum value representing the corresponding mapped LibGV framebuffer sharing mode.
 *
 * @note This function assumes that the enum values for both enums (amdgv_gpumon_xgmi_fb_sharing_mode and smi_xgmi_fb_sharing_mode)
 * are compatible and represent similar concepts.
 */
enum amdgv_gpumon_xgmi_fb_sharing_mode smi_map_fb_sharing_mode(enum smi_xgmi_fb_sharing_mode mode);

/**
 * @brief Maps an enum representing an SMI memory partition mode to an enum representing a LibGV memory mode.
 *
 * This function takes an enum value representing an SMI memory partition mode and maps it to an
 * equivalent enum value representing a LibGV memory partition mode. The mapping is performed based on
 * predefined set of rules. If the provided SMI memory partition mode does not match any of the
 * predefined values, it is mapped to AMDGV_MEMORY_PARTITION_MODE_UNKNOWN.
 *
 * @param[in] mode The enum value representing SMI memory partition mode.
 * @return The enum value representing the corresponding mapped LibGV memory partition mode.
 *
 * @note This function assumes that the enum values for both enums (enum smi_memory_partition_type and amdgv_memory_partition_mode)
 * are compatible and represent similar concepts.
 */
enum amdgv_memory_partition_mode smi_map_memory_partition_mode(enum smi_memory_partition_type mode);

/**
 * @brief Maps an enum representing a LibGV link status to an enum representing an SMI link status.
 *
 * This function takes an enum value representing a LibGV link status and maps it to an
 * equivalent enum value representing an SMI link status. The mapping is performed based on
 * predefined set of rules. If the provided LibGV link status does not match any of the
 * predefined values, it is mapped to SMI_LINK_STATUS_ERROR.
 *
 * @param[in] amdgv_link_status The enum value representing LibGV link status.
 * @return The enum value representing the corresponding mapped SMI link status.
 *
 * @note This function assumes that the enum values for both enums (smi_link_status and amdgv_link_status)
 * are compatible and represent similar concepts.
 */
enum smi_link_status smi_map_link_status(enum amdgv_gpumon_link_status amdgv_link_status);

/**
 * @brief Maps an enum representing a LibGV link type to an enum representing an SMI link type.
 *
 * This function takes an enum value representing a LibGV link type and maps it to an
 * equivalent enum value representing an SMI link type. The mapping is performed based on
 * predefined set of rules. If the provided LibGV link type does not match any of the
 * predefined values, it is mapped to SMI_LINK_TYPE_UNKNOWN.
 *
 * @param[in] amdgv_link_type The enum value representing LibGV link type.
 * @return The enum value representing the corresponding mapped SMI link type.
 *
 * @note This function assumes that the enum values for both enums (smi_link_type and amdgv_gpumon_link_type)
 * are compatible and represent similar concepts.
 */
enum smi_link_type smi_map_link_type(enum amdgv_gpumon_link_type amdgv_link_type);

/**
 * @brief Maps an enum representing a LibGV card form factor to an enum representing an SMI card form factor.
 *
 * This function takes an enum value representing a LibGV card form factor and maps it to an
 * equivalent enum value representing an SMI card form factor. The mapping is performed based on
 * predefined set of rules. If the provided LibGV card form factor does not match any of the
 * predefined values, it is mapped to SMI_CARD_FORM_FACTOR_UNKNOWN.
 *
 * @param[in] type The enum value representing LibGV card form factor.
 * @return The enum value representing the corresponding mapped SMI card form factor.
 *
 * @note This function assumes that the enum values for both enums (smi_card_form_factor and amdgv_gpumon_card_form_factor)
 * are compatible and represent similar concepts.
 */
enum smi_card_form_factor smi_map_card_form_factor(enum amdgv_gpumon_card_form_factor type);

/**
 * @brief Maps an enum representing an SMI GPU block to an enum representing a LibGV GPU block.
 *
 * This function takes an enum value representing an SMI GPU block and maps it to an
 * equivalent enum value representing a LibGV GPU block. The mapping is performed based on
 * predefined set of rules. If the provided SMI GPU block does not match any of the
 * predefined values, it is mapped to AMDGV_SMI_NUM_BLOCK_MAX.
 *
 * @param[in] block The enum value representing SMI GPU block.
 * @return The enum value representing the corresponding mapped LibGV GPU block.
 *
 * @note This function assumes that the enum values for both enums (amdgv_smi_ras_block and smi_gpu_block)
 * are compatible and represent similar concepts.
 */
enum amdgv_smi_ras_block smi_map_gpu_block(enum smi_gpu_block block);

/**
 * @brief Maps an enum representing a LibGV vram type to an enum representing an SMI vram type.
 *
 * This function takes an enum value representing a LibGV vram type and maps it to an
 * equivalent enum value representing an SMI vram type. The mapping is performed based on
 * predefined set of rules. If the provided LibGV vram type does not match any of the
 * predefined values, it is mapped to SMI_VRAM_TYPE_UNKNOWN.
 *
 * @param[in] type The enum value representing LibGV vram type.
 * @return The enum value representing the corresponding mapped SMI vram type.
 *
 * @note This function assumes that the enum values for both enums (smi_vram_type and amdgv_gpumon_vram_type)
 * are compatible and represent similar concepts.
 */
enum smi_vram_type smi_map_vram_type(enum amdgv_gpumon_vram_type type);

/**
 * @brief Maps an enum representing a LibGV vram vendor to an enum representing an SMI vram vendor.
 *
 * This function takes an enum value representing a LibGV vram vendor and maps it to an
 * equivalent enum value representing an SMI vram vendor. The mapping is performed based on
 * predefined set of rules. If the provided LibGV vram vendor does not match any of the
 * predefined values, it is mapped to SMI_VRAM_VENDOR_UNKNOWN.
 *
 * @param[in] vendor The enum value representing LibGV vram vendor.
 * @return The enum value representing the corresponding mapped SMI vram vendor.
 *
 * @note This function assumes that the enum values for both enums (smi_vram_vendor and amdgv_gpumon_vram_vendor)
 * are compatible and represent similar concepts.
 */
enum smi_vram_vendor smi_map_vram_vendor(enum amdgv_gpumon_vram_vendor vendor);

/**
 * @brief Comparison function for sorting devices by BDF.
 *
 * @param[in] a Pointer to the first smi_device_data structure.
 * @param[in] b Pointer to the second smi_device_data structure.
 * @return A negative value if the value of the BDF in 'a' is less than that in 'b',
 *         zero if they are equal, and a positive value if the value of the BDF in 'a' is greater than that in 'b'.
 *
 * @note This function assumes that the 'bdf' field within the smi_device_data structure
 *       holds a value that is suitable for comparison.
 *
 */
int smi_compare_dev_bdf(const void *a, const void *b);

/**
 * @brief Maps an enum representing a LibGV metric category to an enum representing an SMI metric category.
 *
 * This function takes an enum value representing a LibGV metric category and maps it to an
 * equivalent enum value representing an SMI metric category. The mapping is performed based on
 * predefined set of rules. If the provided LibGV metric category does not match any of the
 * predefined values, it is mapped to SMI_METRIC_CATEGORY_UNKNOWN.
 *
 * @param[in] category The enum value representing LibGV metric category.
 * @return The enum value representing the corresponding mapped SMI metric category.
 *
 * @note This function assumes that the enum values for both enums (smi_metric_category and amdgv_gpumon_metric_ext_category)
 * are compatible and represent similar concepts.
 */
enum smi_metric_category smi_map_metric_category(enum amdgv_gpumon_metric_ext_category category);

/**
 * @brief Maps an enum representing a LibGV metric name to an enum representing an SMI metric name.
 *
 * This function takes an enum value representing a LibGV metric name and maps it to an
 * equivalent enum value representing an SMI metric name. The mapping is performed based on
 * predefined set of rules. If the provided LibGV metric name does not match any of the
 * predefined values, it is mapped to SMI_METRIC_NAME_UNKNOWN.
 *
 * @param[in] name The enum value representing LibGV metric name.
 * @return The enum value representing the corresponding mapped SMI metric name.
 *
 * @note This function assumes that the enum values for both enums (smi_metric_name and amdgv_gpumon_metric_ext_name)
 * are compatible and represent similar concepts.
 */
enum smi_metric_name smi_map_metric_name(enum amdgv_gpumon_metric_ext_name name);

/**
 * @brief Maps an enum representing a LibGV metric unit to an enum representing an SMI metric unit.
 *
 * This function takes an enum value representing a LibGV metric unit and maps it to an
 * equivalent enum value representing an SMI metric unit. The mapping is performed based on
 * predefined set of rules. If the provided LibGV metric unit does not match any of the
 * predefined values, it is mapped to SMI_METRIC_UNIT_UNKNOWN.
 *
 * @param[in] unit The enum value representing LibGV metric unit.
 * @return The enum value representing the corresponding mapped SMI metric unit.
 *
 * @note This function assumes that the enum values for both enums (smi_metric_unit and amdgv_gpumon_metric_ext_unit)
 * are compatible and represent similar concepts.
 */
enum smi_metric_unit smi_map_metric_unit(enum amdgv_gpumon_metric_ext_unit unit);

/**
 * @brief Maps an enum representing a LibGV memory partition mode to an enum representing an SMI memory partition mode.
 *
 * This function takes an enum value representing a LibGV memory partition mode and maps it to an
 * equivalent enum value representing an SMI memory partition mode. The mapping is performed based on
 * predefined set of rules. If the provided LibGV memory partition mode does not match any of the
 * predefined values, it is mapped to SMI_MEMORY_PARTITION_UNKNOWN.
 *
 * @param[in] unit The enum value representing LibGV memory partition mode.
 * @return The enum value representing the corresponding mapped SMI memory partition mode.
 *
 * @note This function assumes that the enum values for both enums (smi_memory_partition_type and amdgv_memory_partition_mode)
 * are compatible and represent similar concepts.
 */
enum smi_memory_partition_type smi_map_mp_mode(enum amdgv_memory_partition_mode mode);

/**
 * @brief Maps an enum representing a LibGV partition type to an enum representing an SMI partition type.
 *
 * This function takes an enum value representing a LibGV partition type and maps it to an
 * equivalent enum value representing an SMI partition type. The mapping is performed based on
 * predefined set of rules. If the provided LibGV partition type does not match any of the
 * predefined values, it is mapped to SMI_ACCELERATOR_PARTITION_INVALID.
 *
 * @param[in] unit The enum value representing LibGV partition type.
 * @return The enum value representing the corresponding mapped SMI partition type.
 *
 * @note This function assumes that the enum values for both enums (smi_accelerator_partition_type and amdgv_gpumon_acccelerator_partition_type)
 * are compatible and represent similar concepts.
 */
enum smi_accelerator_partition_type smi_map_partition_type(enum amdgv_gpumon_acccelerator_partition_type type);

/**
 * @brief Maps an enum representing a LibGV resource type to an enum representing an SMI resource type.
 *
 * This function takes an enum value representing a LibGV resource type and maps it to an
 * equivalent enum value representing an SMI resource type. The mapping is performed based on
 * predefined set of rules. If the provided LibGV resource type does not match any of the
 * predefined values, it is mapped to SMI_ACCELERATOR_MAX.
 *
 * @param[in] unit The enum value representing LibGV resource type.
 * @return The enum value representing the corresponding mapped SMI resource type.
 *
 * @note This function assumes that the enum values for both enums (smi_accelerator_partition_resource_type and amdgv_gpumon_accelerator_partition_resource_type)
 * are compatible and represent similar concepts.
 */
enum smi_accelerator_partition_resource_type smi_map_resource_type(enum amdgv_gpumon_accelerator_partition_resource_type type);

/*
*/
enum smi_vf_sched_state smi_map_sched_state(enum amdgv_sched_state state);

#endif // __SMI_DRV_UTILS_H__
