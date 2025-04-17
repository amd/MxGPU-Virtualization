/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef __SMI_DRV_CMD_H__
#define __SMI_DRV_CMD_H__

int smi_dummy_cmd(struct smi_ctx *ctx, void *inb, void *outb,
			  uint16_t in_len, uint16_t out_len);
int smi_get_server_static_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_vbios_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_board_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_asic_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_vram_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_driver_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_power_cap_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_fb_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_cache_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_set_gpu_power_cap(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_fw_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_performance_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_is_power_management_enabled(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_vf_static_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_vf_dynamic_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_vf_partition_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_set_vf_partition_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_clear_vf_fb(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_create_event_set(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_ecc_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_ecc_block_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_query_bad_page_info(struct smi_ctx *ctx, void *inb,
		     void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_handle_id(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_guest_data(struct smi_ctx *ctx, void *inb,
		       void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_dfc_fw(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_pcie_info(struct smi_ctx *ctx, void *inb,
		      void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_ucode_err_records(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_vf_ucode_info(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_partition_profile_info(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_link_metrics(struct smi_ctx *ctx, void *inb,
			 void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_link_topology(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_xgmi_fb_sharing_caps(struct smi_ctx *ctx, void *inb,
				 void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_xgmi_fb_sharing_mode_info(struct smi_ctx *ctx, void *inb,
				      void *outb, uint16_t in_len, uint16_t out_len);
int smi_set_xgmi_fb_sharing_mode(struct smi_ctx *ctx, void *inb,
				 void *outb, uint16_t in_len, uint16_t out_len);
int smi_set_xgmi_fb_custom_sharing_mode(struct smi_ctx *ctx, void *inb,
				 void *outb, uint16_t in_len, uint16_t out_len);
int smi_read_event_set(struct smi_ctx *ctx, void *inb,
		       void *outb, uint16_t in_len, uint16_t out_len);
int smi_destroy_event_set(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_ras_feature_info(struct smi_ctx *ctx, void *inb,
			     void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_data(struct smi_ctx *ctx, void *inb,
		 void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_metrics_table(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_accelerator_partition_profile_config(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_memory_partition_caps(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_accelerator_partition_profile(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_memory_partition_config(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_set_accelerator_partition_setting(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_set_memory_partition_setting(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_gpu_driver_model(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_soc_pstate(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_set_soc_pstate(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len);
int smi_get_cper_error(struct smi_ctx *ctx, void *inb,
			    void *outb, uint16_t in_len, uint16_t out_len);

#endif // __SMI_DRV_CMD_H__
