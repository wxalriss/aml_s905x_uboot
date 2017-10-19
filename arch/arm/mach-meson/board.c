/*
 * (C) Copyright 2016 Beniamino Galvani <b.galvani@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <libfdt.h>
#include <linux/err.h>
#include <asm/arch/gxbb.h>
#include <asm/arch/sm.h>
#include <asm/armv8/mmu.h>
#include <asm/unaligned.h>
#include <linux/sizes.h>
#include <asm/io.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	const fdt64_t *val;
	int offset;
	int len;

	offset = fdt_path_offset(gd->fdt_blob, "/memory");
	if (offset < 0)
		return -EINVAL;

	val = fdt_getprop(gd->fdt_blob, offset, "reg", &len);
	if (len < sizeof(*val) * 2)
		return -EINVAL;

	/* Use unaligned access since cache is still disabled */
	gd->ram_size = get_unaligned_be64(&val[1]);

	return 0;
}

phys_size_t get_effective_memsize(void)
{
	/* Size is reported in MiB, convert it in bytes */
	return ((in_le32(GXBB_AO_SEC_GP_CFG0) & GXBB_AO_MEM_SIZE_MASK)
			>> GXBB_AO_MEM_SIZE_SHIFT) * SZ_1M;
}

int dram_init_banksize(void)
{
	u32 bl31_size, bl31_start;
	u32 bl32_size, bl32_start;
	/* Start after first 16MiB reserved zone */
	unsigned int next = 0;
	u32 last = 0x1000000;
	u32 reg;

	/*
	 * Get ARM Trusted Firmware reserved memory zones in :
	 * - AO_SEC_GP_CFG3: bl32 & bl31 size in KiB, can be 0
	 * - AO_SEC_GP_CFG5: bl31 physical start address, can be NULL
	 * - AO_SEC_GP_CFG4: bl32 physical start address, can be NULL
	 */

	reg = in_le32(GXBB_AO_SEC_GP_CFG3);

	bl31_size = ((reg & GXBB_AO_BL31_RSVMEM_SIZE_MASK)
			>> GXBB_AO_BL31_RSVMEM_SIZE_SHIFT) * SZ_1K;
	bl32_size = (reg & GXBB_AO_BL32_RSVMEM_SIZE_MASK) * SZ_1K;

	bl31_start = in_le32(GXBB_AO_SEC_GP_CFG5);
	bl32_start = in_le32(GXBB_AO_SEC_GP_CFG4);

	if (bl31_size && bl31_start && bl32_size && bl32_start) {
		/* Reserve memory for ARM Trusted Firmware (BL31 && BL32) */
		gd->bd->bi_dram[next].start = last;
		if (bl31_start > bl32_start)
			gd->bd->bi_dram[next].size = bl32_start - last;
		else
			gd->bd->bi_dram[next].size = bl31_start - last;

		last = gd->bd->bi_dram[next].start +
			gd->bd->bi_dram[next].size;

		if (bl31_start > bl32_start)
			last += bl32_size;
		else
			last += bl31_size;
		next++;

		gd->bd->bi_dram[next].start = last;
		if (bl31_start > bl32_start)
			gd->bd->bi_dram[next].size = bl31_start - last;
		else
			gd->bd->bi_dram[next].size = bl32_start - last;

		last = gd->bd->bi_dram[next].start +
			gd->bd->bi_dram[next].size;

		if (bl31_start > bl32_start)
			last += bl31_size;
		else
			last += bl32_size;
		next++;
	} else if ((bl31_size && bl31_start) || (bl32_size && bl32_start)) {
		/* Reserve memory for ARM Trusted Firmware (BL31 || BL32) */
		gd->bd->bi_dram[next].start = last;
		if (bl31_start && bl31_size)
			gd->bd->bi_dram[next].size = bl31_start - last;
		else
			gd->bd->bi_dram[next].size = bl32_start - last;

		last = gd->bd->bi_dram[next].start +
			gd->bd->bi_dram[next].size;

		if (bl31_start && bl31_size)
			last += bl31_size;
		else
			last += bl32_size;

		next++;
	}

	/* Add remaining memory */
	gd->bd->bi_dram[next].start = last;
	gd->bd->bi_dram[next].size = get_effective_memsize() - last;
	next++;

	/* Reset unused banks */
	for ( ; next < CONFIG_NR_DRAM_BANKS ; ++next) {
		gd->bd->bi_dram[next].start = 0;
		gd->bd->bi_dram[next].size = 0;
	}

	return 0;
}

void reset_cpu(ulong addr)
{
	psci_system_reset();
}

static struct mm_region gxbb_mem_map[] = {
	{
		.virt = 0x0UL,
		.phys = 0x0UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_NORMAL) |
			 PTE_BLOCK_INNER_SHARE
	}, {
		.virt = 0x80000000UL,
		.phys = 0x80000000UL,
		.size = 0x80000000UL,
		.attrs = PTE_BLOCK_MEMTYPE(MT_DEVICE_NGNRNE) |
			 PTE_BLOCK_NON_SHARE |
			 PTE_BLOCK_PXN | PTE_BLOCK_UXN
	}, {
		/* List terminator */
		0,
	}
};

struct mm_region *mem_map = gxbb_mem_map;
