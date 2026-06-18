/*
 * Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <unordered_set>

namespace smi_cli_nic_default_stats {

// Default human-display subset for `amd-smi metric --nic --port` when --extended
// is NOT passed. Each set is a strict subset of the corresponding library
// whitelist in nic/inc/smi_nic_stats.h. JSON / CSV output is never filtered by
// these sets - it always returns whatever the library returned.
//
// These lists are maintained independently of the library whitelist on purpose:
// the library list is the universe of stats SMI is allowed to surface; this
// header is the CLI's opinion on what is most useful at a glance.

inline const std::unordered_set<std::string> pensando_default_display = {
	"frames_rx_ok",
	"frames_rx_all",
	"frames_rx_bad_fcs",
	"frames_rx_bad_all",
	"octets_rx_ok",
	"octets_rx_all",
	"frames_rx_unicast",
	"frames_rx_multicast",
	"frames_rx_broadcast",
	"frames_rx_pause",
	"frames_rx_bad_length",
	"frames_rx_undersized",
	"frames_rx_oversized",
	"frames_rx_fragments",
	"frames_rx_jabber",
	"frames_rx_pripause",
	"frames_rx_stomped_crc",
	"frames_rx_too_long",
	"frames_rx_vlan_good",
	"frames_rx_dropped",
	"frames_tx_ok",
	"frames_tx_all",
	"frames_tx_bad",
	"octets_tx_ok",
	"octets_tx_total",
	"frames_tx_unicast",
	"frames_tx_multicast",
	"frames_tx_broadcast",
	"frames_tx_pause",
	"frames_tx_pripause",
	"frames_tx_vlan",
	"frames_tx_truncated",
};

inline const std::unordered_set<std::string> broadcom_default_display = {
	"rx_pfc_frames",
	"tx_pfc_frames",
	"rx_total_ring_discards",
	"tx_total_ring_discards",
	"continuous_roce_pause_events",
	"rx_fec_corrected_blocks",
	"rx_fec_uncorrectable_blocks",
	"rx_bytes_cos0",
	"rx_bytes_cos1",
	"rx_bytes_cos2",
	"rx_bytes_cos3",
	"rx_bytes_cos4",
	"rx_bytes_cos5",
	"rx_bytes_cos6",
	"rx_bytes_cos7",
	"tx_bytes_cos0",
	"tx_bytes_cos1",
	"tx_bytes_cos2",
	"tx_bytes_cos3",
	"tx_bytes_cos4",
	"tx_bytes_cos5",
	"tx_bytes_cos6",
	"tx_bytes_cos7",
};

} // namespace smi_cli_nic_default_stats
