// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (c) 2012 The Chromium OS Authors.
 *
 * (C) Copyright 2010
 * Petr Stetiar <ynezz@true.cz>
 *
 * Contains stolen code from ddcprobe project which is:
 * Copyright (C) Nalin Dahyabhai <bigfun@pobox.com>
 */

#include <common.h>
#include <edid.h>
#include <errno.h>
#include <fdtdec.h>
#include <linux/ctype.h>
#include <linux/string.h>

#define CEA_MODE_MATCH_TIMINGS (0)
#define CEA_MODE_MATCH_FLAGS   (1)

#define DRM_MODE_FLAG_PHSYNC	DISPLAY_FLAGS_HSYNC_HIGH
#define DRM_MODE_FLAG_NHSYNC	DISPLAY_FLAGS_HSYNC_LOW
#define DRM_MODE_FLAG_PVSYNC	DISPLAY_FLAGS_VSYNC_HIGH
#define DRM_MODE_FLAG_NVSYNC	DISPLAY_FLAGS_VSYNC_LOW
#define DRM_MODE_FLAG_INTERLACE	DISPLAY_FLAGS_INTERLACED
#define DRM_MODE_FLAG_DBLSCAN	DISPLAY_FLAGS_DOUBLESCAN
#define DRM_MODE_FLAG_DBLCLK	DISPLAY_FLAGS_DOUBLECLK

#define TIMING_ENTRY(a)  .min = (a), .typ = (a), .max = (a)

#define DRM_MODE(nm, t, c, hd, hss, hse, ht, hsk, vd, vss, vse, vt, vs, f) \
	.pixelclock = { TIMING_ENTRY((c)) }, \
	.hactive =  { TIMING_ENTRY((hd)) }, \
	.hfront_porch = { TIMING_ENTRY((ht) - (hse)) },\
	.hback_porch = { TIMING_ENTRY((hss) - (hd)) }, \
	.hsync_len = { TIMING_ENTRY((hse) - (hss)) }, \
	.vactive =  { TIMING_ENTRY((vd)) }, \
	.vfront_porch = { TIMING_ENTRY((vt) - (vse)) }, \
	.hback_porch = { TIMING_ENTRY((vss) - (vd)) }, \
	.hsync_len = { TIMING_ENTRY((vse) - (vss)) }, \
	.flags = (f)

/*
 * Probably taken from CEA-861 spec.
 * This table is converted from xorg's hw/xfree86/modes/xf86EdidModes.c.
 *
 * Index using the VIC.
 */
#define DRM_MODE_TYPE_DRIVER	(0)

