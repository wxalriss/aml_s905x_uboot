/*
 * Copyright (C) 2016 BayLibre, SAS
 * Author: Neil Armstrong <narmstrong@baylibre.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef COMMON_DART_MX6_H
#define COMMON_DART_MX6_H

int dart_mx6_dram_init(void);

int dart_mx6_eth_init(bd_t *bis);

void dart_mx6_early_init_f(void);

int dart_mx6_board_init(void);

int dart_mx6_late_init(void);

#ifdef CONFIG_SPL_BUILD
int dart_mx6_eeprom_dram_init(void);

void dart_mx6_init_f(ulong dummy);
#endif

#endif
