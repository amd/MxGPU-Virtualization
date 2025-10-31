/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __SMI_NIC_UTILS_H__
#define __SMI_NIC_UTILS_H__

#include "amdsmi.h"
#include "smi_nic_interface.h"

/**
 * @brief Maps an enum representing a NIC library status to an enum representing AMDSMI library status.
 *
 * This function takes an enum value representing a NIC library status and maps it to an
 * equivalent enum value representing an AMDSMI library status. The mapping is performed based on
 * predefined set of rules. If the provided NIC library status does not match any of the
 * predefined values, it is mapped to AMDSMI_STATUS_MAP_ERROR.
 *
 * @param[in] unit The enum value representing NIC library status.
 * @return The enum value representing the corresponding mapped AMDSMI library status.
 *
 * @note This function assumes that the enum values for both enums (amdsmi_status_t and smi_nic_status)
 * are compatible and represent similar concepts.
*/
amdsmi_status_t smi_map_nic_status(smi_nic_status status);


#endif // __SMI_NIC_UTILS_H__
