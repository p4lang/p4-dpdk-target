/*
 * Copyright(c) 2021 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <regex.h>

#include <osdep/p4_sde_osdep.h>
#include <osdep/p4_sde_osdep.h>
#include <dvm/bf_drv_intf.h>
#include <lld/bf_dev_if.h>
#include "lld.h"
#include "lld_dev.h"
#include "lld_log.h"
#include <lld/python_shell_mutex.h>

// mutex to protect python cli used by lld-python and bf-python
py_shell_context_t py_shell_ctx;

struct lld_log_cfg_t {
	u8 glbl_logs_en;
	u8 chip_logs_en[BF_MAX_DEV_COUNT];
};

struct lld_log_cfg_t lld_log_cfg;

bf_sys_mutex_t lld_log_mtx;  // lock for main LLD log

int lld_log_mtx_initd;

void lld_log_mtx_init(void)
{
	if (!lld_log_mtx_initd) {
		int x;

		lld_log_mtx_initd = 1;

		x = bf_sys_mutex_init(&lld_log_mtx);
		if (x) {
			printf("Error: LLD log lock init failed: <%d>\n", x);
			bf_sys_assert(0);
		} else {
			lld_log_mtx_initd = 1;
		}
	}
}

void lld_debug_init(void)
{
	lld_log_mtx_init();
	// Initialize the python lock
	INIT_PYTHON_SHL_LOCK();
}

void lld_dump_timeval(struct timeval *tm)
{
	char tbuf[256] = {0};
	char ubuf[256] = {0};
	struct tm *loctime;

	loctime = localtime(&tm->tv_sec);

	if (loctime) {
		strftime(tbuf, sizeof(tbuf), "%a %b %d", loctime);
		printf("%s ", tbuf);

		strftime(ubuf, sizeof(ubuf), "%T\n", loctime);
		ubuf[strlen(ubuf) - 1] = 0;  // remove CR
		printf("%s.%06d", ubuf, (int)tm->tv_usec);
	}
}

void lld_log_settings(void)
{
	int i;

	printf("glbl_logs_en: %d\n", lld_log_cfg.glbl_logs_en);
	printf("chip_logs_en: ");
	for (i = 0; i < BF_MAX_DEV_COUNT; i++) {
		if (lld_log_cfg.chip_logs_en[i])
			printf("%d:%d ", i, lld_log_cfg.chip_logs_en[i]);
	}
	printf("\n");
}

/*******************************************************************
 * lld_log_worthy
 *
 * Determine whether or not to log an event.
 *
 * - Logs can be disabled entirely by setting,0
 *     lld_log_cfg.glbl_logs_en=0
 *
 * - If logs are not completely disabled then logging may be enabled
 *   on a per-{ object, verbosity } basis.
 * - Each log has a LOG_TYP, based on the object the log describes.
 * - Each object is described by the LOG_TYP and the first two
 *   parameters, p1 and p2, with p1 generally being "chip"
 * - Each LOG_TYP has a per-object verbosity-level, 0-255, with
 *   "0" meaning disabled. To be logged the event must have a
 *   verbosity-level less-than or equal-to the verbosity-level for
 *   the referenced object.
 *******************************************************************/
int lld_log_worthy(enum lld_log_type_e typ, int p1, int p2, int p3)
{
	if (lld_log_cfg.glbl_logs_en == 0)
		return 0;

	switch (typ) {
	case LOG_TYP_GLBL:
		/* p1=n/a, p2=n/a, p3=verbosity-level */
		return (lld_log_cfg.glbl_logs_en >= p3);
	case LOG_TYP_CHIP:
		/* p1=chip, p2=n/a, p3=verbosity-level */
		return (lld_log_cfg.chip_logs_en[p1] >= p3);
	}
	return 0;
}

int lld_log_set(enum lld_log_type_e typ, int p1, int p2, int p3)
{
	if (p3 > 255)
		return -1;

	switch (typ) {
	case LOG_TYP_GLBL:
		lld_log_cfg.glbl_logs_en = p3;
		break;
	case LOG_TYP_CHIP:
		/* p1=chip, p2=n/a, p3=verbosity-level */
		if (p1 >= BF_MAX_DEV_COUNT)
			return -1;
		lld_log_cfg.chip_logs_en[p1] = p3;
		break;
	}
	return 0;
}
