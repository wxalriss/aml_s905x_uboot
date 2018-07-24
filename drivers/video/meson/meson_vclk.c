// SPDX-License-Identifier: GPL-2.0
/*
 * Amlogic Meson Video Processing Unit driver
 *
 * Copyright (c) 2018 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 */

#include <edid.h>
#include "meson_vpu.h"
#include <linux/iopoll.h>
#include <linux/math64.h>

enum {
	MESON_VCLK_TARGET_CVBS = 0,
	MESON_VCLK_TARGET_HDMI = 1,
	MESON_VCLK_TARGET_DMT = 2,
};

/* HHI Registers */
#define HHI_VID_PLL_CLK_DIV	0x1a0 /* 0x68 offset in data sheet */
#define VID_PLL_EN		BIT(19)
#define VID_PLL_BYPASS		BIT(18)
#define VID_PLL_PRESET		BIT(15)
#define HHI_VIID_CLK_DIV	0x128 /* 0x4a offset in data sheet */
#define VCLK2_DIV_MASK		0xff
#define VCLK2_DIV_EN		BIT(16)
#define VCLK2_DIV_RESET		BIT(17)
#define CTS_VDAC_SEL_MASK	(0xf << 28)
#define CTS_VDAC_SEL_SHIFT	28
#define HHI_VIID_CLK_CNTL	0x12c /* 0x4b offset in data sheet */
#define VCLK2_EN		BIT(19)
#define VCLK2_SEL_MASK		(0x7 << 16)
#define VCLK2_SEL_SHIFT		16
#define VCLK2_SOFT_RESET	BIT(15)
#define VCLK2_DIV1_EN		BIT(0)
#define HHI_VID_CLK_DIV		0x164 /* 0x59 offset in data sheet */
#define VCLK_DIV_MASK		0xff
#define VCLK_DIV_EN		BIT(16)
#define VCLK_DIV_RESET		BIT(17)
#define CTS_ENCP_SEL_MASK	(0xf << 24)
#define CTS_ENCP_SEL_SHIFT	24
#define CTS_ENCI_SEL_MASK	(0xf << 28)
#define CTS_ENCI_SEL_SHIFT	28
#define HHI_VID_CLK_CNTL	0x17c /* 0x5f offset in data sheet */
#define VCLK_EN			BIT(19)
#define VCLK_SEL_MASK		(0x7 << 16)
#define VCLK_SEL_SHIFT		16
#define VCLK_SOFT_RESET		BIT(15)
#define VCLK_DIV1_EN		BIT(0)
#define VCLK_DIV2_EN		BIT(1)
#define VCLK_DIV4_EN		BIT(2)
#define VCLK_DIV6_EN		BIT(3)
#define VCLK_DIV12_EN		BIT(4)
#define HHI_VID_CLK_CNTL2	0x194 /* 0x65 offset in data sheet */
#define CTS_ENCI_EN		BIT(0)
#define CTS_ENCP_EN		BIT(2)
#define CTS_VDAC_EN		BIT(4)
#define HDMI_TX_PIXEL_EN	BIT(5)
#define HHI_HDMI_CLK_CNTL	0x1cc /* 0x73 offset in data sheet */
#define HDMI_TX_PIXEL_SEL_MASK	(0xf << 16)
#define HDMI_TX_PIXEL_SEL_SHIFT	16
#define CTS_HDMI_SYS_SEL_MASK	(0x7 << 9)
#define CTS_HDMI_SYS_DIV_MASK	(0x7f)
#define CTS_HDMI_SYS_EN		BIT(8)

#define HHI_HDMI_PLL_CNTL	0x320 /* 0xc8 offset in data sheet */
#define HHI_HDMI_PLL_CNTL2	0x324 /* 0xc9 offset in data sheet */
#define HHI_HDMI_PLL_CNTL3	0x328 /* 0xca offset in data sheet */
#define HHI_HDMI_PLL_CNTL4	0x32C /* 0xcb offset in data sheet */
#define HHI_HDMI_PLL_CNTL5	0x330 /* 0xcc offset in data sheet */
#define HHI_HDMI_PLL_CNTL6	0x334 /* 0xcd offset in data sheet */

#define HDMI_PLL_RESET		BIT(28)
#define HDMI_PLL_LOCK		BIT(31)


