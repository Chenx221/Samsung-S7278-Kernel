/*
 * Copyright (C) 2011, SAMSUNG Corporation.
 * Author: YongTaek Lee  <ytk.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */



#ifndef __MACH_SS6000_CHARGER_H
#define __MACH_SS6000_CHARGER_H

#include <linux/workqueue.h>
#include <linux/wakelock.h>

#define SS6000_CHGSB_IRQ_DELAY	(HZ*1)	//1 sec
#define SS6000_PGB_IRQ_DELAY	(HZ*1)	//1 sec

struct ss6000_platform_data {
	unsigned int en_set;
	unsigned int pgb;
	unsigned int chgsb;
	unsigned int irq_en_set;
	unsigned int irq_pgb;
	unsigned int irq_chgsb;

	struct delayed_work	chgsb_irq_work;
	struct wake_lock	chgsb_irq_wakelock;
	struct delayed_work	pgb_irq_work;
	struct wake_lock	pgb_irq_wakelock;

	struct work_struct chgsb_irq_pre_work;
	struct work_struct pgb_irq_pre_work;
};

#if defined(CONFIG_MACH_KYLEVE) || defined(CONFIG_MACH_LOGAN)
void external_restart_charge(enum power_supply_type type);
void external_enable_charge(enum power_supply_type type);
void external_disable_charge(void);
#endif

#ifdef CONFIG_MACH_NEVISTD
#define CHARGERIC_CHG_EN_GPIO 142
#define CHARGERIC_TA_INT_GPIO 135
#define CHARGERIC_CHG_INT_GPIO 143

void external_restart_charge(enum power_supply_type type);
void external_enable_charge(enum power_supply_type type);
void external_disable_charge(void);
#endif
#endif
