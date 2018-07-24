// SPDX-License-Identifier: GPL-2.0
/*
 * Amlogic Meson Video Processing Unit driver
 *
 * Copyright (c) 2018 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 */

#include <edid.h>
#include "meson_vpu.h"

enum {
	MESON_VENC_MODE_NONE = 0,
	MESON_VENC_MODE_CVBS_PAL,
	MESON_VENC_MODE_CVBS_NTSC,
	MESON_VENC_MODE_HDMI,
};

enum meson_venc_source {
	MESON_VENC_SOURCE_NONE = 0,
	MESON_VENC_SOURCE_ENCI = 1,
	MESON_VENC_SOURCE_ENCP = 2,
};

#define HHI_VDAC_CNTL0		0x2F4 /* 0xbd offset in data sheet */
#define HHI_VDAC_CNTL1		0x2F8 /* 0xbe offset in data sheet */

struct meson_cvbs_enci_mode {
	unsigned int mode_tag;
	unsigned int hso_begin; /* HSO begin position */
	unsigned int hso_end; /* HSO end position */
	unsigned int vso_even; /* VSO even line */
	unsigned int vso_odd; /* VSO odd line */
	unsigned int macv_max_amp; /* Macrovision max amplitude */
	unsigned int video_prog_mode;
	unsigned int video_mode;
	unsigned int sch_adjust;
	unsigned int yc_delay;
	unsigned int pixel_start;
	unsigned int pixel_end;
	unsigned int top_field_line_start;
	unsigned int top_field_line_end;
	unsigned int bottom_field_line_start;
	unsigned int bottom_field_line_end;
	unsigned int video_saturation;
	unsigned int video_contrast;
	unsigned int video_brightness;
	unsigned int video_hue;
	unsigned int analog_sync_adj;
};

struct meson_cvbs_enci_mode meson_cvbs_enci_pal = {
	.mode_tag = MESON_VENC_MODE_CVBS_PAL,
	.hso_begin = 3,
	.hso_end = 129,
	.vso_even = 3,
	.vso_odd = 260,
	.macv_max_amp = 7,
	.video_prog_mode = 0xff,
	.video_mode = 0x13,
	.sch_adjust = 0x28,
	.yc_delay = 0x343,
	.pixel_start = 251,
	.pixel_end = 1691,
	.top_field_line_start = 22,
	.top_field_line_end = 310,
	.bottom_field_line_start = 23,
	.bottom_field_line_end = 311,
	.video_saturation = 9,
	.video_contrast = 0,
	.video_brightness = 0,
	.video_hue = 0,
	.analog_sync_adj = 0x8080,
};

struct meson_cvbs_enci_mode meson_cvbs_enci_ntsc = {
	.mode_tag = MESON_VENC_MODE_CVBS_NTSC,
	.hso_begin = 5,
	.hso_end = 129,
	.vso_even = 3,
	.vso_odd = 260,
	.macv_max_amp = 0xb,
	.video_prog_mode = 0xf0,
	.video_mode = 0x8,
	.sch_adjust = 0x20,
	.yc_delay = 0x333,
	.pixel_start = 227,
	.pixel_end = 1667,
	.top_field_line_start = 18,
	.top_field_line_end = 258,
	.bottom_field_line_start = 19,
	.bottom_field_line_end = 259,
	.video_saturation = 18,
	.video_contrast = 3,
	.video_brightness = 0,
	.video_hue = 0,
	.analog_sync_adj = 0x9c00,
};


static void meson_venci_cvbs_mode_set(struct meson_vpu_priv *priv,
			     	      struct meson_cvbs_enci_mode *mode)
{
	/* CVBS Filter settings */
	writel(0x12, priv->io_base + _REG(ENCI_CFILT_CTRL));
	writel(0x12, priv->io_base + _REG(ENCI_CFILT_CTRL2));

	/* Digital Video Select : Interlace, clk27 clk, external */
	writel(0, priv->io_base + _REG(VENC_DVI_SETTING));

	/* Reset Video Mode */
	writel(0, priv->io_base + _REG(ENCI_VIDEO_MODE));
	writel(0, priv->io_base + _REG(ENCI_VIDEO_MODE_ADV));

	/* Horizontal sync signal output */
	writel(mode->hso_begin,
			priv->io_base + _REG(ENCI_SYNC_HSO_BEGIN));
	writel(mode->hso_end,
			priv->io_base + _REG(ENCI_SYNC_HSO_END));

	/* Vertical Sync lines */
	writel(mode->vso_even,
			priv->io_base + _REG(ENCI_SYNC_VSO_EVNLN));
	writel(mode->vso_odd,
			priv->io_base + _REG(ENCI_SYNC_VSO_ODDLN));

	/* Macrovision max amplitude change */
	writel(0x8100 + mode->macv_max_amp,
			priv->io_base + _REG(ENCI_MACV_MAX_AMP));

	/* Video mode */
	writel(mode->video_prog_mode,
			priv->io_base + _REG(VENC_VIDEO_PROG_MODE));
	writel(mode->video_mode,
			priv->io_base + _REG(ENCI_VIDEO_MODE));

	/* Advanced Video Mode :
	 * Demux shifting 0x2
	 * Blank line end at line17/22
	 * High bandwidth Luma Filter
	 * Low bandwidth Chroma Filter
	 * Bypass luma low pass filter
	 * No macrovision on CSYNC
	 */
	writel(0x26, priv->io_base + _REG(ENCI_VIDEO_MODE_ADV));

