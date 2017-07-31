/*
 * Configuration for LibreTech CC
 * (C) Copyright 2016 Beniamino Galvani <b.galvani@gmail.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#define CONFIG_MISC_INIT_R

/* Serial setup */
#define CONFIG_CONS_INDEX		0

#define MESON_FDTFILE_SETTING "fdtfile=amlogic/meson-gxl-s905x-libretech-cc.dtb\0"

#include <configs/meson-gxbb-common.h>

#endif /* __CONFIG_H */