/* VID PLL Dividers */
enum {
	VID_PLL_DIV_1 = 0,
	VID_PLL_DIV_2,
	VID_PLL_DIV_2p5,
	VID_PLL_DIV_3,
	VID_PLL_DIV_3p5,
	VID_PLL_DIV_3p75,
	VID_PLL_DIV_4,
	VID_PLL_DIV_5,
	VID_PLL_DIV_6,
	VID_PLL_DIV_6p25,
	VID_PLL_DIV_7,
	VID_PLL_DIV_7p5,
	VID_PLL_DIV_12,
	VID_PLL_DIV_14,
	VID_PLL_DIV_15,
};

void meson_vid_pll_set(struct meson_vpu_priv *priv, unsigned int div)
{
	unsigned int shift_val = 0;
	unsigned int shift_sel = 0;

	/* Disable vid_pll output clock */
	hhi_update_bits(HHI_VID_PLL_CLK_DIV, VID_PLL_EN, 0);
	hhi_update_bits(HHI_VID_PLL_CLK_DIV, VID_PLL_PRESET, 0);

	switch (div) {
	case VID_PLL_DIV_2:
		shift_val = 0x0aaa;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_2p5:
		shift_val = 0x5294;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_3:
		shift_val = 0x0db6;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_3p5:
		shift_val = 0x36cc;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_3p75:
		shift_val = 0x6666;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_4:
		shift_val = 0x0ccc;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_5:
		shift_val = 0x739c;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_6:
		shift_val = 0x0e38;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_6p25:
		shift_val = 0x0000;
		shift_sel = 3;
		break;
	case VID_PLL_DIV_7:
		shift_val = 0x3c78;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_7p5:
		shift_val = 0x78f0;
		shift_sel = 2;
		break;
	case VID_PLL_DIV_12:
		shift_val = 0x0fc0;
		shift_sel = 0;
		break;
	case VID_PLL_DIV_14:
		shift_val = 0x3f80;
		shift_sel = 1;
		break;
	case VID_PLL_DIV_15:
		shift_val = 0x7f80;
		shift_sel = 2;
		break;
	}

	if (div == VID_PLL_DIV_1)
		/* Enable vid_pll bypass to HDMI pll */
		hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				   VID_PLL_BYPASS, VID_PLL_BYPASS);
	else {
		/* Disable Bypass */
		hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				   VID_PLL_BYPASS, 0);
		/* Clear sel */
		hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				   3 << 16, 0);
		hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				   VID_PLL_PRESET, 0);
		hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				   0x7fff, 0);

		/* Setup sel and val */
		hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				   3 << 16, shift_sel << 16);
		hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				   VID_PLL_PRESET, VID_PLL_PRESET);
		hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				   0x7fff, shift_val);

		hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				   VID_PLL_PRESET, 0);
	}

	/* Enable the vid_pll output clock */
	hhi_update_bits(HHI_VID_PLL_CLK_DIV,
				VID_PLL_EN, VID_PLL_EN);
}

/*
 * Setup VCLK2 for 27MHz, and enable clocks for ENCI and VDAC
 *
 * TOFIX: Refactor into table to also handle HDMI frequency and paths
 */
