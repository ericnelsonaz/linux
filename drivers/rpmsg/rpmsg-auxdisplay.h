#ifndef __RPMSG_AUXDISPLAY_H__
#define __RPMSG_AUXDISPLAY_H__

// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright 2019 NXP
 * Copyright 2022 Technology Advancement Group
 */

/* commands */
#define AUX_CLEAR_DISPLAY	0
#define AUX_UPDATE_DISPLAY	1
#define AUX_DISABLE_DISPLAY	2
#define AUX_ENABLE_DISPLAY	3

#define AUX_XRES 160
#define AUX_YRES 68
#define AUX_BPL (AUX_XRES/8)
#define AUX_DISPBYTES (AUX_YRES*AUX_BPL)

#endif
