/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2018-2020 Oplus. All rights reserved.
 */

#ifndef _OPLUS_FP_COMMON_H_
#define _OPLUS_FP_COMMON_H_

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>

#define FP_ID_MAX_LENGTH                60 /*the length of /proc/fp_id should less than FP_ID_MAX_LENGTH !!!*/
#define ENGINEER_MENU_SELECT_MAXLENTH   20
#define FP_ID_SUFFIX_MAX_LENGTH         30 /*the length of FP ID SUFFIX !!!*/
#define MAX_ID_AMOUNT                   3 /*normally ,the id amount should be less than 3*/



typedef enum {
    FP_SILEAD_6159 = 0,
    FP_JIIOV_0101 = 1,
    FP_UNKNOWN,
} fp_vendor_t;

enum {
    FP_OK,
    FP_ERROR_GPIO,
    FP_ERROR_GENERAL,
};
typedef void (*finger_screen)(int);


fp_vendor_t get_fpsensor_type(void);


#endif  /*_OPLUS_FP_COMMON_H_*/