	writel(mode->sch_adjust, priv->io_base + _REG(ENCI_VIDEO_SCH));

	/* Sync mode : MASTER Master mode, free run, send HSO/VSO out */
	writel(0x07, priv->io_base + _REG(ENCI_SYNC_MODE));

	/* 0x3 Y, C, and Component Y delay */
	writel(mode->yc_delay, priv->io_base + _REG(ENCI_YC_DELAY));

	/* Timings */
	writel(mode->pixel_start,
			priv->io_base + _REG(ENCI_VFIFO2VD_PIXEL_START));
	writel(mode->pixel_end,
			priv->io_base + _REG(ENCI_VFIFO2VD_PIXEL_END));

	writel(mode->top_field_line_start,
			priv->io_base + _REG(ENCI_VFIFO2VD_LINE_TOP_START));
	writel(mode->top_field_line_end,
			priv->io_base + _REG(ENCI_VFIFO2VD_LINE_TOP_END));

	writel(mode->bottom_field_line_start,
			priv->io_base + _REG(ENCI_VFIFO2VD_LINE_BOT_START));
	writel(mode->bottom_field_line_end,
			priv->io_base + _REG(ENCI_VFIFO2VD_LINE_BOT_END));

	/* Internal Venc, Internal VIU Sync, Internal Vencoder */
	writel(0, priv->io_base + _REG(VENC_SYNC_ROUTE));

	/* UNreset Interlaced TV Encoder */
	writel(0, priv->io_base + _REG(ENCI_DBG_PX_RST));

	/* Enable Vfifo2vd, Y_Cb_Y_Cr select */
	writel(0x4e01, priv->io_base + _REG(ENCI_VFIFO2VD_CTL));

	/* Power UP Dacs */
	writel(0, priv->io_base + _REG(VENC_VDAC_SETTING));

	/* Video Upsampling */
	writel(0x0061, priv->io_base + _REG(VENC_UPSAMPLE_CTRL0));
	writel(0x4061, priv->io_base + _REG(VENC_UPSAMPLE_CTRL1));
	writel(0x5061, priv->io_base + _REG(VENC_UPSAMPLE_CTRL2));

	/* Select Interlace Y DACs */
	writel(0, priv->io_base + _REG(VENC_VDAC_DACSEL0));
	writel(0, priv->io_base + _REG(VENC_VDAC_DACSEL1));
	writel(0, priv->io_base + _REG(VENC_VDAC_DACSEL2));
	writel(0, priv->io_base + _REG(VENC_VDAC_DACSEL3));
	writel(0, priv->io_base + _REG(VENC_VDAC_DACSEL4));
	writel(0, priv->io_base + _REG(VENC_VDAC_DACSEL5));

	/* Select ENCI for VIU */
	meson_vpp_setup_mux(priv, MESON_VIU_VPP_MUX_ENCI);

	/* Enable ENCI FIFO */
	writel(0x2000, priv->io_base + _REG(VENC_VDAC_FIFO_CTRL));

	/* Select ENCI DACs 0, 1, 4, and 5 */
	writel(0x11, priv->io_base + _REG(ENCI_DACSEL_0));
	writel(0x11, priv->io_base + _REG(ENCI_DACSEL_1));

	/* Interlace video enable */
	writel(1, priv->io_base + _REG(ENCI_VIDEO_EN));

	/* Configure Video Saturation / Contrast / Brightness / Hue */
	writel(mode->video_saturation,
			priv->io_base + _REG(ENCI_VIDEO_SAT));
	writel(mode->video_contrast,
			priv->io_base + _REG(ENCI_VIDEO_CONT));
	writel(mode->video_brightness,
			priv->io_base + _REG(ENCI_VIDEO_BRIGHT));
	writel(mode->video_hue,
			priv->io_base + _REG(ENCI_VIDEO_HUE));

	/* Enable DAC0 Filter */
	writel(0x1, priv->io_base + _REG(VENC_VDAC_DAC0_FILT_CTRL0));
	writel(0xfc48, priv->io_base + _REG(VENC_VDAC_DAC0_FILT_CTRL1));

	/* 0 in Macrovision register 0 */
	writel(0, priv->io_base + _REG(ENCI_MACV_N0));

	/* Analog Synchronization and color burst value adjust */
	writel(mode->analog_sync_adj,
			priv->io_base + _REG(ENCI_SYNC_ADJ));

	/* enable VDAC */
	writel_bits(BIT(5), 0, priv->io_base + _REG(VENC_VDAC_DACSEL0));

	if (meson_vpu_is_compatible(priv, VPU_COMPATIBLE_GXBB))
		hhi_write(HHI_VDAC_CNTL0, 1);
	else if (meson_vpu_is_compatible(priv, VPU_COMPATIBLE_GXL) ||
		 meson_vpu_is_compatible(priv, VPU_COMPATIBLE_GXM))
		hhi_write(HHI_VDAC_CNTL0, 0xf0001);

	hhi_write(HHI_VDAC_CNTL1, 0);
}

void meson_vpu_setup_venc(struct udevice *dev,
			  const struct display_timing *mode, bool is_cvbs)
{
	struct meson_vpu_priv *priv = dev_get_priv(dev);

	if (is_cvbs)
		return meson_venci_cvbs_mode_set(priv, &meson_cvbs_enci_pal);

}