static const struct display_timing edid_cea_modes[] = {
	/* 0 - dummy, VICs start at 1 */
	{ },
	/* 1 - 640x480@60Hz 4:3 */
	{ DRM_MODE("640x480", DRM_MODE_TYPE_DRIVER, 25175, 640, 656,
		   752, 800, 0, 480, 490, 492, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 2 - 720x480@60Hz 4:3 */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 27000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 3 - 720x480@60Hz 16:9 */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 27000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 4 - 1280x720@60Hz 16:9 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 5 - 1920x1080i@60Hz 16:9 */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
		   DRM_MODE_FLAG_INTERLACE),
	},
	/* 6 - 720(1440)x480i@60Hz 4:3 */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 13500, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 7 - 720(1440)x480i@60Hz 16:9 */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 13500, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 8 - 720(1440)x240@60Hz 4:3 */
	{ DRM_MODE("720x240", DRM_MODE_TYPE_DRIVER, 13500, 720, 739,
		   801, 858, 0, 240, 244, 247, 262, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_DBLCLK),
	},
	/* 9 - 720(1440)x240@60Hz 16:9 */
	{ DRM_MODE("720x240", DRM_MODE_TYPE_DRIVER, 13500, 720, 739,
		   801, 858, 0, 240, 244, 247, 262, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_DBLCLK),
	},
	/* 10 - 2880x480i@60Hz 4:3 */
	{ DRM_MODE("2880x480i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
		   3204, 3432, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE),
	},
	/* 11 - 2880x480i@60Hz 16:9 */
	{ DRM_MODE("2880x480i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
		   3204, 3432, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE),
	},
	/* 12 - 2880x240@60Hz 4:3 */
	{ DRM_MODE("2880x240", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
		   3204, 3432, 0, 240, 244, 247, 262, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 13 - 2880x240@60Hz 16:9 */
	{ DRM_MODE("2880x240", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2956,
		   3204, 3432, 0, 240, 244, 247, 262, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 14 - 1440x480@60Hz 4:3 */
	{ DRM_MODE("1440x480", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1472,
		   1596, 1716, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 15 - 1440x480@60Hz 16:9 */
	{ DRM_MODE("1440x480", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1472,
		   1596, 1716, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 16 - 1920x1080@60Hz 16:9 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 17 - 720x576@50Hz 4:3 */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 27000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 18 - 720x576@50Hz 16:9 */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 27000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 19 - 1280x720@50Hz 16:9 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1720,
		   1760, 1980, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 20 - 1920x1080i@50Hz 16:9 */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
		   DRM_MODE_FLAG_INTERLACE),
	},
	/* 21 - 720(1440)x576i@50Hz 4:3 */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 13500, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 22 - 720(1440)x576i@50Hz 16:9 */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 13500, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 23 - 720(1440)x288@50Hz 4:3 */
	{ DRM_MODE("720x288", DRM_MODE_TYPE_DRIVER, 13500, 720, 732,
		   795, 864, 0, 288, 290, 293, 312, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_DBLCLK),
	},
	/* 24 - 720(1440)x288@50Hz 16:9 */
	{ DRM_MODE("720x288", DRM_MODE_TYPE_DRIVER, 13500, 720, 732,
		   795, 864, 0, 288, 290, 293, 312, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_DBLCLK),
	},
	/* 25 - 2880x576i@50Hz 4:3 */
	{ DRM_MODE("2880x576i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
		   3180, 3456, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE),
	},
	/* 26 - 2880x576i@50Hz 16:9 */
	{ DRM_MODE("2880x576i", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
		   3180, 3456, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE),
	},
	/* 27 - 2880x288@50Hz 4:3 */
	{ DRM_MODE("2880x288", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
		   3180, 3456, 0, 288, 290, 293, 312, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 28 - 2880x288@50Hz 16:9 */
	{ DRM_MODE("2880x288", DRM_MODE_TYPE_DRIVER, 54000, 2880, 2928,
		   3180, 3456, 0, 288, 290, 293, 312, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 29 - 1440x576@50Hz 4:3 */
	{ DRM_MODE("1440x576", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1464,
		   1592, 1728, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 30 - 1440x576@50Hz 16:9 */
	{ DRM_MODE("1440x576", DRM_MODE_TYPE_DRIVER, 54000, 1440, 1464,
		   1592, 1728, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 31 - 1920x1080@50Hz 16:9 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 32 - 1920x1080@24Hz 16:9 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2558,
		   2602, 2750, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 33 - 1920x1080@25Hz 16:9 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 34 - 1920x1080@30Hz 16:9 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 35 - 2880x480@60Hz 4:3 */
	{ DRM_MODE("2880x480", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2944,
		   3192, 3432, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 36 - 2880x480@60Hz 16:9 */
	{ DRM_MODE("2880x480", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2944,
		   3192, 3432, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 37 - 2880x576@50Hz 4:3 */
	{ DRM_MODE("2880x576", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2928,
		   3184, 3456, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 38 - 2880x576@50Hz 16:9 */
	{ DRM_MODE("2880x576", DRM_MODE_TYPE_DRIVER, 108000, 2880, 2928,
		   3184, 3456, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 39 - 1920x1080i@50Hz 16:9 */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 72000, 1920, 1952,
		   2120, 2304, 0, 1080, 1126, 1136, 1250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE),
	},
	/* 40 - 1920x1080i@100Hz 16:9 */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
		   DRM_MODE_FLAG_INTERLACE),
	},
	/* 41 - 1280x720@100Hz 16:9 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1720,
		   1760, 1980, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 42 - 720x576@100Hz 4:3 */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 54000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 43 - 720x576@100Hz 16:9 */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 54000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 44 - 720(1440)x576i@100Hz 4:3 */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 27000, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 45 - 720(1440)x576i@100Hz 16:9 */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 27000, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 46 - 1920x1080i@120Hz 16:9 */
	{ DRM_MODE("1920x1080i", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1094, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC |
		   DRM_MODE_FLAG_INTERLACE),
	},
	/* 47 - 1280x720@120Hz 16:9 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 48 - 720x480@120Hz 4:3 */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 54000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 49 - 720x480@120Hz 16:9 */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 54000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 50 - 720(1440)x480i@120Hz 4:3 */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 27000, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 51 - 720(1440)x480i@120Hz 16:9 */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 27000, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 52 - 720x576@200Hz 4:3 */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 108000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 53 - 720x576@200Hz 16:9 */
	{ DRM_MODE("720x576", DRM_MODE_TYPE_DRIVER, 108000, 720, 732,
		   796, 864, 0, 576, 581, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 54 - 720(1440)x576i@200Hz 4:3 */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 54000, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 55 - 720(1440)x576i@200Hz 16:9 */
	{ DRM_MODE("720x576i", DRM_MODE_TYPE_DRIVER, 54000, 720, 732,
		   795, 864, 0, 576, 580, 586, 625, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 56 - 720x480@240Hz 4:3 */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 108000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 57 - 720x480@240Hz 16:9 */
	{ DRM_MODE("720x480", DRM_MODE_TYPE_DRIVER, 108000, 720, 736,
		   798, 858, 0, 480, 489, 495, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC),
	},
	/* 58 - 720(1440)x480i@240Hz 4:3 */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 54000, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 59 - 720(1440)x480i@240Hz 16:9 */
	{ DRM_MODE("720x480i", DRM_MODE_TYPE_DRIVER, 54000, 720, 739,
		   801, 858, 0, 480, 488, 494, 525, 0,
		   DRM_MODE_FLAG_NHSYNC | DRM_MODE_FLAG_NVSYNC |
		   DRM_MODE_FLAG_INTERLACE | DRM_MODE_FLAG_DBLCLK),
	},
	/* 60 - 1280x720@24Hz 16:9 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 59400, 1280, 3040,
		   3080, 3300, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 61 - 1280x720@25Hz 16:9 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3700,
		   3740, 3960, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 62 - 1280x720@30Hz 16:9 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3040,
		   3080, 3300, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 63 - 1920x1080@120Hz 16:9 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 64 - 1920x1080@100Hz 16:9 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 65 - 1280x720@24Hz 64:27 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 59400, 1280, 3040,
		   3080, 3300, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 66 - 1280x720@25Hz 64:27 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3700,
		   3740, 3960, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 67 - 1280x720@30Hz 64:27 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 3040,
		   3080, 3300, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 68 - 1280x720@50Hz 64:27 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1720,
		   1760, 1980, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 69 - 1280x720@60Hz 64:27 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 74250, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 70 - 1280x720@100Hz 64:27 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1720,
		   1760, 1980, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 71 - 1280x720@120Hz 64:27 */
	{ DRM_MODE("1280x720", DRM_MODE_TYPE_DRIVER, 148500, 1280, 1390,
		   1430, 1650, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 72 - 1920x1080@24Hz 64:27 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2558,
		   2602, 2750, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 73 - 1920x1080@25Hz 64:27 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 74 - 1920x1080@30Hz 64:27 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 74250, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 75 - 1920x1080@50Hz 64:27 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 76 - 1920x1080@60Hz 64:27 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 148500, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 77 - 1920x1080@100Hz 64:27 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2448,
		   2492, 2640, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 78 - 1920x1080@120Hz 64:27 */
	{ DRM_MODE("1920x1080", DRM_MODE_TYPE_DRIVER, 297000, 1920, 2008,
		   2052, 2200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 79 - 1680x720@24Hz 64:27 */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 59400, 1680, 3040,
		   3080, 3300, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 80 - 1680x720@25Hz 64:27 */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 59400, 1680, 2908,
		   2948, 3168, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 81 - 1680x720@30Hz 64:27 */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 59400, 1680, 2380,
		   2420, 2640, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 82 - 1680x720@50Hz 64:27 */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 82500, 1680, 1940,
		   1980, 2200, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 83 - 1680x720@60Hz 64:27 */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 99000, 1680, 1940,
		   1980, 2200, 0, 720, 725, 730, 750, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 84 - 1680x720@100Hz 64:27 */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 165000, 1680, 1740,
		   1780, 2000, 0, 720, 725, 730, 825, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 85 - 1680x720@120Hz 64:27 */
	{ DRM_MODE("1680x720", DRM_MODE_TYPE_DRIVER, 198000, 1680, 1740,
		   1780, 2000, 0, 720, 725, 730, 825, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 86 - 2560x1080@24Hz 64:27 */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 99000, 2560, 3558,
		   3602, 3750, 0, 1080, 1084, 1089, 1100, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 87 - 2560x1080@25Hz 64:27 */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 90000, 2560, 3008,
		   3052, 3200, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 88 - 2560x1080@30Hz 64:27 */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 118800, 2560, 3328,
		   3372, 3520, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 89 - 2560x1080@50Hz 64:27 */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 185625, 2560, 3108,
		   3152, 3300, 0, 1080, 1084, 1089, 1125, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 90 - 2560x1080@60Hz 64:27 */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 198000, 2560, 2808,
		   2852, 3000, 0, 1080, 1084, 1089, 1100, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 91 - 2560x1080@100Hz 64:27 */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 371250, 2560, 2778,
		   2822, 2970, 0, 1080, 1084, 1089, 1250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 92 - 2560x1080@120Hz 64:27 */
	{ DRM_MODE("2560x1080", DRM_MODE_TYPE_DRIVER, 495000, 2560, 3108,
		   3152, 3300, 0, 1080, 1084, 1089, 1250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 93 - 3840x2160@24Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 5116,
		   5204, 5500, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 94 - 3840x2160@25Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4896,
		   4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 95 - 3840x2160@30Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4016,
		   4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 96 - 3840x2160@50Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4896,
		   4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 97 - 3840x2160@60Hz 16:9 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016,
		   4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 98 - 4096x2160@24Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 297000, 4096, 5116,
		   5204, 5500, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 99 - 4096x2160@25Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 297000, 4096, 5064,
		   5152, 5280, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 100 - 4096x2160@30Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 297000, 4096, 4184,
		   4272, 4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 101 - 4096x2160@50Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 594000, 4096, 5064,
		   5152, 5280, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 102 - 4096x2160@60Hz 256:135 */
	{ DRM_MODE("4096x2160", DRM_MODE_TYPE_DRIVER, 594000, 4096, 4184,
		   4272, 4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 103 - 3840x2160@24Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 5116,
		   5204, 5500, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 104 - 3840x2160@25Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4896,
		   4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 105 - 3840x2160@30Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 297000, 3840, 4016,
		   4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 106 - 3840x2160@50Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4896,
		   4984, 5280, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
	/* 107 - 3840x2160@60Hz 64:27 */
	{ DRM_MODE("3840x2160", DRM_MODE_TYPE_DRIVER, 594000, 3840, 4016,
		   4104, 4400, 0, 2160, 2168, 2178, 2250, 0,
		   DRM_MODE_FLAG_PHSYNC | DRM_MODE_FLAG_PVSYNC),
	},
};

#define KHZ2PICOS(a) (1000000000UL/(a))

static bool cea_mode_match_timings(const struct display_timing *mode1,
				   const struct display_timing *mode2)

{
	return (mode1->hactive.typ == mode2->hactive.typ &&
		mode1->hfront_porch.typ == mode2->hfront_porch.typ &&
		mode1->hback_porch.typ == mode2->hback_porch.typ &&
		mode1->hsync_len.typ == mode2->hsync_len.typ &&
		mode1->vactive.typ == mode2->vactive.typ &&
		mode1->vfront_porch.typ == mode2->vfront_porch.typ &&
		mode1->vback_porch.typ == mode2->vback_porch.typ &&
		mode1->vsync_len.typ == mode2->vsync_len.typ);

}

static bool cea_mode_match_flags(const struct display_timing *mode1,
				 const struct display_timing *mode2)
{
	return (mode1->flags == mode2->flags);
}

static bool cea_mode_match(const struct display_timing *mode1,
			   const struct display_timing *mode2,
			   unsigned int match_flags)
{
	if (!mode1 && !mode2)
		return true;

	if (!mode1 || !mode2)
		return false;

	if (match_flags & CEA_MODE_MATCH_TIMINGS &&
	    !cea_mode_match_timings(mode1, mode2))
		return false;

	if (match_flags & CEA_MODE_MATCH_FLAGS &&
	    !cea_mode_match_flags(mode1, mode2))
		return false;

	return true;
}

static unsigned int
cea_mode_alternate_clock(const struct display_timing *cea_mode)
{
	unsigned int clock = cea_mode->pixelclock.typ;

	/*
	 * edid_cea_modes contains the 59.94Hz
	 * variant for 240 and 480 line modes,
	 * and the 60Hz variant otherwise.
	 */
	if (cea_mode->vactive.typ == 240 || cea_mode->vactive.typ == 480)
		clock = DIV_ROUND_CLOSEST(clock * 1001, 1000);
	else
		clock = DIV_ROUND_CLOSEST(clock * 1000, 1001);

	return clock;
}

static bool
cea_mode_alternate_timings(u8 vic, struct display_timing *mode)
{
	/*
	 * For certain VICs the spec allows the vertical
	 * front porch to vary by one or two lines.
	 *
	 * cea_modes[] stores the variant with the shortest
	 * vertical front porch. We can adjust the mode to
	 * get the other variants by simply increasing the
	 * vertical front porch length.
	 */
	int mode_vtotal = mode->vback_porch.typ +
			  mode->vactive.typ +
			  mode->vfront_porch.typ +
			  mode->vsync_len.typ;

	if (((vic == 8 || vic == 9 ||
	      vic == 12 || vic == 13) && mode_vtotal < 263) ||
	    ((vic == 23 || vic == 24 ||
	      vic == 27 || vic == 28) && mode_vtotal < 314)) {
		mode->vfront_porch.typ++;
		mode->vfront_porch.min++;
		mode->vfront_porch.max++;
		return true;
	}

	return false;
}

int edid_match_cea_mode(const struct display_timing *to_match)
{
	unsigned int match_flags = CEA_MODE_MATCH_TIMINGS | CEA_MODE_MATCH_FLAGS;
	u32 clock = to_match->pixelclock.typ / 1000;
	int vic;

	if (!clock)
		return 0;

	for (vic = 1; vic < ARRAY_SIZE(edid_cea_modes); vic++) {
		struct display_timing cea_mode = edid_cea_modes[vic];
		unsigned int clock1, clock2;

		/* Check both 60Hz and 59.94Hz */
		clock1 = cea_mode.pixelclock.typ;
		clock2 = cea_mode_alternate_clock(&cea_mode);

		if (KHZ2PICOS(clock) != KHZ2PICOS(clock1) &&
		    KHZ2PICOS(clock) != KHZ2PICOS(clock2))
			continue;

		do {
			if (cea_mode_match(to_match, &cea_mode, match_flags))
				return vic;
		} while (cea_mode_alternate_timings(vic, &cea_mode));
	}

	return 0;
}

int edid_check_info(struct edid1_info *edid_info)
{
	if ((edid_info == NULL) || (edid_info->version == 0))
		return -1;

	if (memcmp(edid_info->header, "\x0\xff\xff\xff\xff\xff\xff\x0", 8))
		return -1;

	if (edid_info->version == 0xff && edid_info->revision == 0xff)
		return -1;

	return 0;
}

int edid_check_checksum(u8 *edid_block)
{
	u8 checksum = 0;
	int i;

	for (i = 0; i < 128; i++)
		checksum += edid_block[i];

	return (checksum == 0) ? 0 : -EINVAL;
}

int edid_get_ranges(struct edid1_info *edid, unsigned int *hmin,
		    unsigned int *hmax, unsigned int *vmin,
		    unsigned int *vmax)
{
	int i;
	struct edid_monitor_descriptor *monitor;

	*hmin = *hmax = *vmin = *vmax = 0;
	if (edid_check_info(edid))
		return -1;

	for (i = 0; i < ARRAY_SIZE(edid->monitor_details.descriptor); i++) {
		monitor = &edid->monitor_details.descriptor[i];
		if (monitor->type == EDID_MONITOR_DESCRIPTOR_RANGE) {
			*hmin = monitor->data.range_data.horizontal_min;
			*hmax = monitor->data.range_data.horizontal_max;
			*vmin = monitor->data.range_data.vertical_min;
			*vmax = monitor->data.range_data.vertical_max;
			return 0;
		}
	}
	return -1;
}

/* Set all parts of a timing entry to the same value */
static void set_entry(struct timing_entry *entry, u32 value)
{
	entry->min = value;
	entry->typ = value;
	entry->max = value;
}

/**
 * decode_timing() - Decoding an 18-byte detailed timing record
 *
 * @buf:	Pointer to EDID detailed timing record
 * @timing:	Place to put timing
 */
static void decode_timing(u8 *buf, struct display_timing *timing)
{
	uint x_mm, y_mm;
	unsigned int ha, hbl, hso, hspw, hborder;
	unsigned int va, vbl, vso, vspw, vborder;
	struct edid_detailed_timing *t = (struct edid_detailed_timing *)buf;

	/* Edid contains pixel clock in terms of 10KHz */
	set_entry(&timing->pixelclock, (buf[0] + (buf[1] << 8)) * 10000);
	x_mm = (buf[12] + ((buf[14] & 0xf0) << 4));
	y_mm = (buf[13] + ((buf[14] & 0x0f) << 8));
	ha = (buf[2] + ((buf[4] & 0xf0) << 4));
	hbl = (buf[3] + ((buf[4] & 0x0f) << 8));
	hso = (buf[8] + ((buf[11] & 0xc0) << 2));
	hspw = (buf[9] + ((buf[11] & 0x30) << 4));
	hborder = buf[15];
	va = (buf[5] + ((buf[7] & 0xf0) << 4));
	vbl = (buf[6] + ((buf[7] & 0x0f) << 8));
	vso = ((buf[10] >> 4) + ((buf[11] & 0x0c) << 2));
	vspw = ((buf[10] & 0x0f) + ((buf[11] & 0x03) << 4));
	vborder = buf[16];

	set_entry(&timing->hactive, ha);
	set_entry(&timing->hfront_porch, hso);
	set_entry(&timing->hback_porch, hbl - hso - hspw);
	set_entry(&timing->hsync_len, hspw);

	set_entry(&timing->vactive, va);
	set_entry(&timing->vfront_porch, vso);
	set_entry(&timing->vback_porch, vbl - vso - vspw);
	set_entry(&timing->vsync_len, vspw);

	timing->flags = 0;
	if (EDID_DETAILED_TIMING_FLAG_HSYNC_POLARITY(*t))
		timing->flags |= DISPLAY_FLAGS_HSYNC_HIGH;
	else
		timing->flags |= DISPLAY_FLAGS_HSYNC_LOW;
	if (EDID_DETAILED_TIMING_FLAG_VSYNC_POLARITY(*t))
		timing->flags |= DISPLAY_FLAGS_VSYNC_HIGH;
	else
		timing->flags |= DISPLAY_FLAGS_VSYNC_LOW;

	if (EDID_DETAILED_TIMING_FLAG_INTERLACED(*t))
		timing->flags = DISPLAY_FLAGS_INTERLACED;

	debug("Detailed mode clock %u Hz, %d mm x %d mm\n"
	      "               %04x %04x %04x %04x hborder %x\n"
	      "               %04x %04x %04x %04x vborder %x\n",
	      timing->pixelclock.typ,
	      x_mm, y_mm,
	      ha, ha + hso, ha + hso + hspw,
	      ha + hbl, hborder,
	      va, va + vso, va + vso + vspw,
	      va + vbl, vborder);
}

/**
 * Check if HDMI vendor specific data block is present in CEA block
 * @param info	CEA extension block
 * @return true if block is found
 */
static bool cea_is_hdmi_vsdb_present(struct edid_cea861_info *info)
{
	u8 end, i = 0;

	/* check for end of data block */
	end = info->dtd_offset;
	if (end == 0)
		end = sizeof(info->data);
	if (end < 4 || end > sizeof(info->data))
		return false;
	end -= 4;

	while (i < end) {
		/* Look for vendor specific data block of appropriate size */
		if ((EDID_CEA861_DB_TYPE(*info, i) == EDID_CEA861_DB_VENDOR) &&
		    (EDID_CEA861_DB_LEN(*info, i) >= 5)) {
			u8 *db = &info->data[i + 1];
			u32 oui = db[0] | (db[1] << 8) | (db[2] << 16);

			if (oui == HDMI_IEEE_OUI)
				return true;
		}
		i += EDID_CEA861_DB_LEN(*info, i) + 1;
	}

	return false;
}

int edid_get_timing(u8 *buf, int buf_size, struct display_timing *timing,
		    int *panel_bits_per_colourp)
{
	struct edid1_info *edid = (struct edid1_info *)buf;
	bool timing_done;
	int i;

	if (buf_size < sizeof(*edid) || edid_check_info(edid)) {
		debug("%s: Invalid buffer\n", __func__);
		return -EINVAL;
	}

	if (!EDID1_INFO_FEATURE_PREFERRED_TIMING_MODE(*edid)) {
		debug("%s: No preferred timing\n", __func__);
		return -ENOENT;
	}

	/* Look for detailed timing */
	timing_done = false;
	for (i = 0; i < 4; i++) {
		struct edid_monitor_descriptor *desc;

		desc = &edid->monitor_details.descriptor[i];
		if (desc->zero_flag_1 != 0) {
			decode_timing((u8 *)desc, timing);
			timing_done = true;
			break;
		}
	}
	if (!timing_done) {
		debug("%s: timing not completed\n", __func__);
		return -EINVAL;
	}

	if (!EDID1_INFO_VIDEO_INPUT_DIGITAL(*edid)) {
		debug("%s: Not a digital display\n", __func__);
		return -ENOSYS;
	}
	if (edid->version != 1 || edid->revision < 4) {
		debug("%s: EDID version %d.%d does not have required info\n",
		      __func__, edid->version, edid->revision);
		*panel_bits_per_colourp = -1;
	} else  {
		*panel_bits_per_colourp =
			((edid->video_input_definition & 0x70) >> 3) + 4;
	}

	timing->hdmi_monitor = false;
	if (edid->extension_flag && (buf_size >= EDID_EXT_SIZE)) {
		struct edid_cea861_info *info =
			(struct edid_cea861_info *)(buf + sizeof(*edid));

		if (info->extension_tag == EDID_CEA861_EXTENSION_TAG)
			timing->hdmi_monitor = cea_is_hdmi_vsdb_present(info);
	}

	return 0;
}

/**
 * Snip the tailing whitespace/return of a string.
 *
 * @param string	The string to be snipped
 * @return the snipped string
 */
static char *snip(char *string)
{
	char *s;

	/*
	 * This is always a 13 character buffer
	 * and it's not always terminated.
	 */
	string[12] = '\0';
	s = &string[strlen(string) - 1];

	while (s >= string && (isspace(*s) || *s == '\n' || *s == '\r' ||
			*s == '\0'))
		*(s--) = '\0';

	return string;
}

/**
 * Print an EDID monitor descriptor block
 *
 * @param monitor	The EDID monitor descriptor block
 * @have_timing		Modifies to 1 if the desciptor contains timing info
 */
static void edid_print_dtd(struct edid_monitor_descriptor *monitor,
			   unsigned int *have_timing)
{
	unsigned char *bytes = (unsigned char *)monitor;
	struct edid_detailed_timing *timing =
			(struct edid_detailed_timing *)monitor;

	if (bytes[0] == 0 && bytes[1] == 0) {
		if (monitor->type == EDID_MONITOR_DESCRIPTOR_SERIAL)
			printf("Monitor serial number: %s\n",
			       snip(monitor->data.string));
		else if (monitor->type == EDID_MONITOR_DESCRIPTOR_ASCII)
			printf("Monitor ID: %s\n",
			       snip(monitor->data.string));
		else if (monitor->type == EDID_MONITOR_DESCRIPTOR_NAME)
			printf("Monitor name: %s\n",
			       snip(monitor->data.string));
		else if (monitor->type == EDID_MONITOR_DESCRIPTOR_RANGE)
			printf("Monitor range limits, horizontal sync: "
			       "%d-%d kHz, vertical refresh: "
			       "%d-%d Hz, max pixel clock: "
			       "%d MHz\n",
			       monitor->data.range_data.horizontal_min,
			       monitor->data.range_data.horizontal_max,
			       monitor->data.range_data.vertical_min,
			       monitor->data.range_data.vertical_max,
			       monitor->data.range_data.pixel_clock_max * 10);
	} else {
		uint32_t pixclock, h_active, h_blanking, v_active, v_blanking;
		uint32_t h_total, v_total, vfreq;

		pixclock = EDID_DETAILED_TIMING_PIXEL_CLOCK(*timing);
		h_active = EDID_DETAILED_TIMING_HORIZONTAL_ACTIVE(*timing);
		h_blanking = EDID_DETAILED_TIMING_HORIZONTAL_BLANKING(*timing);
		v_active = EDID_DETAILED_TIMING_VERTICAL_ACTIVE(*timing);
		v_blanking = EDID_DETAILED_TIMING_VERTICAL_BLANKING(*timing);

		h_total = h_active + h_blanking;
		v_total = v_active + v_blanking;
		if (v_total > 0 && h_total > 0)
			vfreq = pixclock / (v_total * h_total);
		else
			vfreq = 1; /* Error case */
		printf("\t%dx%d\%c\t%d Hz (detailed)\n", h_active,
		       v_active, h_active > 1000 ? ' ' : '\t', vfreq);
		*have_timing = 1;
	}
}

/**
 * Get the manufacturer name from an EDID info.
 *
 * @param edid_info     The EDID info to be printed
 * @param name		Returns the string of the manufacturer name
 */
static void edid_get_manufacturer_name(struct edid1_info *edid, char *name)
{
	name[0] = EDID1_INFO_MANUFACTURER_NAME_CHAR1(*edid) + 'A' - 1;
	name[1] = EDID1_INFO_MANUFACTURER_NAME_CHAR2(*edid) + 'A' - 1;
	name[2] = EDID1_INFO_MANUFACTURER_NAME_CHAR3(*edid) + 'A' - 1;
	name[3] = '\0';
}

void edid_print_info(struct edid1_info *edid_info)
{
	int i;
	char manufacturer[4];
	unsigned int have_timing = 0;
	uint32_t serial_number;

	if (edid_check_info(edid_info)) {
		printf("Not a valid EDID\n");
		return;
	}

	printf("EDID version: %d.%d\n",
	       edid_info->version, edid_info->revision);

	printf("Product ID code: %04x\n", EDID1_INFO_PRODUCT_CODE(*edid_info));

	edid_get_manufacturer_name(edid_info, manufacturer);
	printf("Manufacturer: %s\n", manufacturer);

	serial_number = EDID1_INFO_SERIAL_NUMBER(*edid_info);
	if (serial_number != 0xffffffff) {
		if (strcmp(manufacturer, "MAG") == 0)
			serial_number -= 0x7000000;
		if (strcmp(manufacturer, "OQI") == 0)
			serial_number -= 456150000;
		if (strcmp(manufacturer, "VSC") == 0)
			serial_number -= 640000000;
	}
	printf("Serial number: %08x\n", serial_number);
	printf("Manufactured in week: %d year: %d\n",
	       edid_info->week, edid_info->year + 1990);

	printf("Video input definition: %svoltage level %d%s%s%s%s%s\n",
	       EDID1_INFO_VIDEO_INPUT_DIGITAL(*edid_info) ?
	       "digital signal, " : "analog signal, ",
	       EDID1_INFO_VIDEO_INPUT_VOLTAGE_LEVEL(*edid_info),
	       EDID1_INFO_VIDEO_INPUT_BLANK_TO_BLACK(*edid_info) ?
	       ", blank to black" : "",
	       EDID1_INFO_VIDEO_INPUT_SEPARATE_SYNC(*edid_info) ?
	       ", separate sync" : "",
	       EDID1_INFO_VIDEO_INPUT_COMPOSITE_SYNC(*edid_info) ?
	       ", composite sync" : "",
	       EDID1_INFO_VIDEO_INPUT_SYNC_ON_GREEN(*edid_info) ?
	       ", sync on green" : "",
	       EDID1_INFO_VIDEO_INPUT_SERRATION_V(*edid_info) ?
	       ", serration v" : "");

	printf("Monitor is %s\n",
	       EDID1_INFO_FEATURE_RGB(*edid_info) ? "RGB" : "non-RGB");

	printf("Maximum visible display size: %d cm x %d cm\n",
	       edid_info->max_size_horizontal,
	       edid_info->max_size_vertical);

	printf("Power management features: %s%s, %s%s, %s%s\n",
	       EDID1_INFO_FEATURE_ACTIVE_OFF(*edid_info) ?
	       "" : "no ", "active off",
	       EDID1_INFO_FEATURE_SUSPEND(*edid_info) ? "" : "no ", "suspend",
	       EDID1_INFO_FEATURE_STANDBY(*edid_info) ? "" : "no ", "standby");

	printf("Estabilished timings:\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_720X400_70(*edid_info))
		printf("\t720x400\t\t70 Hz (VGA 640x400, IBM)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_720X400_88(*edid_info))
		printf("\t720x400\t\t88 Hz (XGA2)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_640X480_60(*edid_info))
		printf("\t640x480\t\t60 Hz (VGA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_640X480_67(*edid_info))
		printf("\t640x480\t\t67 Hz (Mac II, Apple)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_640X480_72(*edid_info))
		printf("\t640x480\t\t72 Hz (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_640X480_75(*edid_info))
		printf("\t640x480\t\t75 Hz (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_800X600_56(*edid_info))
		printf("\t800x600\t\t56 Hz (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_800X600_60(*edid_info))
		printf("\t800x600\t\t60 Hz (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_800X600_72(*edid_info))
		printf("\t800x600\t\t72 Hz (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_800X600_75(*edid_info))
		printf("\t800x600\t\t75 Hz (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_832X624_75(*edid_info))
		printf("\t832x624\t\t75 Hz (Mac II)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_1024X768_87I(*edid_info))
		printf("\t1024x768\t87 Hz Interlaced (8514A)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_1024X768_60(*edid_info))
		printf("\t1024x768\t60 Hz (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_1024X768_70(*edid_info))
		printf("\t1024x768\t70 Hz (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_1024X768_75(*edid_info))
		printf("\t1024x768\t75 Hz (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_1280X1024_75(*edid_info))
		printf("\t1280x1024\t75 (VESA)\n");
	if (EDID1_INFO_ESTABLISHED_TIMING_1152X870_75(*edid_info))
		printf("\t1152x870\t75 (Mac II)\n");

	/* Standard timings. */
	printf("Standard timings:\n");
	for (i = 0; i < ARRAY_SIZE(edid_info->standard_timings); i++) {
		unsigned int aspect = 10000;
		unsigned int x, y;
		unsigned char xres, vfreq;

		xres = EDID1_INFO_STANDARD_TIMING_XRESOLUTION(*edid_info, i);
		vfreq = EDID1_INFO_STANDARD_TIMING_VFREQ(*edid_info, i);
		if ((xres != vfreq) ||
		    ((xres != 0) && (xres != 1)) ||
		    ((vfreq != 0) && (vfreq != 1))) {
			switch (EDID1_INFO_STANDARD_TIMING_ASPECT(*edid_info,
					i)) {
			case ASPECT_625:
				aspect = 6250;
				break;
			case ASPECT_75:
				aspect = 7500;
				break;
			case ASPECT_8:
				aspect = 8000;
				break;
			case ASPECT_5625:
				aspect = 5625;
				break;
			}
			x = (xres + 31) * 8;
			y = x * aspect / 10000;
			printf("\t%dx%d%c\t%d Hz\n", x, y,
			       x > 1000 ? ' ' : '\t', (vfreq & 0x3f) + 60);
			have_timing = 1;
		}
	}

	/* Detailed timing information. */
	for (i = 0; i < ARRAY_SIZE(edid_info->monitor_details.descriptor);
			i++) {
		edid_print_dtd(&edid_info->monitor_details.descriptor[i],
			       &have_timing);
	}

	if (!have_timing)
		printf("\tNone\n");
}
