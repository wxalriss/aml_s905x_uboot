/*
 * Copyright (C) 2016 BayLibre, SAS
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 *
 * Copyright (C) 2016 Variscite Ltd. All Rights Reserved.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <asm/arch/clock.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux.h>
#include <asm/arch/mx6-pins.h>
#include <linux/errno.h>
#include <asm/gpio.h>
#include <asm/mach-imx/iomux-v3.h>
#include <asm/mach-imx/mxc_i2c.h>
#include <mmc.h>
#include <fsl_esdhc.h>
#include <malloc.h>
#include <miiphy.h>
#include <netdev.h>
#include <asm/arch/crm_regs.h>
#include <asm/io.h>
#include <asm/arch/sys_proto.h>
#include <spl.h>
#include <usb.h>
#include <usb/ehci-ci.h>
#include <power/pmic.h>
#include <power/pfuze100_pmic.h>
#include "dart-mx6.h"

#define RAM_SIZE_ADDR	((CONFIG_SPL_TEXT_BASE) + (CONFIG_SPL_MAX_SIZE))

#define UART_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm |			\
	PAD_CTL_SRE_FAST  | PAD_CTL_HYS)

static iomux_v3_cfg_t const uart1_pads[] = {
	IOMUX_PADS(PAD_CSI0_DAT10__UART1_TX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL)),
	IOMUX_PADS(PAD_CSI0_DAT11__UART1_RX_DATA | MUX_PAD_CTRL(UART_PAD_CTRL)),
};

#ifdef CONFIG_SYS_I2C_MXC

#define I2C_PAD_CTRL  (PAD_CTL_PUS_100K_UP |			\
	PAD_CTL_SPEED_MED | PAD_CTL_DSE_40ohm | PAD_CTL_HYS |	\
	PAD_CTL_ODE | PAD_CTL_SRE_FAST)

I2C_PADS(i2c2_pads,
	PAD_KEY_COL3__I2C2_SCL | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_KEY_COL3__GPIO4_IO12 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(4, 12),
	PAD_KEY_ROW3__I2C2_SDA | MUX_PAD_CTRL(I2C_PAD_CTRL),
	PAD_KEY_ROW3__GPIO4_IO13 | MUX_PAD_CTRL(I2C_PAD_CTRL),
	IMX_GPIO_NR(4, 13));
#endif

int dart_mx6_dram_init(void)
{
	u32 *p_ram_size = (u32 *)RAM_SIZE_ADDR;

	gd->ram_size = *p_ram_size  * 1024 * 1024;

	return 0;
}

int dart_mx6_eth_init(bd_t *bis)
{
	struct iomuxc *const iomuxc_regs = (struct iomuxc *)IOMUXC_BASE_ADDR;

	int ret = enable_fec_anatop_clock(0, ENET_25MHZ);
	if (ret)
		return ret;

	/* set gpr1[ENET_CLK_SEL] */
	setbits_le32(&iomuxc_regs->gpr[1], IOMUXC_GPR1_ENET_CLK_SEL_MASK);

	return 0;
}

void dart_mx6_early_init_f(void)
{
	SETUP_IOMUX_PADS(uart1_pads);

#ifdef CONFIG_SYS_I2C_MXC
	setup_i2c(1, CONFIG_SYS_I2C_SPEED, 0x7f, I2C_PADS_INFO(i2c2_pads));
#endif
}

int dart_mx6_board_init(void)
{
	int ret = 0;

	/* address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

	return ret;
}

struct pmic_write_values {
	u32 reg;
	u32 mask;
	u32 writeval;
};

static int pmic_write_vals(struct pmic *dev,
			   const struct pmic_write_values * const arr,
			   int arr_size)
{
	unsigned int i, val;
	int ret;

	for (i = 0; i < arr_size; ++i) {
		ret = pmic_reg_read(dev, arr[i].reg, &val);
		if (ret)
			return ret;
		val &= ~(arr[i].mask);
		val |= arr[i].writeval;
		ret = pmic_reg_write(dev, arr[i].reg, val);
		if (ret)
			return ret;
	}

	return 0;
}

static const struct pmic_write_values pmic_arr[] = {
	/* Set SW1AB standby volage to 0.9V */
	{PFUZE100_SW1ABSTBY, SW1x_STBY_MASK, SW1x_0_900V},

	/* Set SW1AB off volage to 0.9V */
	{PFUZE100_SW1ABOFF, SW1x_OFF_MASK, SW1x_0_900V},

	/* Set SW1C standby voltage to 0.9V */
	{PFUZE100_SW1CSTBY, SW1x_STBY_MASK, SW1x_0_900V},

	/* Set SW1C off volage to 0.9V */
	{PFUZE100_SW1COFF, SW1x_OFF_MASK, SW1x_0_900V},

	/* Set SW2 to 3.3V */
	{PFUZE100_SW2VOL, SWx_NORMAL_MASK, SWx_HR_3_300V},

	/* Set SW2 standby voltage to 3.2V */
	{PFUZE100_SW2STBY, SWx_STBY_MASK, SWx_HR_3_200V},

	/* Set SW2 off voltage to 3.2V */
	{PFUZE100_SW2OFF, SWx_OFF_MASK, SWx_HR_3_200V},

	/* Set SW1AB/VDDARM step ramp up time 2us */
	{PFUZE100_SW1ABCONF, SW1xCONF_DVSSPEED_MASK, SW1xCONF_DVSSPEED_2US},

	/* Set SW1AB, SW1C, SW2 normal mode to PWM, and standby mode to PFM */
	{PFUZE100_SW1ABMODE, SW_MODE_MASK, PWM_PFM},
	{PFUZE100_SW1CMODE, SW_MODE_MASK, PWM_PFM},
	{PFUZE100_SW2MODE, SW_MODE_MASK, PWM_PFM},

	/* Set VGEN6 to 3.3V */
	{PFUZE100_VGEN6VOL, LDO_VOL_MASK, LDOB_3_30V},

	/* Set SW1C/VDDSOC step ramp up time from 16us to 4us/25mV */
	{PFUZE100_SW1CCONF, SW1xCONF_DVSSPEED_MASK, SW1xCONF_DVSSPEED_4US},
};

