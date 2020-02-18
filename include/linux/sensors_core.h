/*
 * Driver model for sensor
 *
 * Copyright (C) 2008 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef __LINUX_SENSORS_CORE_H_INCLUDED
#define __LINUX_SENSORS_CORE_H_INCLUDED

extern struct class *sensors_class;
extern int sensors_register(struct device *dev,
	void *drvdata, struct device_attribute *attributes[], char *name);
extern void sensors_unregister(struct device *dev);

#ifdef CONFIG_MULTI_ACC_GARDA
extern bool isAccFound;
#endif

struct accel_platform_data {
	// (*accel_get_position) (void);
	int accel_position;
	 /* Change axis or not for user-level
	 * If it is true, driver reports adjusted axis-raw-data
	 * to user-space based on accel_get_position() value,
	 * or if it is false, driver reports original axis-raw-data */
	bool axis_adjust;
	//axes_func_s16 (*select_func) (u8);
};

#endif	/* __LINUX_SENSORS_CORE_H_INCLUDED */
