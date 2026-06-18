/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __SMI_DEBUG_H__
#define __SMI_DEBUG_H__

#include <stdio.h>

#define RST  "\x1B[0m\n"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

#define FRED(x) (KRED x RST)
#define FGRN(x) (KGRN x RST)
#define FYEL(x) (KYEL x RST)
#define FBLU(x) (KBLU x RST)
#define FMAG(x) (KMAG x RST)
#define FCYN(x) (KCYN x RST)
#define FWHT(x) (KWHT x RST)

#define BOLD(x) ("\x1B[1m" x RST)
#define UNDL(x) ("\x1B[4m" x RST)

#ifdef SMI_ENABLE_LOGGING
#define SMI_LOG_BASE(COLOR, x, ...)                   \
	do {                                          \
		printf("%s:%d ", __FILE__, __LINE__); \
		printf(COLOR(x), ##__VA_ARGS__);      \
	} while (0);
#else
#define SMI_LOG_BASE(COLOR, x, ...)
#endif

#define SMI_DEBUG(x, ...)   SMI_LOG_BASE(FGRN, x, ##__VA_ARGS__)
#define SMI_WARNING(x, ...) SMI_LOG_BASE(FYEL, x, ##__VA_ARGS__)
#define SMI_ERROR(x, ...)   SMI_LOG_BASE(FRED, x, ##__VA_ARGS__)

#ifdef SMI_ENABLE_LOGGING
#define SMI_ERROR_MESSAGE(ret)                                                                \
	do {                                                                                  \
		if (ret != AMDSMI_STATUS_RETRY) {                                             \
			printf("%s:%d \x1B[31m %s:%d \x1B[0m\n", __FUNCTION__, __LINE__,      \
				smi_get_error_message(ret), ret);                             \
		}                                                                             \
	} while (0);
#else
#define SMI_ERROR_MESSAGE(ret)
#endif

#endif //__SMI_DEBUG_H__
