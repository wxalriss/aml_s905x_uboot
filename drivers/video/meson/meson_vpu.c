// SPDX-License-Identifier: GPL-2.0
/*
 * Amlogic Meson Video Processing Unit driver
 *
 * Copyright (c) 2018 BayLibre, SAS.
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 */

#include "meson_vpu.h"
#include <power-domain.h>
#include <efi_loader.h>
#include <dm/device-internal.h>
#include <dm/uclass-internal.h>
#include "meson_registers.h"

static int meson_vpu_setup_mode(struct udevice *dev)
{
	struct video_uc_platdata *uc_plat = dev_get_uclass_platdata(dev);
	struct video_priv *uc_priv = dev_get_uclass_priv(dev);
	struct display_timing timing;
	bool is_cvbs = false;

	/* CVBS has a fixed 720x480i (NTSC) and 720x576i (PAL) */
	is_cvbs = true;
	timing.flags = DISPLAY_FLAGS_INTERLACED;
	uc_priv->xsize = 720;
	uc_priv->ysize = 576;

	uc_priv->bpix = VPU_MAX_LOG2_BPP;
	meson_vpu_setup_plane(dev, timing.flags & DISPLAY_FLAGS_INTERLACED);
	meson_vpu_setup_venc(dev, &timing, is_cvbs);
	meson_vpu_setup_vclk(dev, &timing, is_cvbs);

#ifdef CONFIG_EFI_LOADER
	efi_add_memory_map(uc_plat->base,
			   ALIGN(timing.hactive.typ * timing.vactive.typ *
			   (1 << VPU_MAX_LOG2_BPP) / 8, EFI_PAGE_SIZE)
			   				>> EFI_PAGE_SHIFT,
			   EFI_RESERVED_MEMORY_TYPE, false);
#endif
	video_set_flush_dcache(dev, 1);

	return 0;
}

static const struct udevice_id meson_vpu_ids[] = {                          
	{ .compatible = "amlogic,meson-gxbb-vpu", .data = VPU_COMPATIBLE_GXBB },
	{ .compatible = "amlogic,meson-gxl-vpu", .data = VPU_COMPATIBLE_GXL },
	{ .compatible = "amlogic,meson-gxm-vpu", .data = VPU_COMPATIBLE_GXM },
	{ }                                                                     
};

static int meson_vpu_probe(struct udevice *dev)
{
	struct meson_vpu_priv *priv = dev_get_priv(dev);
	struct power_domain pd;
	int ret;

	/* Before relocation we don't need to do anything */
	if (!(gd->flags & GD_FLG_RELOC))
		return 0;

	priv->dev = dev;

	priv->io_base = dev_remap_addr_index(dev, 0);
	if (!priv->io_base)
		return -EINVAL;

	priv->hhi_base = dev_remap_addr_index(dev, 1);
	if (!priv->hhi_base)
		return -EINVAL;

	priv->dmc_base = dev_remap_addr_index(dev, 2);
	if (!priv->dmc_base)
		return -EINVAL;

	ret = power_domain_get(dev, &pd);
	if (ret)
		return ret;

	ret = power_domain_on(&pd);
	if (ret)
		return ret;

	meson_vpu_init(dev);

	return meson_vpu_setup_mode(dev);
}

static int meson_vpu_bind(struct udevice *dev)
{
	struct video_uc_platdata *plat = dev_get_uclass_platdata(dev);

	plat->size = VPU_MAX_WIDTH * VPU_MAX_HEIGHT *
		(1 << VPU_MAX_LOG2_BPP) / 8;

	return 0;
}

U_BOOT_DRIVER(meson_vpu) = {
	.name = "meson_vpu",
	.id = UCLASS_VIDEO,
	.of_match = meson_vpu_ids,
	.probe = meson_vpu_probe,
	.bind = meson_vpu_bind,
	.priv_auto_alloc_size = sizeof(struct meson_vpu_priv),
	.flags  = DM_FLAG_PRE_RELOC,
};
