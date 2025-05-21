/*
 * Copyright (C) 2021  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef NAVI_31_6MCD_OFFSET_H
#define NAVI_31_6MCD_OFFSET_H


#define MAX_INSTANCE                                        6
#define MAX_SEGMENT                                         6


struct IP_BASE_INSTANCE
{
    unsigned int segment[MAX_SEGMENT];
};

struct IP_BASE
{
    struct IP_BASE_INSTANCE instance[MAX_INSTANCE];
};


static const struct IP_BASE ATHUB_BASE = { { { { 0x00000C00, 0x02408C00, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE CLK_BASE = { { { { 0x00016C00, 0x02401800, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE CLK_MCD_BASE = { { { { 0x0C000400, 0x0C021000, 0, 0, 0, 0 } },
                                        { { 0x0C800400, 0x0C821000, 0, 0, 0, 0 } },
                                        { { 0x0D000400, 0x0D021000, 0, 0, 0, 0 } },
                                        { { 0x0D800400, 0x0D821000, 0, 0, 0, 0 } },
                                        { { 0x0E000400, 0x0E021000, 0, 0, 0, 0 } },
                                        { { 0x0E800400, 0x0E821000, 0, 0, 0, 0 } } } };
static const struct IP_BASE DF_BASE = { { { { 0x00007000, 0x00C00000, 0x0240B800, 0x07C00000, 0x12400000, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE DF_MCD_BASE = { { { { 0x0C000000, 0x0C021800, 0x0C022000, 0x0C2C0000, 0x0C300000, 0 } },
                                        { { 0x0C800000, 0x0C821800, 0x0C822000, 0x0CAC0000, 0x0CB00000, 0 } },
                                        { { 0x0D000000, 0x0D021800, 0x0D022000, 0x0D2C0000, 0x0D300000, 0 } },
                                        { { 0x0D800000, 0x0D821800, 0x0D822000, 0x0DAC0000, 0x0DB00000, 0 } },
                                        { { 0x0E000000, 0x0E021800, 0x0E022000, 0x0E2C0000, 0x0E300000, 0 } },
                                        { { 0x0E800000, 0x0E821800, 0x0E822000, 0x0EAC0000, 0x0EB00000, 0 } } } };
static const struct IP_BASE DIO_BASE = { { { { 0x02404000, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE DCN_BASE = { { { { 0x00000012, 0x000000C0, 0x000034C0, 0x00009000, 0x02403C00, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE DPCS_BASE = { { { { 0x00000012, 0x000000C0, 0x000034C0, 0x00009000, 0x02403C00, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE FUSE_BASE = { { { { 0x00017400, 0x02401400, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE FUSE_MCD_BASE = { { { { 0x0C002C00, 0x0C020600, 0, 0, 0, 0 } },
                                        { { 0x0C802C00, 0x0C820600, 0, 0, 0, 0 } },
                                        { { 0x0D002C00, 0x0D020600, 0, 0, 0, 0 } },
                                        { { 0x0D802C00, 0x0D820600, 0, 0, 0, 0 } },
                                        { { 0x0E002C00, 0x0E020600, 0, 0, 0, 0 } },
                                        { { 0x0E802C00, 0x0E820600, 0, 0, 0, 0 } } } };
static const struct IP_BASE GC_BASE = { { { { 0x00001260, 0x0000A000, 0x0001C000, 0x02402C00, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE HDA_BASE = { { { { 0x004C0000, 0x02404800, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE HDP_BASE = { { { { 0x00000F20, 0x0240A400, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE LSDMA_BASE = { { { { 0x00011400, 0x02456800, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE MMHUB_BASE = { { { { 0x0001A000, 0x02408800, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE MP0_BASE = { { { { 0x00016000, 0x00DC0000, 0x00E00000, 0x00E40000, 0x0243FC00, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE MP1_BASE = { { { { 0x00016000, 0x00DC0000, 0x00E00000, 0x00E40000, 0x0243FC00, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE NBIO_BASE = { { { { 0x00000000, 0x00000014, 0x00000D20, 0x00010400, 0x0241B000, 0x04040000 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE OSSSYS_BASE = { { { { 0x000010A0, 0x0240A000, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE PCIE0_BASE = { { { { 0x00000000, 0x00000014, 0x00000D20, 0x00010400, 0x0241B000, 0x04040000 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE SDMA0_BASE = { { { { 0x00001260, 0x0000A000, 0x0001C000, 0x02402C00, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE SDMA1_BASE = { { { { 0x00001260, 0x0000A000, 0x0001C000, 0x02402C00, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE SMUIO_BASE = { { { { 0x00016800, 0x00016A00, 0x02401000, 0x03440000, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE THM_BASE = { { { { 0x00016600, 0x02400C00, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE THM_MCD_BASE = { { { { 0x0C003400, 0x0C020200, 0, 0, 0, 0 } },
                                        { { 0x0C803400, 0x0C820200, 0, 0, 0, 0 } },
                                        { { 0x0D003400, 0x0D020200, 0, 0, 0, 0 } },
                                        { { 0x0D803400, 0x0D820200, 0, 0, 0, 0 } },
                                        { { 0x0E003400, 0x0E020200, 0, 0, 0, 0 } },
                                        { { 0x0E803400, 0x0E820200, 0, 0, 0, 0 } } } };
static const struct IP_BASE UMC_MCD_BASE = { { { { 0x0C004000, 0x0C400000, 0x0C004400, 0x0C404000, 0, 0 } },
                                        { { 0x0C804000, 0x0CC00000, 0x0C804400, 0x0CC04000, 0, 0 } },
                                        { { 0x0D004000, 0x0D400000, 0x0D004400, 0x0D404000, 0, 0 } },
                                        { { 0x0D804000, 0x0DC00000, 0x0D804400, 0x0DC04000, 0, 0 } },
                                        { { 0x0E004000, 0x0E400000, 0x0E004400, 0x0E404000, 0, 0 } },
                                        { { 0x0E804000, 0x0EC00000, 0x0E804400, 0x0EC04000, 0, 0 } } } };
static const struct IP_BASE USB0_BASE = { { { { 0x0242A800, 0x05B00000, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };
static const struct IP_BASE VCN_BASE = { { { { 0x00007800, 0x00007E00, 0x02403000, 0, 0, 0 } },
                                        { { 0x00007B00, 0x00012000, 0x02445000, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } },
                                        { { 0, 0, 0, 0, 0, 0 } } } };


#define ATHUB_BASE__INST0_SEG0                     0x00000C00
#define ATHUB_BASE__INST0_SEG1                     0x02408C00
#define ATHUB_BASE__INST0_SEG2                     0
#define ATHUB_BASE__INST0_SEG3                     0
#define ATHUB_BASE__INST0_SEG4                     0
#define ATHUB_BASE__INST0_SEG5                     0

#define ATHUB_BASE__INST1_SEG0                     0
#define ATHUB_BASE__INST1_SEG1                     0
#define ATHUB_BASE__INST1_SEG2                     0
#define ATHUB_BASE__INST1_SEG3                     0
#define ATHUB_BASE__INST1_SEG4                     0
#define ATHUB_BASE__INST1_SEG5                     0

#define ATHUB_BASE__INST2_SEG0                     0
#define ATHUB_BASE__INST2_SEG1                     0
#define ATHUB_BASE__INST2_SEG2                     0
#define ATHUB_BASE__INST2_SEG3                     0
#define ATHUB_BASE__INST2_SEG4                     0
#define ATHUB_BASE__INST2_SEG5                     0

#define ATHUB_BASE__INST3_SEG0                     0
#define ATHUB_BASE__INST3_SEG1                     0
#define ATHUB_BASE__INST3_SEG2                     0
#define ATHUB_BASE__INST3_SEG3                     0
#define ATHUB_BASE__INST3_SEG4                     0
#define ATHUB_BASE__INST3_SEG5                     0

#define ATHUB_BASE__INST4_SEG0                     0
#define ATHUB_BASE__INST4_SEG1                     0
#define ATHUB_BASE__INST4_SEG2                     0
#define ATHUB_BASE__INST4_SEG3                     0
#define ATHUB_BASE__INST4_SEG4                     0
#define ATHUB_BASE__INST4_SEG5                     0

#define ATHUB_BASE__INST5_SEG0                     0
#define ATHUB_BASE__INST5_SEG1                     0
#define ATHUB_BASE__INST5_SEG2                     0
#define ATHUB_BASE__INST5_SEG3                     0
#define ATHUB_BASE__INST5_SEG4                     0
#define ATHUB_BASE__INST5_SEG5                     0

#define CLK_BASE__INST0_SEG0                       0x00016C00
#define CLK_BASE__INST0_SEG1                       0x02401800
#define CLK_BASE__INST0_SEG2                       0
#define CLK_BASE__INST0_SEG3                       0
#define CLK_BASE__INST0_SEG4                       0
#define CLK_BASE__INST0_SEG5                       0

#define CLK_BASE__INST1_SEG0                       0
#define CLK_BASE__INST1_SEG1                       0
#define CLK_BASE__INST1_SEG2                       0
#define CLK_BASE__INST1_SEG3                       0
#define CLK_BASE__INST1_SEG4                       0
#define CLK_BASE__INST1_SEG5                       0

#define CLK_BASE__INST2_SEG0                       0
#define CLK_BASE__INST2_SEG1                       0
#define CLK_BASE__INST2_SEG2                       0
#define CLK_BASE__INST2_SEG3                       0
#define CLK_BASE__INST2_SEG4                       0
#define CLK_BASE__INST2_SEG5                       0

#define CLK_BASE__INST3_SEG0                       0
#define CLK_BASE__INST3_SEG1                       0
#define CLK_BASE__INST3_SEG2                       0
#define CLK_BASE__INST3_SEG3                       0
#define CLK_BASE__INST3_SEG4                       0
#define CLK_BASE__INST3_SEG5                       0

#define CLK_BASE__INST4_SEG0                       0
#define CLK_BASE__INST4_SEG1                       0
#define CLK_BASE__INST4_SEG2                       0
#define CLK_BASE__INST4_SEG3                       0
#define CLK_BASE__INST4_SEG4                       0
#define CLK_BASE__INST4_SEG5                       0

#define CLK_BASE__INST5_SEG0                       0
#define CLK_BASE__INST5_SEG1                       0
#define CLK_BASE__INST5_SEG2                       0
#define CLK_BASE__INST5_SEG3                       0
#define CLK_BASE__INST5_SEG4                       0
#define CLK_BASE__INST5_SEG5                       0

#define CLK_MCD_BASE__INST0_SEG0                   0x0C000400
#define CLK_MCD_BASE__INST0_SEG1                   0x0C021000
#define CLK_MCD_BASE__INST0_SEG2                   0
#define CLK_MCD_BASE__INST0_SEG3                   0
#define CLK_MCD_BASE__INST0_SEG4                   0
#define CLK_MCD_BASE__INST0_SEG5                   0

#define CLK_MCD_BASE__INST1_SEG0                   0x0C800400
#define CLK_MCD_BASE__INST1_SEG1                   0x0C821000
#define CLK_MCD_BASE__INST1_SEG2                   0
#define CLK_MCD_BASE__INST1_SEG3                   0
#define CLK_MCD_BASE__INST1_SEG4                   0
#define CLK_MCD_BASE__INST1_SEG5                   0

#define CLK_MCD_BASE__INST2_SEG0                   0x0D000400
#define CLK_MCD_BASE__INST2_SEG1                   0x0D021000
#define CLK_MCD_BASE__INST2_SEG2                   0
#define CLK_MCD_BASE__INST2_SEG3                   0
#define CLK_MCD_BASE__INST2_SEG4                   0
#define CLK_MCD_BASE__INST2_SEG5                   0

#define CLK_MCD_BASE__INST3_SEG0                   0x0D800400
#define CLK_MCD_BASE__INST3_SEG1                   0x0D821000
#define CLK_MCD_BASE__INST3_SEG2                   0
#define CLK_MCD_BASE__INST3_SEG3                   0
#define CLK_MCD_BASE__INST3_SEG4                   0
#define CLK_MCD_BASE__INST3_SEG5                   0

#define CLK_MCD_BASE__INST4_SEG0                   0x0E000400
#define CLK_MCD_BASE__INST4_SEG1                   0x0E021000
#define CLK_MCD_BASE__INST4_SEG2                   0
#define CLK_MCD_BASE__INST4_SEG3                   0
#define CLK_MCD_BASE__INST4_SEG4                   0
#define CLK_MCD_BASE__INST4_SEG5                   0

#define CLK_MCD_BASE__INST5_SEG0                   0x0E800400
#define CLK_MCD_BASE__INST5_SEG1                   0x0E821000
#define CLK_MCD_BASE__INST5_SEG2                   0
#define CLK_MCD_BASE__INST5_SEG3                   0
#define CLK_MCD_BASE__INST5_SEG4                   0
#define CLK_MCD_BASE__INST5_SEG5                   0

#define DF_BASE__INST0_SEG0                        0x00007000
#define DF_BASE__INST0_SEG1                        0x00C00000
#define DF_BASE__INST0_SEG2                        0x0240B800
#define DF_BASE__INST0_SEG3                        0x07C00000
#define DF_BASE__INST0_SEG4                        0x12400000
#define DF_BASE__INST0_SEG5                        0

#define DF_BASE__INST1_SEG0                        0
#define DF_BASE__INST1_SEG1                        0
#define DF_BASE__INST1_SEG2                        0
#define DF_BASE__INST1_SEG3                        0
#define DF_BASE__INST1_SEG4                        0
#define DF_BASE__INST1_SEG5                        0

#define DF_BASE__INST2_SEG0                        0
#define DF_BASE__INST2_SEG1                        0
#define DF_BASE__INST2_SEG2                        0
#define DF_BASE__INST2_SEG3                        0
#define DF_BASE__INST2_SEG4                        0
#define DF_BASE__INST2_SEG5                        0

#define DF_BASE__INST3_SEG0                        0
#define DF_BASE__INST3_SEG1                        0
#define DF_BASE__INST3_SEG2                        0
#define DF_BASE__INST3_SEG3                        0
#define DF_BASE__INST3_SEG4                        0
#define DF_BASE__INST3_SEG5                        0

#define DF_BASE__INST4_SEG0                        0
#define DF_BASE__INST4_SEG1                        0
#define DF_BASE__INST4_SEG2                        0
#define DF_BASE__INST4_SEG3                        0
#define DF_BASE__INST4_SEG4                        0
#define DF_BASE__INST4_SEG5                        0

#define DF_BASE__INST5_SEG0                        0
#define DF_BASE__INST5_SEG1                        0
#define DF_BASE__INST5_SEG2                        0
#define DF_BASE__INST5_SEG3                        0
#define DF_BASE__INST5_SEG4                        0
#define DF_BASE__INST5_SEG5                        0

#define DF_MCD_BASE__INST0_SEG0                    0x0C000000
#define DF_MCD_BASE__INST0_SEG1                    0x0C021800
#define DF_MCD_BASE__INST0_SEG2                    0x0C022000
#define DF_MCD_BASE__INST0_SEG3                    0x0C2C0000
#define DF_MCD_BASE__INST0_SEG4                    0x0C300000
#define DF_MCD_BASE__INST0_SEG5                    0

#define DF_MCD_BASE__INST1_SEG0                    0x0C800000
#define DF_MCD_BASE__INST1_SEG1                    0x0C821800
#define DF_MCD_BASE__INST1_SEG2                    0x0C822000
#define DF_MCD_BASE__INST1_SEG3                    0x0CAC0000
#define DF_MCD_BASE__INST1_SEG4                    0x0CB00000
#define DF_MCD_BASE__INST1_SEG5                    0

#define DF_MCD_BASE__INST2_SEG0                    0x0D000000
#define DF_MCD_BASE__INST2_SEG1                    0x0D021800
#define DF_MCD_BASE__INST2_SEG2                    0x0D022000
#define DF_MCD_BASE__INST2_SEG3                    0x0D2C0000
#define DF_MCD_BASE__INST2_SEG4                    0x0D300000
#define DF_MCD_BASE__INST2_SEG5                    0

#define DF_MCD_BASE__INST3_SEG0                    0x0D800000
#define DF_MCD_BASE__INST3_SEG1                    0x0D821800
#define DF_MCD_BASE__INST3_SEG2                    0x0D822000
#define DF_MCD_BASE__INST3_SEG3                    0x0DAC0000
#define DF_MCD_BASE__INST3_SEG4                    0x0DB00000
#define DF_MCD_BASE__INST3_SEG5                    0

#define DF_MCD_BASE__INST4_SEG0                    0x0E000000
#define DF_MCD_BASE__INST4_SEG1                    0x0E021800
#define DF_MCD_BASE__INST4_SEG2                    0x0E022000
#define DF_MCD_BASE__INST4_SEG3                    0x0E2C0000
#define DF_MCD_BASE__INST4_SEG4                    0x0E300000
#define DF_MCD_BASE__INST4_SEG5                    0

#define DF_MCD_BASE__INST5_SEG0                    0x0E800000
#define DF_MCD_BASE__INST5_SEG1                    0x0E821800
#define DF_MCD_BASE__INST5_SEG2                    0x0E822000
#define DF_MCD_BASE__INST5_SEG3                    0x0EAC0000
#define DF_MCD_BASE__INST5_SEG4                    0x0EB00000
#define DF_MCD_BASE__INST5_SEG5                    0

#define DIO_BASE__INST0_SEG0                       0x02404000
#define DIO_BASE__INST0_SEG1                       0
#define DIO_BASE__INST0_SEG2                       0
#define DIO_BASE__INST0_SEG3                       0
#define DIO_BASE__INST0_SEG4                       0
#define DIO_BASE__INST0_SEG5                       0

#define DIO_BASE__INST1_SEG0                       0
#define DIO_BASE__INST1_SEG1                       0
#define DIO_BASE__INST1_SEG2                       0
#define DIO_BASE__INST1_SEG3                       0
#define DIO_BASE__INST1_SEG4                       0
#define DIO_BASE__INST1_SEG5                       0

#define DIO_BASE__INST2_SEG0                       0
#define DIO_BASE__INST2_SEG1                       0
#define DIO_BASE__INST2_SEG2                       0
#define DIO_BASE__INST2_SEG3                       0
#define DIO_BASE__INST2_SEG4                       0
#define DIO_BASE__INST2_SEG5                       0

#define DIO_BASE__INST3_SEG0                       0
#define DIO_BASE__INST3_SEG1                       0
#define DIO_BASE__INST3_SEG2                       0
#define DIO_BASE__INST3_SEG3                       0
#define DIO_BASE__INST3_SEG4                       0
#define DIO_BASE__INST3_SEG5                       0

#define DIO_BASE__INST4_SEG0                       0
#define DIO_BASE__INST4_SEG1                       0
#define DIO_BASE__INST4_SEG2                       0
#define DIO_BASE__INST4_SEG3                       0
#define DIO_BASE__INST4_SEG4                       0
#define DIO_BASE__INST4_SEG5                       0

#define DIO_BASE__INST5_SEG0                       0
#define DIO_BASE__INST5_SEG1                       0
#define DIO_BASE__INST5_SEG2                       0
#define DIO_BASE__INST5_SEG3                       0
#define DIO_BASE__INST5_SEG4                       0
#define DIO_BASE__INST5_SEG5                       0

#define DCN_BASE__INST0_SEG0                       0x00000012
#define DCN_BASE__INST0_SEG1                       0x000000C0
#define DCN_BASE__INST0_SEG2                       0x000034C0
#define DCN_BASE__INST0_SEG3                       0x00009000
#define DCN_BASE__INST0_SEG4                       0x02403C00
#define DCN_BASE__INST0_SEG5                       0

#define DCN_BASE__INST1_SEG0                       0
#define DCN_BASE__INST1_SEG1                       0
#define DCN_BASE__INST1_SEG2                       0
#define DCN_BASE__INST1_SEG3                       0
#define DCN_BASE__INST1_SEG4                       0
#define DCN_BASE__INST1_SEG5                       0

#define DCN_BASE__INST2_SEG0                       0
#define DCN_BASE__INST2_SEG1                       0
#define DCN_BASE__INST2_SEG2                       0
#define DCN_BASE__INST2_SEG3                       0
#define DCN_BASE__INST2_SEG4                       0
#define DCN_BASE__INST2_SEG5                       0

#define DCN_BASE__INST3_SEG0                       0
#define DCN_BASE__INST3_SEG1                       0
#define DCN_BASE__INST3_SEG2                       0
#define DCN_BASE__INST3_SEG3                       0
#define DCN_BASE__INST3_SEG4                       0
#define DCN_BASE__INST3_SEG5                       0

#define DCN_BASE__INST4_SEG0                       0
#define DCN_BASE__INST4_SEG1                       0
#define DCN_BASE__INST4_SEG2                       0
#define DCN_BASE__INST4_SEG3                       0
#define DCN_BASE__INST4_SEG4                       0
#define DCN_BASE__INST4_SEG5                       0

#define DCN_BASE__INST5_SEG0                       0
#define DCN_BASE__INST5_SEG1                       0
#define DCN_BASE__INST5_SEG2                       0
#define DCN_BASE__INST5_SEG3                       0
#define DCN_BASE__INST5_SEG4                       0
#define DCN_BASE__INST5_SEG5                       0

#define DPCS_BASE__INST0_SEG0                      0x00000012
#define DPCS_BASE__INST0_SEG1                      0x000000C0
#define DPCS_BASE__INST0_SEG2                      0x000034C0
#define DPCS_BASE__INST0_SEG3                      0x00009000
#define DPCS_BASE__INST0_SEG4                      0x02403C00
#define DPCS_BASE__INST0_SEG5                      0

#define DPCS_BASE__INST1_SEG0                      0
#define DPCS_BASE__INST1_SEG1                      0
#define DPCS_BASE__INST1_SEG2                      0
#define DPCS_BASE__INST1_SEG3                      0
#define DPCS_BASE__INST1_SEG4                      0
#define DPCS_BASE__INST1_SEG5                      0

#define DPCS_BASE__INST2_SEG0                      0
#define DPCS_BASE__INST2_SEG1                      0
#define DPCS_BASE__INST2_SEG2                      0
#define DPCS_BASE__INST2_SEG3                      0
#define DPCS_BASE__INST2_SEG4                      0
#define DPCS_BASE__INST2_SEG5                      0

#define DPCS_BASE__INST3_SEG0                      0
#define DPCS_BASE__INST3_SEG1                      0
#define DPCS_BASE__INST3_SEG2                      0
#define DPCS_BASE__INST3_SEG3                      0
#define DPCS_BASE__INST3_SEG4                      0
#define DPCS_BASE__INST3_SEG5                      0

#define DPCS_BASE__INST4_SEG0                      0
#define DPCS_BASE__INST4_SEG1                      0
#define DPCS_BASE__INST4_SEG2                      0
#define DPCS_BASE__INST4_SEG3                      0
#define DPCS_BASE__INST4_SEG4                      0
#define DPCS_BASE__INST4_SEG5                      0

#define DPCS_BASE__INST5_SEG0                      0
#define DPCS_BASE__INST5_SEG1                      0
#define DPCS_BASE__INST5_SEG2                      0
#define DPCS_BASE__INST5_SEG3                      0
#define DPCS_BASE__INST5_SEG4                      0
#define DPCS_BASE__INST5_SEG5                      0

#define FUSE_BASE__INST0_SEG0                      0x00017400
#define FUSE_BASE__INST0_SEG1                      0x02401400
#define FUSE_BASE__INST0_SEG2                      0
#define FUSE_BASE__INST0_SEG3                      0
#define FUSE_BASE__INST0_SEG4                      0
#define FUSE_BASE__INST0_SEG5                      0

#define FUSE_BASE__INST1_SEG0                      0
#define FUSE_BASE__INST1_SEG1                      0
#define FUSE_BASE__INST1_SEG2                      0
#define FUSE_BASE__INST1_SEG3                      0
#define FUSE_BASE__INST1_SEG4                      0
#define FUSE_BASE__INST1_SEG5                      0

#define FUSE_BASE__INST2_SEG0                      0
#define FUSE_BASE__INST2_SEG1                      0
#define FUSE_BASE__INST2_SEG2                      0
#define FUSE_BASE__INST2_SEG3                      0
#define FUSE_BASE__INST2_SEG4                      0
#define FUSE_BASE__INST2_SEG5                      0

#define FUSE_BASE__INST3_SEG0                      0
#define FUSE_BASE__INST3_SEG1                      0
#define FUSE_BASE__INST3_SEG2                      0
#define FUSE_BASE__INST3_SEG3                      0
#define FUSE_BASE__INST3_SEG4                      0
#define FUSE_BASE__INST3_SEG5                      0

#define FUSE_BASE__INST4_SEG0                      0
#define FUSE_BASE__INST4_SEG1                      0
#define FUSE_BASE__INST4_SEG2                      0
#define FUSE_BASE__INST4_SEG3                      0
#define FUSE_BASE__INST4_SEG4                      0
#define FUSE_BASE__INST4_SEG5                      0

#define FUSE_BASE__INST5_SEG0                      0
#define FUSE_BASE__INST5_SEG1                      0
#define FUSE_BASE__INST5_SEG2                      0
#define FUSE_BASE__INST5_SEG3                      0
#define FUSE_BASE__INST5_SEG4                      0
#define FUSE_BASE__INST5_SEG5                      0

#define FUSE_MCD_BASE__INST0_SEG0                  0x0C002C00
#define FUSE_MCD_BASE__INST0_SEG1                  0x0C020600
#define FUSE_MCD_BASE__INST0_SEG2                  0
#define FUSE_MCD_BASE__INST0_SEG3                  0
#define FUSE_MCD_BASE__INST0_SEG4                  0
#define FUSE_MCD_BASE__INST0_SEG5                  0

#define FUSE_MCD_BASE__INST1_SEG0                  0x0C802C00
#define FUSE_MCD_BASE__INST1_SEG1                  0x0C820600
#define FUSE_MCD_BASE__INST1_SEG2                  0
#define FUSE_MCD_BASE__INST1_SEG3                  0
#define FUSE_MCD_BASE__INST1_SEG4                  0
#define FUSE_MCD_BASE__INST1_SEG5                  0

#define FUSE_MCD_BASE__INST2_SEG0                  0x0D002C00
#define FUSE_MCD_BASE__INST2_SEG1                  0x0D020600
#define FUSE_MCD_BASE__INST2_SEG2                  0
#define FUSE_MCD_BASE__INST2_SEG3                  0
#define FUSE_MCD_BASE__INST2_SEG4                  0
#define FUSE_MCD_BASE__INST2_SEG5                  0

#define FUSE_MCD_BASE__INST3_SEG0                  0x0D802C00
#define FUSE_MCD_BASE__INST3_SEG1                  0x0D820600
#define FUSE_MCD_BASE__INST3_SEG2                  0
#define FUSE_MCD_BASE__INST3_SEG3                  0
#define FUSE_MCD_BASE__INST3_SEG4                  0
#define FUSE_MCD_BASE__INST3_SEG5                  0

#define FUSE_MCD_BASE__INST4_SEG0                  0x0E002C00
#define FUSE_MCD_BASE__INST4_SEG1                  0x0E020600
#define FUSE_MCD_BASE__INST4_SEG2                  0
#define FUSE_MCD_BASE__INST4_SEG3                  0
#define FUSE_MCD_BASE__INST4_SEG4                  0
#define FUSE_MCD_BASE__INST4_SEG5                  0

#define FUSE_MCD_BASE__INST5_SEG0                  0x0E802C00
#define FUSE_MCD_BASE__INST5_SEG1                  0x0E820600
#define FUSE_MCD_BASE__INST5_SEG2                  0
#define FUSE_MCD_BASE__INST5_SEG3                  0
#define FUSE_MCD_BASE__INST5_SEG4                  0
#define FUSE_MCD_BASE__INST5_SEG5                  0

#define GC_BASE__INST0_SEG0                        0x00001260
#define GC_BASE__INST0_SEG1                        0x0000A000
#define GC_BASE__INST0_SEG2                        0x0001C000
#define GC_BASE__INST0_SEG3                        0x02402C00
#define GC_BASE__INST0_SEG4                        0
#define GC_BASE__INST0_SEG5                        0

#define GC_BASE__INST1_SEG0                        0
#define GC_BASE__INST1_SEG1                        0
#define GC_BASE__INST1_SEG2                        0
#define GC_BASE__INST1_SEG3                        0
#define GC_BASE__INST1_SEG4                        0
#define GC_BASE__INST1_SEG5                        0

#define GC_BASE__INST2_SEG0                        0
#define GC_BASE__INST2_SEG1                        0
#define GC_BASE__INST2_SEG2                        0
#define GC_BASE__INST2_SEG3                        0
#define GC_BASE__INST2_SEG4                        0
#define GC_BASE__INST2_SEG5                        0

#define GC_BASE__INST3_SEG0                        0
#define GC_BASE__INST3_SEG1                        0
#define GC_BASE__INST3_SEG2                        0
#define GC_BASE__INST3_SEG3                        0
#define GC_BASE__INST3_SEG4                        0
#define GC_BASE__INST3_SEG5                        0

#define GC_BASE__INST4_SEG0                        0
#define GC_BASE__INST4_SEG1                        0
#define GC_BASE__INST4_SEG2                        0
#define GC_BASE__INST4_SEG3                        0
#define GC_BASE__INST4_SEG4                        0
#define GC_BASE__INST4_SEG5                        0

#define GC_BASE__INST5_SEG0                        0
#define GC_BASE__INST5_SEG1                        0
#define GC_BASE__INST5_SEG2                        0
#define GC_BASE__INST5_SEG3                        0
#define GC_BASE__INST5_SEG4                        0
#define GC_BASE__INST5_SEG5                        0

#define HDA_BASE__INST0_SEG0                       0x004C0000
#define HDA_BASE__INST0_SEG1                       0x02404800
#define HDA_BASE__INST0_SEG2                       0
#define HDA_BASE__INST0_SEG3                       0
#define HDA_BASE__INST0_SEG4                       0
#define HDA_BASE__INST0_SEG5                       0

#define HDA_BASE__INST1_SEG0                       0
#define HDA_BASE__INST1_SEG1                       0
#define HDA_BASE__INST1_SEG2                       0
#define HDA_BASE__INST1_SEG3                       0
#define HDA_BASE__INST1_SEG4                       0
#define HDA_BASE__INST1_SEG5                       0

#define HDA_BASE__INST2_SEG0                       0
#define HDA_BASE__INST2_SEG1                       0
#define HDA_BASE__INST2_SEG2                       0
#define HDA_BASE__INST2_SEG3                       0
#define HDA_BASE__INST2_SEG4                       0
#define HDA_BASE__INST2_SEG5                       0

#define HDA_BASE__INST3_SEG0                       0
#define HDA_BASE__INST3_SEG1                       0
#define HDA_BASE__INST3_SEG2                       0
#define HDA_BASE__INST3_SEG3                       0
#define HDA_BASE__INST3_SEG4                       0
#define HDA_BASE__INST3_SEG5                       0

#define HDA_BASE__INST4_SEG0                       0
#define HDA_BASE__INST4_SEG1                       0
#define HDA_BASE__INST4_SEG2                       0
#define HDA_BASE__INST4_SEG3                       0
#define HDA_BASE__INST4_SEG4                       0
#define HDA_BASE__INST4_SEG5                       0

#define HDA_BASE__INST5_SEG0                       0
#define HDA_BASE__INST5_SEG1                       0
#define HDA_BASE__INST5_SEG2                       0
#define HDA_BASE__INST5_SEG3                       0
#define HDA_BASE__INST5_SEG4                       0
#define HDA_BASE__INST5_SEG5                       0

#define HDP_BASE__INST0_SEG0                       0x00000F20
#define HDP_BASE__INST0_SEG1                       0x0240A400
#define HDP_BASE__INST0_SEG2                       0
#define HDP_BASE__INST0_SEG3                       0
#define HDP_BASE__INST0_SEG4                       0
#define HDP_BASE__INST0_SEG5                       0

#define HDP_BASE__INST1_SEG0                       0
#define HDP_BASE__INST1_SEG1                       0
#define HDP_BASE__INST1_SEG2                       0
#define HDP_BASE__INST1_SEG3                       0
#define HDP_BASE__INST1_SEG4                       0
#define HDP_BASE__INST1_SEG5                       0

#define HDP_BASE__INST2_SEG0                       0
#define HDP_BASE__INST2_SEG1                       0
#define HDP_BASE__INST2_SEG2                       0
#define HDP_BASE__INST2_SEG3                       0
#define HDP_BASE__INST2_SEG4                       0
#define HDP_BASE__INST2_SEG5                       0

#define HDP_BASE__INST3_SEG0                       0
#define HDP_BASE__INST3_SEG1                       0
#define HDP_BASE__INST3_SEG2                       0
#define HDP_BASE__INST3_SEG3                       0
#define HDP_BASE__INST3_SEG4                       0
#define HDP_BASE__INST3_SEG5                       0

#define HDP_BASE__INST4_SEG0                       0
#define HDP_BASE__INST4_SEG1                       0
#define HDP_BASE__INST4_SEG2                       0
#define HDP_BASE__INST4_SEG3                       0
#define HDP_BASE__INST4_SEG4                       0
#define HDP_BASE__INST4_SEG5                       0

#define HDP_BASE__INST5_SEG0                       0
#define HDP_BASE__INST5_SEG1                       0
#define HDP_BASE__INST5_SEG2                       0
#define HDP_BASE__INST5_SEG3                       0
#define HDP_BASE__INST5_SEG4                       0
#define HDP_BASE__INST5_SEG5                       0

#define LSDMA_BASE__INST0_SEG0                     0x00011400
#define LSDMA_BASE__INST0_SEG1                     0x02456800
#define LSDMA_BASE__INST0_SEG2                     0
#define LSDMA_BASE__INST0_SEG3                     0
#define LSDMA_BASE__INST0_SEG4                     0
#define LSDMA_BASE__INST0_SEG5                     0

#define LSDMA_BASE__INST1_SEG0                     0
#define LSDMA_BASE__INST1_SEG1                     0
#define LSDMA_BASE__INST1_SEG2                     0
#define LSDMA_BASE__INST1_SEG3                     0
#define LSDMA_BASE__INST1_SEG4                     0
#define LSDMA_BASE__INST1_SEG5                     0

#define LSDMA_BASE__INST2_SEG0                     0
#define LSDMA_BASE__INST2_SEG1                     0
#define LSDMA_BASE__INST2_SEG2                     0
#define LSDMA_BASE__INST2_SEG3                     0
#define LSDMA_BASE__INST2_SEG4                     0
#define LSDMA_BASE__INST2_SEG5                     0

#define LSDMA_BASE__INST3_SEG0                     0
#define LSDMA_BASE__INST3_SEG1                     0
#define LSDMA_BASE__INST3_SEG2                     0
#define LSDMA_BASE__INST3_SEG3                     0
#define LSDMA_BASE__INST3_SEG4                     0
#define LSDMA_BASE__INST3_SEG5                     0

#define LSDMA_BASE__INST4_SEG0                     0
#define LSDMA_BASE__INST4_SEG1                     0
#define LSDMA_BASE__INST4_SEG2                     0
#define LSDMA_BASE__INST4_SEG3                     0
#define LSDMA_BASE__INST4_SEG4                     0
#define LSDMA_BASE__INST4_SEG5                     0

#define LSDMA_BASE__INST5_SEG0                     0
#define LSDMA_BASE__INST5_SEG1                     0
#define LSDMA_BASE__INST5_SEG2                     0
#define LSDMA_BASE__INST5_SEG3                     0
#define LSDMA_BASE__INST5_SEG4                     0
#define LSDMA_BASE__INST5_SEG5                     0

#define MMHUB_BASE__INST0_SEG0                     0x0001A000
#define MMHUB_BASE__INST0_SEG1                     0x02408800
#define MMHUB_BASE__INST0_SEG2                     0
#define MMHUB_BASE__INST0_SEG3                     0
#define MMHUB_BASE__INST0_SEG4                     0
#define MMHUB_BASE__INST0_SEG5                     0

#define MMHUB_BASE__INST1_SEG0                     0
#define MMHUB_BASE__INST1_SEG1                     0
#define MMHUB_BASE__INST1_SEG2                     0
#define MMHUB_BASE__INST1_SEG3                     0
#define MMHUB_BASE__INST1_SEG4                     0
#define MMHUB_BASE__INST1_SEG5                     0

#define MMHUB_BASE__INST2_SEG0                     0
#define MMHUB_BASE__INST2_SEG1                     0
#define MMHUB_BASE__INST2_SEG2                     0
#define MMHUB_BASE__INST2_SEG3                     0
#define MMHUB_BASE__INST2_SEG4                     0
#define MMHUB_BASE__INST2_SEG5                     0

#define MMHUB_BASE__INST3_SEG0                     0
#define MMHUB_BASE__INST3_SEG1                     0
#define MMHUB_BASE__INST3_SEG2                     0
#define MMHUB_BASE__INST3_SEG3                     0
#define MMHUB_BASE__INST3_SEG4                     0
#define MMHUB_BASE__INST3_SEG5                     0

#define MMHUB_BASE__INST4_SEG0                     0
#define MMHUB_BASE__INST4_SEG1                     0
#define MMHUB_BASE__INST4_SEG2                     0
#define MMHUB_BASE__INST4_SEG3                     0
#define MMHUB_BASE__INST4_SEG4                     0
#define MMHUB_BASE__INST4_SEG5                     0

#define MMHUB_BASE__INST5_SEG0                     0
#define MMHUB_BASE__INST5_SEG1                     0
#define MMHUB_BASE__INST5_SEG2                     0
#define MMHUB_BASE__INST5_SEG3                     0
#define MMHUB_BASE__INST5_SEG4                     0
#define MMHUB_BASE__INST5_SEG5                     0

#define MP0_BASE__INST0_SEG0                       0x00016000
#define MP0_BASE__INST0_SEG1                       0x00DC0000
#define MP0_BASE__INST0_SEG2                       0x00E00000
#define MP0_BASE__INST0_SEG3                       0x00E40000
#define MP0_BASE__INST0_SEG4                       0x0243FC00
#define MP0_BASE__INST0_SEG5                       0

#define MP0_BASE__INST1_SEG0                       0
#define MP0_BASE__INST1_SEG1                       0
#define MP0_BASE__INST1_SEG2                       0
#define MP0_BASE__INST1_SEG3                       0
#define MP0_BASE__INST1_SEG4                       0
#define MP0_BASE__INST1_SEG5                       0

#define MP0_BASE__INST2_SEG0                       0
#define MP0_BASE__INST2_SEG1                       0
#define MP0_BASE__INST2_SEG2                       0
#define MP0_BASE__INST2_SEG3                       0
#define MP0_BASE__INST2_SEG4                       0
#define MP0_BASE__INST2_SEG5                       0

#define MP0_BASE__INST3_SEG0                       0
#define MP0_BASE__INST3_SEG1                       0
#define MP0_BASE__INST3_SEG2                       0
#define MP0_BASE__INST3_SEG3                       0
#define MP0_BASE__INST3_SEG4                       0
#define MP0_BASE__INST3_SEG5                       0

#define MP0_BASE__INST4_SEG0                       0
#define MP0_BASE__INST4_SEG1                       0
#define MP0_BASE__INST4_SEG2                       0
#define MP0_BASE__INST4_SEG3                       0
#define MP0_BASE__INST4_SEG4                       0
#define MP0_BASE__INST4_SEG5                       0

#define MP0_BASE__INST5_SEG0                       0
#define MP0_BASE__INST5_SEG1                       0
#define MP0_BASE__INST5_SEG2                       0
#define MP0_BASE__INST5_SEG3                       0
#define MP0_BASE__INST5_SEG4                       0
#define MP0_BASE__INST5_SEG5                       0

#define MP1_BASE__INST0_SEG0                       0x00016000
#define MP1_BASE__INST0_SEG1                       0x00DC0000
#define MP1_BASE__INST0_SEG2                       0x00E00000
#define MP1_BASE__INST0_SEG3                       0x00E40000
#define MP1_BASE__INST0_SEG4                       0x0243FC00
#define MP1_BASE__INST0_SEG5                       0

#define MP1_BASE__INST1_SEG0                       0
#define MP1_BASE__INST1_SEG1                       0
#define MP1_BASE__INST1_SEG2                       0
#define MP1_BASE__INST1_SEG3                       0
#define MP1_BASE__INST1_SEG4                       0
#define MP1_BASE__INST1_SEG5                       0

#define MP1_BASE__INST2_SEG0                       0
#define MP1_BASE__INST2_SEG1                       0
#define MP1_BASE__INST2_SEG2                       0
#define MP1_BASE__INST2_SEG3                       0
#define MP1_BASE__INST2_SEG4                       0
#define MP1_BASE__INST2_SEG5                       0

#define MP1_BASE__INST3_SEG0                       0
#define MP1_BASE__INST3_SEG1                       0
#define MP1_BASE__INST3_SEG2                       0
#define MP1_BASE__INST3_SEG3                       0
#define MP1_BASE__INST3_SEG4                       0
#define MP1_BASE__INST3_SEG5                       0

#define MP1_BASE__INST4_SEG0                       0
#define MP1_BASE__INST4_SEG1                       0
#define MP1_BASE__INST4_SEG2                       0
#define MP1_BASE__INST4_SEG3                       0
#define MP1_BASE__INST4_SEG4                       0
#define MP1_BASE__INST4_SEG5                       0

#define MP1_BASE__INST5_SEG0                       0
#define MP1_BASE__INST5_SEG1                       0
#define MP1_BASE__INST5_SEG2                       0
#define MP1_BASE__INST5_SEG3                       0
#define MP1_BASE__INST5_SEG4                       0
#define MP1_BASE__INST5_SEG5                       0

#define NBIO_BASE__INST0_SEG0                      0x00000000
#define NBIO_BASE__INST0_SEG1                      0x00000014
#define NBIO_BASE__INST0_SEG2                      0x00000D20
#define NBIO_BASE__INST0_SEG3                      0x00010400
#define NBIO_BASE__INST0_SEG4                      0x0241B000
#define NBIO_BASE__INST0_SEG5                      0x04040000

#define NBIO_BASE__INST1_SEG0                      0
#define NBIO_BASE__INST1_SEG1                      0
#define NBIO_BASE__INST1_SEG2                      0
#define NBIO_BASE__INST1_SEG3                      0
#define NBIO_BASE__INST1_SEG4                      0
#define NBIO_BASE__INST1_SEG5                      0

#define NBIO_BASE__INST2_SEG0                      0
#define NBIO_BASE__INST2_SEG1                      0
#define NBIO_BASE__INST2_SEG2                      0
#define NBIO_BASE__INST2_SEG3                      0
#define NBIO_BASE__INST2_SEG4                      0
#define NBIO_BASE__INST2_SEG5                      0

#define NBIO_BASE__INST3_SEG0                      0
#define NBIO_BASE__INST3_SEG1                      0
#define NBIO_BASE__INST3_SEG2                      0
#define NBIO_BASE__INST3_SEG3                      0
#define NBIO_BASE__INST3_SEG4                      0
#define NBIO_BASE__INST3_SEG5                      0

#define NBIO_BASE__INST4_SEG0                      0
#define NBIO_BASE__INST4_SEG1                      0
#define NBIO_BASE__INST4_SEG2                      0
#define NBIO_BASE__INST4_SEG3                      0
#define NBIO_BASE__INST4_SEG4                      0
#define NBIO_BASE__INST4_SEG5                      0

#define NBIO_BASE__INST5_SEG0                      0
#define NBIO_BASE__INST5_SEG1                      0
#define NBIO_BASE__INST5_SEG2                      0
#define NBIO_BASE__INST5_SEG3                      0
#define NBIO_BASE__INST5_SEG4                      0
#define NBIO_BASE__INST5_SEG5                      0

#define OSSSYS_BASE__INST0_SEG0                    0x000010A0
#define OSSSYS_BASE__INST0_SEG1                    0x0240A000
#define OSSSYS_BASE__INST0_SEG2                    0
#define OSSSYS_BASE__INST0_SEG3                    0
#define OSSSYS_BASE__INST0_SEG4                    0
#define OSSSYS_BASE__INST0_SEG5                    0

#define OSSSYS_BASE__INST1_SEG0                    0
#define OSSSYS_BASE__INST1_SEG1                    0
#define OSSSYS_BASE__INST1_SEG2                    0
#define OSSSYS_BASE__INST1_SEG3                    0
#define OSSSYS_BASE__INST1_SEG4                    0
#define OSSSYS_BASE__INST1_SEG5                    0

#define OSSSYS_BASE__INST2_SEG0                    0
#define OSSSYS_BASE__INST2_SEG1                    0
#define OSSSYS_BASE__INST2_SEG2                    0
#define OSSSYS_BASE__INST2_SEG3                    0
#define OSSSYS_BASE__INST2_SEG4                    0
#define OSSSYS_BASE__INST2_SEG5                    0

#define OSSSYS_BASE__INST3_SEG0                    0
#define OSSSYS_BASE__INST3_SEG1                    0
#define OSSSYS_BASE__INST3_SEG2                    0
#define OSSSYS_BASE__INST3_SEG3                    0
#define OSSSYS_BASE__INST3_SEG4                    0
#define OSSSYS_BASE__INST3_SEG5                    0

#define OSSSYS_BASE__INST4_SEG0                    0
#define OSSSYS_BASE__INST4_SEG1                    0
#define OSSSYS_BASE__INST4_SEG2                    0
#define OSSSYS_BASE__INST4_SEG3                    0
#define OSSSYS_BASE__INST4_SEG4                    0
#define OSSSYS_BASE__INST4_SEG5                    0

#define OSSSYS_BASE__INST5_SEG0                    0
#define OSSSYS_BASE__INST5_SEG1                    0
#define OSSSYS_BASE__INST5_SEG2                    0
#define OSSSYS_BASE__INST5_SEG3                    0
#define OSSSYS_BASE__INST5_SEG4                    0
#define OSSSYS_BASE__INST5_SEG5                    0

#define PCIE0_BASE__INST0_SEG0                     0x00000000
#define PCIE0_BASE__INST0_SEG1                     0x00000014
#define PCIE0_BASE__INST0_SEG2                     0x00000D20
#define PCIE0_BASE__INST0_SEG3                     0x00010400
#define PCIE0_BASE__INST0_SEG4                     0x0241B000
#define PCIE0_BASE__INST0_SEG5                     0x04040000

#define PCIE0_BASE__INST1_SEG0                     0
#define PCIE0_BASE__INST1_SEG1                     0
#define PCIE0_BASE__INST1_SEG2                     0
#define PCIE0_BASE__INST1_SEG3                     0
#define PCIE0_BASE__INST1_SEG4                     0
#define PCIE0_BASE__INST1_SEG5                     0

#define PCIE0_BASE__INST2_SEG0                     0
#define PCIE0_BASE__INST2_SEG1                     0
#define PCIE0_BASE__INST2_SEG2                     0
#define PCIE0_BASE__INST2_SEG3                     0
#define PCIE0_BASE__INST2_SEG4                     0
#define PCIE0_BASE__INST2_SEG5                     0

#define PCIE0_BASE__INST3_SEG0                     0
#define PCIE0_BASE__INST3_SEG1                     0
#define PCIE0_BASE__INST3_SEG2                     0
#define PCIE0_BASE__INST3_SEG3                     0
#define PCIE0_BASE__INST3_SEG4                     0
#define PCIE0_BASE__INST3_SEG5                     0

#define PCIE0_BASE__INST4_SEG0                     0
#define PCIE0_BASE__INST4_SEG1                     0
#define PCIE0_BASE__INST4_SEG2                     0
#define PCIE0_BASE__INST4_SEG3                     0
#define PCIE0_BASE__INST4_SEG4                     0
#define PCIE0_BASE__INST4_SEG5                     0

#define PCIE0_BASE__INST5_SEG0                     0
#define PCIE0_BASE__INST5_SEG1                     0
#define PCIE0_BASE__INST5_SEG2                     0
#define PCIE0_BASE__INST5_SEG3                     0
#define PCIE0_BASE__INST5_SEG4                     0
#define PCIE0_BASE__INST5_SEG5                     0

#define SDMA0_BASE__INST0_SEG0                     0x00001260
#define SDMA0_BASE__INST0_SEG1                     0x0000A000
#define SDMA0_BASE__INST0_SEG2                     0x0001C000
#define SDMA0_BASE__INST0_SEG3                     0x02402C00
#define SDMA0_BASE__INST0_SEG4                     0
#define SDMA0_BASE__INST0_SEG5                     0

#define SDMA0_BASE__INST1_SEG0                     0
#define SDMA0_BASE__INST1_SEG1                     0
#define SDMA0_BASE__INST1_SEG2                     0
#define SDMA0_BASE__INST1_SEG3                     0
#define SDMA0_BASE__INST1_SEG4                     0
#define SDMA0_BASE__INST1_SEG5                     0

#define SDMA0_BASE__INST2_SEG0                     0
#define SDMA0_BASE__INST2_SEG1                     0
#define SDMA0_BASE__INST2_SEG2                     0
#define SDMA0_BASE__INST2_SEG3                     0
#define SDMA0_BASE__INST2_SEG4                     0
#define SDMA0_BASE__INST2_SEG5                     0

#define SDMA0_BASE__INST3_SEG0                     0
#define SDMA0_BASE__INST3_SEG1                     0
#define SDMA0_BASE__INST3_SEG2                     0
#define SDMA0_BASE__INST3_SEG3                     0
#define SDMA0_BASE__INST3_SEG4                     0
#define SDMA0_BASE__INST3_SEG5                     0

#define SDMA0_BASE__INST4_SEG0                     0
#define SDMA0_BASE__INST4_SEG1                     0
#define SDMA0_BASE__INST4_SEG2                     0
#define SDMA0_BASE__INST4_SEG3                     0
#define SDMA0_BASE__INST4_SEG4                     0
#define SDMA0_BASE__INST4_SEG5                     0

#define SDMA0_BASE__INST5_SEG0                     0
#define SDMA0_BASE__INST5_SEG1                     0
#define SDMA0_BASE__INST5_SEG2                     0
#define SDMA0_BASE__INST5_SEG3                     0
#define SDMA0_BASE__INST5_SEG4                     0
#define SDMA0_BASE__INST5_SEG5                     0

#define SDMA1_BASE__INST0_SEG0                     0x00001260
#define SDMA1_BASE__INST0_SEG1                     0x0000A000
#define SDMA1_BASE__INST0_SEG2                     0x0001C000
#define SDMA1_BASE__INST0_SEG3                     0x02402C00
#define SDMA1_BASE__INST0_SEG4                     0
#define SDMA1_BASE__INST0_SEG5                     0

#define SDMA1_BASE__INST1_SEG0                     0
#define SDMA1_BASE__INST1_SEG1                     0
#define SDMA1_BASE__INST1_SEG2                     0
#define SDMA1_BASE__INST1_SEG3                     0
#define SDMA1_BASE__INST1_SEG4                     0
#define SDMA1_BASE__INST1_SEG5                     0

#define SDMA1_BASE__INST2_SEG0                     0
#define SDMA1_BASE__INST2_SEG1                     0
#define SDMA1_BASE__INST2_SEG2                     0
#define SDMA1_BASE__INST2_SEG3                     0
#define SDMA1_BASE__INST2_SEG4                     0
#define SDMA1_BASE__INST2_SEG5                     0

#define SDMA1_BASE__INST3_SEG0                     0
#define SDMA1_BASE__INST3_SEG1                     0
#define SDMA1_BASE__INST3_SEG2                     0
#define SDMA1_BASE__INST3_SEG3                     0
#define SDMA1_BASE__INST3_SEG4                     0
#define SDMA1_BASE__INST3_SEG5                     0

#define SDMA1_BASE__INST4_SEG0                     0
#define SDMA1_BASE__INST4_SEG1                     0
#define SDMA1_BASE__INST4_SEG2                     0
#define SDMA1_BASE__INST4_SEG3                     0
#define SDMA1_BASE__INST4_SEG4                     0
#define SDMA1_BASE__INST4_SEG5                     0

#define SDMA1_BASE__INST5_SEG0                     0
#define SDMA1_BASE__INST5_SEG1                     0
#define SDMA1_BASE__INST5_SEG2                     0
#define SDMA1_BASE__INST5_SEG3                     0
#define SDMA1_BASE__INST5_SEG4                     0
#define SDMA1_BASE__INST5_SEG5                     0

#define SMUIO_BASE__INST0_SEG0                     0x00016800
#define SMUIO_BASE__INST0_SEG1                     0x00016A00
#define SMUIO_BASE__INST0_SEG2                     0x02401000
#define SMUIO_BASE__INST0_SEG3                     0x03440000
#define SMUIO_BASE__INST0_SEG4                     0
#define SMUIO_BASE__INST0_SEG5                     0

#define SMUIO_BASE__INST1_SEG0                     0
#define SMUIO_BASE__INST1_SEG1                     0
#define SMUIO_BASE__INST1_SEG2                     0
#define SMUIO_BASE__INST1_SEG3                     0
#define SMUIO_BASE__INST1_SEG4                     0
#define SMUIO_BASE__INST1_SEG5                     0

#define SMUIO_BASE__INST2_SEG0                     0
#define SMUIO_BASE__INST2_SEG1                     0
#define SMUIO_BASE__INST2_SEG2                     0
#define SMUIO_BASE__INST2_SEG3                     0
#define SMUIO_BASE__INST2_SEG4                     0
#define SMUIO_BASE__INST2_SEG5                     0

#define SMUIO_BASE__INST3_SEG0                     0
#define SMUIO_BASE__INST3_SEG1                     0
#define SMUIO_BASE__INST3_SEG2                     0
#define SMUIO_BASE__INST3_SEG3                     0
#define SMUIO_BASE__INST3_SEG4                     0
#define SMUIO_BASE__INST3_SEG5                     0

#define SMUIO_BASE__INST4_SEG0                     0
#define SMUIO_BASE__INST4_SEG1                     0
#define SMUIO_BASE__INST4_SEG2                     0
#define SMUIO_BASE__INST4_SEG3                     0
#define SMUIO_BASE__INST4_SEG4                     0
#define SMUIO_BASE__INST4_SEG5                     0

#define SMUIO_BASE__INST5_SEG0                     0
#define SMUIO_BASE__INST5_SEG1                     0
#define SMUIO_BASE__INST5_SEG2                     0
#define SMUIO_BASE__INST5_SEG3                     0
#define SMUIO_BASE__INST5_SEG4                     0
#define SMUIO_BASE__INST5_SEG5                     0

#define THM_BASE__INST0_SEG0                       0x00016600
#define THM_BASE__INST0_SEG1                       0x02400C00
#define THM_BASE__INST0_SEG2                       0
#define THM_BASE__INST0_SEG3                       0
#define THM_BASE__INST0_SEG4                       0
#define THM_BASE__INST0_SEG5                       0

#define THM_BASE__INST1_SEG0                       0
#define THM_BASE__INST1_SEG1                       0
#define THM_BASE__INST1_SEG2                       0
#define THM_BASE__INST1_SEG3                       0
#define THM_BASE__INST1_SEG4                       0
#define THM_BASE__INST1_SEG5                       0

#define THM_BASE__INST2_SEG0                       0
#define THM_BASE__INST2_SEG1                       0
#define THM_BASE__INST2_SEG2                       0
#define THM_BASE__INST2_SEG3                       0
#define THM_BASE__INST2_SEG4                       0
#define THM_BASE__INST2_SEG5                       0

#define THM_BASE__INST3_SEG0                       0
#define THM_BASE__INST3_SEG1                       0
#define THM_BASE__INST3_SEG2                       0
#define THM_BASE__INST3_SEG3                       0
#define THM_BASE__INST3_SEG4                       0
#define THM_BASE__INST3_SEG5                       0

#define THM_BASE__INST4_SEG0                       0
#define THM_BASE__INST4_SEG1                       0
#define THM_BASE__INST4_SEG2                       0
#define THM_BASE__INST4_SEG3                       0
#define THM_BASE__INST4_SEG4                       0
#define THM_BASE__INST4_SEG5                       0

#define THM_BASE__INST5_SEG0                       0
#define THM_BASE__INST5_SEG1                       0
#define THM_BASE__INST5_SEG2                       0
#define THM_BASE__INST5_SEG3                       0
#define THM_BASE__INST5_SEG4                       0
#define THM_BASE__INST5_SEG5                       0

#define THM_MCD_BASE__INST0_SEG0                   0x0C003400
#define THM_MCD_BASE__INST0_SEG1                   0x0C020200
#define THM_MCD_BASE__INST0_SEG2                   0
#define THM_MCD_BASE__INST0_SEG3                   0
#define THM_MCD_BASE__INST0_SEG4                   0
#define THM_MCD_BASE__INST0_SEG5                   0

#define THM_MCD_BASE__INST1_SEG0                   0x0C803400
#define THM_MCD_BASE__INST1_SEG1                   0x0C820200
#define THM_MCD_BASE__INST1_SEG2                   0
#define THM_MCD_BASE__INST1_SEG3                   0
#define THM_MCD_BASE__INST1_SEG4                   0
#define THM_MCD_BASE__INST1_SEG5                   0

#define THM_MCD_BASE__INST2_SEG0                   0x0D003400
#define THM_MCD_BASE__INST2_SEG1                   0x0D020200
#define THM_MCD_BASE__INST2_SEG2                   0
#define THM_MCD_BASE__INST2_SEG3                   0
#define THM_MCD_BASE__INST2_SEG4                   0
#define THM_MCD_BASE__INST2_SEG5                   0

#define THM_MCD_BASE__INST3_SEG0                   0x0D803400
#define THM_MCD_BASE__INST3_SEG1                   0x0D820200
#define THM_MCD_BASE__INST3_SEG2                   0
#define THM_MCD_BASE__INST3_SEG3                   0
#define THM_MCD_BASE__INST3_SEG4                   0
#define THM_MCD_BASE__INST3_SEG5                   0

#define THM_MCD_BASE__INST4_SEG0                   0x0E003400
#define THM_MCD_BASE__INST4_SEG1                   0x0E020200
#define THM_MCD_BASE__INST4_SEG2                   0
#define THM_MCD_BASE__INST4_SEG3                   0
#define THM_MCD_BASE__INST4_SEG4                   0
#define THM_MCD_BASE__INST4_SEG5                   0

#define THM_MCD_BASE__INST5_SEG0                   0x0E803400
#define THM_MCD_BASE__INST5_SEG1                   0x0E820200
#define THM_MCD_BASE__INST5_SEG2                   0
#define THM_MCD_BASE__INST5_SEG3                   0
#define THM_MCD_BASE__INST5_SEG4                   0
#define THM_MCD_BASE__INST5_SEG5                   0

#define UMC_MCD_BASE__INST0_SEG0                   0x0C004000
#define UMC_MCD_BASE__INST0_SEG1                   0x0C400000
#define UMC_MCD_BASE__INST0_SEG2                   0x0C004400
#define UMC_MCD_BASE__INST0_SEG3                   0x0C404000
#define UMC_MCD_BASE__INST0_SEG4                   0
#define UMC_MCD_BASE__INST0_SEG5                   0

#define UMC_MCD_BASE__INST1_SEG0                   0x0C804000
#define UMC_MCD_BASE__INST1_SEG1                   0x0CC00000
#define UMC_MCD_BASE__INST1_SEG2                   0x0C804400
#define UMC_MCD_BASE__INST1_SEG3                   0x0CC04000
#define UMC_MCD_BASE__INST1_SEG4                   0
#define UMC_MCD_BASE__INST1_SEG5                   0

#define UMC_MCD_BASE__INST2_SEG0                   0x0D004000
#define UMC_MCD_BASE__INST2_SEG1                   0x0D400000
#define UMC_MCD_BASE__INST2_SEG2                   0x0D004400
#define UMC_MCD_BASE__INST2_SEG3                   0x0D404000
#define UMC_MCD_BASE__INST2_SEG4                   0
#define UMC_MCD_BASE__INST2_SEG5                   0

#define UMC_MCD_BASE__INST3_SEG0                   0x0D804000
#define UMC_MCD_BASE__INST3_SEG1                   0x0DC00000
#define UMC_MCD_BASE__INST3_SEG2                   0x0D804400
#define UMC_MCD_BASE__INST3_SEG3                   0x0DC04000
#define UMC_MCD_BASE__INST3_SEG4                   0
#define UMC_MCD_BASE__INST3_SEG5                   0

#define UMC_MCD_BASE__INST4_SEG0                   0x0E004000
#define UMC_MCD_BASE__INST4_SEG1                   0x0E400000
#define UMC_MCD_BASE__INST4_SEG2                   0x0E004400
#define UMC_MCD_BASE__INST4_SEG3                   0x0E404000
#define UMC_MCD_BASE__INST4_SEG4                   0
#define UMC_MCD_BASE__INST4_SEG5                   0

#define UMC_MCD_BASE__INST5_SEG0                   0x0E804000
#define UMC_MCD_BASE__INST5_SEG1                   0x0EC00000
#define UMC_MCD_BASE__INST5_SEG2                   0x0E804400
#define UMC_MCD_BASE__INST5_SEG3                   0x0EC04000
#define UMC_MCD_BASE__INST5_SEG4                   0
#define UMC_MCD_BASE__INST5_SEG5                   0

#define USB0_BASE__INST0_SEG0                      0x0242A800
#define USB0_BASE__INST0_SEG1                      0x05B00000
#define USB0_BASE__INST0_SEG2                      0
#define USB0_BASE__INST0_SEG3                      0
#define USB0_BASE__INST0_SEG4                      0
#define USB0_BASE__INST0_SEG5                      0

#define USB0_BASE__INST1_SEG0                      0
#define USB0_BASE__INST1_SEG1                      0
#define USB0_BASE__INST1_SEG2                      0
#define USB0_BASE__INST1_SEG3                      0
#define USB0_BASE__INST1_SEG4                      0
#define USB0_BASE__INST1_SEG5                      0

#define USB0_BASE__INST2_SEG0                      0
#define USB0_BASE__INST2_SEG1                      0
#define USB0_BASE__INST2_SEG2                      0
#define USB0_BASE__INST2_SEG3                      0
#define USB0_BASE__INST2_SEG4                      0
#define USB0_BASE__INST2_SEG5                      0

#define USB0_BASE__INST3_SEG0                      0
#define USB0_BASE__INST3_SEG1                      0
#define USB0_BASE__INST3_SEG2                      0
#define USB0_BASE__INST3_SEG3                      0
#define USB0_BASE__INST3_SEG4                      0
#define USB0_BASE__INST3_SEG5                      0

#define USB0_BASE__INST4_SEG0                      0
#define USB0_BASE__INST4_SEG1                      0
#define USB0_BASE__INST4_SEG2                      0
#define USB0_BASE__INST4_SEG3                      0
#define USB0_BASE__INST4_SEG4                      0
#define USB0_BASE__INST4_SEG5                      0

#define USB0_BASE__INST5_SEG0                      0
#define USB0_BASE__INST5_SEG1                      0
#define USB0_BASE__INST5_SEG2                      0
#define USB0_BASE__INST5_SEG3                      0
#define USB0_BASE__INST5_SEG4                      0
#define USB0_BASE__INST5_SEG5                      0

#define VCN_BASE__INST0_SEG0                       0x00007800
#define VCN_BASE__INST0_SEG1                       0x00007E00
#define VCN_BASE__INST0_SEG2                       0x02403000
#define VCN_BASE__INST0_SEG3                       0
#define VCN_BASE__INST0_SEG4                       0
#define VCN_BASE__INST0_SEG5                       0

#define VCN_BASE__INST1_SEG0                       0x00007B00
#define VCN_BASE__INST1_SEG1                       0x00012000
#define VCN_BASE__INST1_SEG2                       0x02445000
#define VCN_BASE__INST1_SEG3                       0
#define VCN_BASE__INST1_SEG4                       0
#define VCN_BASE__INST1_SEG5                       0

#define VCN_BASE__INST2_SEG0                       0
#define VCN_BASE__INST2_SEG1                       0
#define VCN_BASE__INST2_SEG2                       0
#define VCN_BASE__INST2_SEG3                       0
#define VCN_BASE__INST2_SEG4                       0
#define VCN_BASE__INST2_SEG5                       0

#define VCN_BASE__INST3_SEG0                       0
#define VCN_BASE__INST3_SEG1                       0
#define VCN_BASE__INST3_SEG2                       0
#define VCN_BASE__INST3_SEG3                       0
#define VCN_BASE__INST3_SEG4                       0
#define VCN_BASE__INST3_SEG5                       0

#define VCN_BASE__INST4_SEG0                       0
#define VCN_BASE__INST4_SEG1                       0
#define VCN_BASE__INST4_SEG2                       0
#define VCN_BASE__INST4_SEG3                       0
#define VCN_BASE__INST4_SEG4                       0
#define VCN_BASE__INST4_SEG5                       0

#define VCN_BASE__INST5_SEG0                       0
#define VCN_BASE__INST5_SEG1                       0
#define VCN_BASE__INST5_SEG2                       0
#define VCN_BASE__INST5_SEG3                       0
#define VCN_BASE__INST5_SEG4                       0
#define VCN_BASE__INST5_SEG5                       0

#endif