static void meson_venci_cvbs_clock_config(struct meson_vpu_priv *priv)
{
	unsigned int val;

	debug("%s:%d\n", __func__, __LINE__);

	/* Setup PLL to output 1.485GHz */
	if (meson_vpu_is_compatible(priv, VPU_COMPATIBLE_GXBB)) {
		hhi_write(HHI_HDMI_PLL_CNTL, 0x5800023d);
		hhi_write(HHI_HDMI_PLL_CNTL2, 0x00404e00);
		hhi_write(HHI_HDMI_PLL_CNTL3, 0x0d5c5091);
		hhi_write(HHI_HDMI_PLL_CNTL4, 0x801da72c);
		hhi_write(HHI_HDMI_PLL_CNTL5, 0x71486980);
		hhi_write(HHI_HDMI_PLL_CNTL6, 0x00000e55);
		hhi_write(HHI_HDMI_PLL_CNTL, 0x4800023d);
	} else if (meson_vpu_is_compatible(priv, VPU_COMPATIBLE_GXM) ||
		   meson_vpu_is_compatible(priv, VPU_COMPATIBLE_GXL)) {
		hhi_write(HHI_HDMI_PLL_CNTL, 0x4000027b);
		hhi_write(HHI_HDMI_PLL_CNTL2, 0x800cb300);
		hhi_write(HHI_HDMI_PLL_CNTL3, 0xa6212844);
		hhi_write(HHI_HDMI_PLL_CNTL4, 0x0c4d000c);
		hhi_write(HHI_HDMI_PLL_CNTL5, 0x001fa729);
		hhi_write(HHI_HDMI_PLL_CNTL6, 0x01a31500);

		/* Reset PLL */
		hhi_update_bits(HHI_HDMI_PLL_CNTL,
					HDMI_PLL_RESET, HDMI_PLL_RESET);
		hhi_update_bits(HHI_HDMI_PLL_CNTL,
					HDMI_PLL_RESET, 0);
	}

	debug("%s:%d\n", __func__, __LINE__);

	/* Poll for lock bit */
	readl_poll_timeout(priv->hhi_base + HHI_HDMI_PLL_CNTL, val,
			   (val & HDMI_PLL_LOCK), 10);

	/* Disable VCLK2 */
	hhi_update_bits(HHI_VIID_CLK_CNTL, VCLK2_EN, 0);

	/* Setup vid_pll to /1 */
	meson_vid_pll_set(priv, VID_PLL_DIV_1);

	/* Setup the VCLK2 divider value to achieve 27MHz */
	hhi_update_bits(HHI_VIID_CLK_DIV,
				VCLK2_DIV_MASK, (55 - 1));

	/* select vid_pll for vclk2 */
	hhi_update_bits(HHI_VIID_CLK_CNTL,
				VCLK2_SEL_MASK, (4 << VCLK2_SEL_SHIFT));
	/* enable vclk2 gate */
	hhi_update_bits(HHI_VIID_CLK_CNTL, VCLK2_EN, VCLK2_EN);

	/* select vclk_div1 for enci */
	hhi_update_bits(HHI_VID_CLK_DIV,
				CTS_ENCI_SEL_MASK, (8 << CTS_ENCI_SEL_SHIFT));
	/* select vclk_div1 for vdac */
	hhi_update_bits(HHI_VIID_CLK_DIV,
				CTS_VDAC_SEL_MASK, (8 << CTS_VDAC_SEL_SHIFT));

	/* release vclk2_div_reset and enable vclk2_div */
	hhi_update_bits(HHI_VIID_CLK_DIV,
				VCLK2_DIV_EN | VCLK2_DIV_RESET, VCLK2_DIV_EN);

	/* enable vclk2_div1 gate */
	hhi_update_bits(HHI_VIID_CLK_CNTL,
				VCLK2_DIV1_EN, VCLK2_DIV1_EN);

	/* reset vclk2 */
	hhi_update_bits(HHI_VIID_CLK_CNTL,
				VCLK2_SOFT_RESET, VCLK2_SOFT_RESET);
	hhi_update_bits(HHI_VIID_CLK_CNTL,
				VCLK2_SOFT_RESET, 0);

	/* enable enci_clk */
	hhi_update_bits(HHI_VID_CLK_CNTL2,
				CTS_ENCI_EN, CTS_ENCI_EN);
	/* enable vdac_clk */
	hhi_update_bits(HHI_VID_CLK_CNTL2,
				CTS_VDAC_EN, CTS_VDAC_EN);

	debug("%s:%d\n", __func__, __LINE__);
}

static void meson_vclk_setup(struct meson_vpu_priv *priv, unsigned int target,
			     unsigned int vclk_freq, unsigned int venc_freq,
			     unsigned int dac_freq, bool hdmi_use_enci)
{
	if (target == MESON_VCLK_TARGET_CVBS)
		return meson_venci_cvbs_clock_config(priv);
}

void meson_vpu_setup_vclk(struct udevice *dev,
			  const struct display_timing *mode, bool is_cvbs)
{
	struct meson_vpu_priv *priv = dev_get_priv(dev);

	if (is_cvbs)
		return meson_vclk_setup(priv, MESON_VCLK_TARGET_CVBS,
					0, 0, 0, false);
}