int power_init_board(void)
{
	struct pmic *dev;
	unsigned int reg;
	int ret;

	ret = power_pfuze100_init(I2C_PMIC);
	if (ret)
		return ret;

	dev = pmic_get("PFUZE100");
	ret = pmic_probe(dev);
	if (ret)
		return ret;

	ret = pmic_reg_read(dev, PFUZE100_DEVICEID, &reg);
	if (ret)
		return ret;

	printf("PMIC:  PFUZE100 ID=0x%02x\n", reg);

	return pmic_write_vals(dev, pmic_arr, ARRAY_SIZE(pmic_arr));
}

int dart_mx6_late_init(void)
{
#ifdef CONFIG_ENV_VARS_UBOOT_RUNTIME_CONFIG
	if (is_mx6dq())
		env_set("board_rev", "MX6Q");
	else
		env_set("board_rev", "MX6DL");
#endif

	return 0;
}

#ifdef CONFIG_SPL_BUILD
#include <asm/arch/mx6-ddr.h>

#define PER_VCC_EN_PAD_CTRL  (PAD_CTL_PKE | PAD_CTL_PUE |	\
	PAD_CTL_PUS_100K_UP | PAD_CTL_SPEED_MED |	\
	PAD_CTL_DSE_40ohm   | PAD_CTL_HYS)

static iomux_v3_cfg_t const per_vcc_en_pads[] = {
	IOMUX_PADS(PAD_EIM_D31__GPIO3_IO31 | MUX_PAD_CTRL(PER_VCC_EN_PAD_CTRL)),
};

#define PER_VCC_EN	IMX_GPIO_NR(3, 31)

static iomux_v3_cfg_t const audio_reset_pads[] = {
	IOMUX_PADS(PAD_GPIO_19__GPIO4_IO05 | MUX_PAD_CTRL(NO_PAD_CTRL)),
};

#define AUDIO_RESET	IMX_GPIO_NR(4, 5)

static void ccgr_init(void)
{
	struct mxc_ccm_reg *ccm = (struct mxc_ccm_reg *)CCM_BASE_ADDR;

	writel(0x00C03F3F, &ccm->CCGR0);
	writel(0x0030FC03, &ccm->CCGR1);
	writel(0x0FFFC000, &ccm->CCGR2);
	writel(0x3FF00000, &ccm->CCGR3);
	writel(0x00FFF300, &ccm->CCGR4);
	writel(0x0F0000C3, &ccm->CCGR5);
	writel(0x000003FF, &ccm->CCGR6);
}

/*
 * Bugfix: Fix Freescale wrong processor documentation.
 */
static void spl_mx6qd_dram_setup_iomux_check_reset(void)
{
	volatile struct mx6dq_iomux_ddr_regs *mx6dq_ddr_iomux;

	mx6dq_ddr_iomux = (struct mx6dq_iomux_ddr_regs *) MX6DQ_IOM_DDR_BASE;

	if (mx6dq_ddr_iomux->dram_reset == (u32)0x000C0030)
		mx6dq_ddr_iomux->dram_reset = (u32)0x00000030;
}

static void spl_dram_init(void)
{
	u32 *p_ram_size = (u32 *)RAM_SIZE_ADDR;
	*p_ram_size = dart_mx6_eeprom_dram_init();

	spl_mx6qd_dram_setup_iomux_check_reset();
}

void dart_mx6_init_f(ulong dummy)
{
	/* setup AIPS and disable watchdog */
	arch_cpu_init();

	ccgr_init();
	gpr_init();

	/* Enable peripherals power */
	SETUP_IOMUX_PADS(per_vcc_en_pads);
	gpio_direction_output(PER_VCC_EN, 1);

	/* Reset Audio Codec */
	SETUP_IOMUX_PADS(audio_reset_pads);
	gpio_direction_output(AUDIO_RESET, 0);

	/* iomux and setup of i2c */
	board_early_init_f();

	/* setup GP timer */
	timer_init();

	mdelay(300);

	/* UNReset Audio Codec */
	gpio_direction_output(AUDIO_RESET, 1);

	/* UART clocks enabled and gd valid - init serial console */
	preloader_console_init();

	/* DDR initialization */
	spl_dram_init();

	/* Clear the BSS. */
	memset(__bss_start, 0, __bss_end - __bss_start);

	/* load/boot image from boot device */
	board_init_r(NULL, 0);
}
#endif
