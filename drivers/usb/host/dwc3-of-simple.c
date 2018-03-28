/*
 * dwc3-of-simple.c - OF glue layer for simple integrations
 *
 * Copyright (c) 2015 Texas Instruments Incorporated - http://www.ti.com
 *
 * Author: Felipe Balbi <balbi@ti.com>
 *
 * Copyright (C) 2018 BayLibre, SAS
 * Author: Neil Armstrong <narmstron@baylibre.com>
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#include <common.h>
#include <dm.h>
#include <fdtdec.h>
#include <reset.h>
#include <clk.h>

DECLARE_GLOBAL_DATA_PTR;

struct dwc3_of_simple {
#if CONFIG_IS_ENABLED(CLK)
	struct clk		*clks;
	int			num_clocks;
#endif
#if CONFIG_IS_ENABLED(DM_RESET)
	struct reset_ctl	*resets;
	int			num_resets;
#endif
};

#if CONFIG_IS_ENABLED(DM_RESET)
static int dwc3_of_simple_reset_init(struct udevice *dev,
				     struct dwc3_of_simple *simple,
				     int count)
{
	int i, ret, err;

	if (!count)
		return 0;

	simple->resets = devm_kcalloc(dev, count, sizeof(struct reset_ctl),
				      GFP_KERNEL);
	if (!simple->resets)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		ret = reset_get_by_index(dev, i, &simple->resets[i]);
		if (ret < 0)
			break;

		ret = reset_request(&simple->resets[i]);
		if (ret) {
			pr_err("failed to request reset line %d\n", i);
			reset_free(&simple->resets[i]);
			goto reset_err;
		}

		
		++simple->num_resets;
	}

	for (i = 0; i < simple->num_resets; i++) {
		ret = reset_deassert(&simple->resets[i]);
		if (ret && ret != -ENOSYS && ret != -ENOTSUPP) {
			pr_err("failed to deassert reset line %d\n", i);
			goto reset_err;
		}
	}

	return 0;

reset_err:
	err = reset_release_all(simple->resets, simple->num_resets);
	if (err)
		pr_err("failed to disable all clocks\n");
	
	return ret;
}
#endif

#if CONFIG_IS_ENABLED(CLK)
static int dwc3_of_simple_clk_init(struct udevice *dev,
				   struct dwc3_of_simple *simple,
				   int count)
{
	int i, ret, err;

	if (!count)
		return 0;

	simple->clks = devm_kcalloc(dev, count, sizeof(struct clk),
				    GFP_KERNEL);
	if (!simple->clks)
		return -ENOMEM;

	for (i = 0; i < count; i++) {
		ret = clk_get_by_index(dev, i, &simple->clks[i]);
		if (ret < 0)
			break;

		ret = clk_enable(&simple->clks[i]);
		if (ret && ret != -ENOSYS && ret != -ENOTSUPP) {
			pr_err("failed to enable clock %d\n", i);
			clk_free(&simple->clks[i]);
			goto clk_err;
		}

		++simple->num_clocks;
	}

	return 0;

clk_err:
	err = clk_release_all(simple->clks, simple->num_clocks);
	if (err)
		pr_err("failed to disable all clocks\n");
	
	return ret;
}
#endif

static int dwc3_of_simple_probe(struct udevice *dev)
{
#if CONFIG_IS_ENABLED(DM_RESET) || CONFIG_IS_ENABLED(CLK)
	struct dwc3_of_simple *simple = dev_get_platdata(dev);
	int ret;
#endif

#if CONFIG_IS_ENABLED(CLK)
	ret = dwc3_of_simple_clk_init(dev, simple,
		dev_count_phandle_with_args(dev, "clocks", "#clock-cells"));
	if (ret)
		return ret;
#endif

#if CONFIG_IS_ENABLED(DM_RESET)
	ret = dwc3_of_simple_reset_init(dev, simple,
		dev_count_phandle_with_args(dev, "resets", "#reset-cells"));
	if (ret)
		return ret;
#endif

	return 0;
}

static int dwc3_of_simple_remove(struct udevice *dev)
{
#if CONFIG_IS_ENABLED(DM_RESET) || CONFIG_IS_ENABLED(CLK)
	struct dwc3_of_simple *simple = dev_get_platdata(dev);
	int i;
#endif

#if CONFIG_IS_ENABLED(DM_RESET)
	for (i = 0; i < simple->num_resets; i++)
		reset_assert(&simple->resets[i]);

	if (simple->num_resets)
		reset_release_all(simple->resets, simple->num_resets);
#endif

#if CONFIG_IS_ENABLED(CLK)
	for (i = 0; i < simple->num_clocks; i++)
		clk_disable(&simple->clks[i]);

	if (simple->num_clocks)
		clk_release_all(simple->clks, simple->num_clocks);
#endif

	return dm_scan_fdt_dev(dev);
}

static const struct udevice_id dwc3_of_simple_ids[] = {
	{ .compatible = "amlogic,meson-gxl-dwc3" },
	{ }
};

U_BOOT_DRIVER(dwc3_of_simple) = {
	.name = "dwc3-of-simple",
	.id = UCLASS_SIMPLE_BUS,
	.of_match = dwc3_of_simple_ids,
	.probe = dwc3_of_simple_probe,
	.remove = dwc3_of_simple_remove,
	.platdata_auto_alloc_size = sizeof(struct dwc3_of_simple),
	.flags = DM_FLAG_ALLOC_PRIV_DMA,
};
