/* drivers/video/sc8825/lcd_hx8369b_mipi.c
 *
 * Support for hx8369b mipi LCD device
 *
 * Copyright (C) 2010 Spreadtrum
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include "sprdfb_panel.h"
#include <linux/ktd253b_bl.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/err.h>
#include "sprdfb_dispc_reg.h"
#include <linux/workqueue.h>
#include <mach/gpio.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/io.h>
#include <mach/hardware.h>
#include <linux/timer.h>
#include <linux/semaphore.h>
//#define LCD_Delay(ms)  uDelay(ms*1000)

//thomaszhang debug
#define  LCD_DEBUG
#ifdef LCD_DEBUG
#define LCD_PRINT printk
#else
#define LCD_PRINT(...)
#endif

#define MAX_DATA   150

typedef struct LCM_Init_Code_tag {
	unsigned int tag;
	unsigned char data[MAX_DATA];
}LCM_Init_Code;

typedef struct LCM_force_cmd_code_tag{
	unsigned int datatype;
	LCM_Init_Code real_cmd_code;
}LCM_Force_Cmd_Code;

#define LCM_TAG_SHIFT 24
#define LCM_TAG_MASK  ((1 << 24) -1)
#define LCM_SEND(len) ((1 << LCM_TAG_SHIFT)| len)
#define LCM_SLEEP(ms) ((2 << LCM_TAG_SHIFT)| ms)
//#define ARRAY_SIZE(array) ( sizeof(array) / sizeof(array[0]))

#define LCM_TAG_SEND  (1<< 0)
#define LCM_TAG_SLEEP (1 << 1)

#ifdef CONFIG_BACKLIGHT_USE_HX8369B_CABC_PWM
static u32 current_brightness = 0;
#define DEFAULT_BRIGHTNESS (0x7F)
static LCM_Init_Code cabc_pwm_en[] = {
	{LCM_SEND(6), {4, 0, 0xB9, 0xFF, 0x83, 0x69}},
	{LCM_SEND(5), {3, 0, 0xC9, 0x0F, 0x00}},
	{LCM_SEND(2), {0x51, 0x00}},
	{LCM_SEND(2), {0x53, 0x24}},
};
#endif

static LCM_Init_Code init_data[] = {
	{LCM_SEND(6), {4, 0, 0xB9, 0xFF, 0x83, 0x69}},
	{LCM_SEND(18), {16, 0, 0xBA,
			       0x31, 0x00, 0x16, 0xCA, 0xB1, 0x0A, 0x00, 0x10, 0x28, 0x02,
			       0x21, 0x21, 0x9A, 0x1A, 0x8F}},
	{LCM_SEND(95), {93, 0, 0xD5,
			       0x00, 0x00, 0x0F, 0x03, 0x36, 0x00, 0x00, 0x10, 0x01, 0x00,
			       0x00, 0x00, 0x1A, 0x50, 0x45, 0x00, 0x00, 0x13, 0x44, 0x39,
			       0x47, 0x00, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00,
			       0x00, 0x00, 0x03, 0x00, 0x00, 0x08, 0x88, 0x88, 0x37, 0x5F,
			       0x1E, 0x18, 0x88, 0x88, 0x85, 0x88, 0x88, 0x40, 0x2F, 0x6E,
			       0x48, 0x88, 0x88, 0x80, 0x88, 0x88, 0x26, 0x4F, 0x0E, 0x08,
			       0x88, 0x88, 0x84, 0x88, 0x88, 0x51, 0x3F, 0x7E, 0x58, 0x88,
			       0x88, 0x81, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x07,
			       0xF8, 0x0F, 0xFF, 0xFF, 0x07, 0xF8, 0x0F, 0xFF, 0xFF, 0x00,
			       0x18, 0x5A}},
	{LCM_SEND(2), {0x3A, 0x70}},
	{LCM_SEND(2), {0x36, 0x00}},
	{LCM_SEND(13), {11, 0, 0xB1,
			       0x12, 0x83, 0x77, 0x00, 0x12, 0x12, 0x1A, 0x1A, 0x0C, 0x0A}},
	{LCM_SEND(7), {5, 0, 0xB3,
			      0x83, 0x00, 0x3A, 0x17}},
	{LCM_SEND(2), {0xB4, 0x00}},
	{LCM_SEND(7), {5, 0, 0xE3, 0x0F, 0x0F, 0x0F, 0x0F}},
	{LCM_SEND(9), {7, 0, 0xC0, 0x73, 0x50 ,0x00, 0x34, 0xC4, 0x00}},
	{LCM_SEND(2), {0xC1, 0x00}},
	{LCM_SEND(6), {4, 0, 0xC6, 0x41, 0xFF, 0x7D}},
	{LCM_SEND(2), {0xCC, 0x0C}},
	{LCM_SEND(2), {0xEA, 0x7A}},
	{LCM_SEND(38), {36, 0, 0xE0,
			       0x00, 0x0F, 0x15, 0x11, 0x10, 0x3C, 0x1E, 0x32, 0x06, 0x0A,
			       0x11, 0x16, 0x19, 0x17, 0x17, 0x10, 0x12, 0x00, 0x0F, 0x15,
			       0x11, 0x10, 0x3C, 0x1E, 0x32, 0x06, 0x0A, 0x10, 0x15, 0x18,
			       0x15, 0x16, 0x10, 0x12, 0x01}},
	{LCM_SEND(1), {0x11}}, // sleep out
	{LCM_SLEEP(150),},

	{LCM_SEND(1), {0x29}}, // display on
	{LCM_SLEEP(50),},
};


static LCM_Init_Code sleep_in[] =  {
	{LCM_SEND(1), {0x28}},
	{LCM_SLEEP(150)},
	{LCM_SEND(1), {0x10}},
	{LCM_SLEEP(150)},
//	{LCM_SEND(2), {0x4f, 0x01}},
};

static LCM_Init_Code sleep_out[] =  {
	{LCM_SEND(1), {0x11}},
	{LCM_SLEEP(120)},
	{LCM_SEND(1), {0x29}},
	{LCM_SLEEP(20)},
};

static LCM_Force_Cmd_Code rd_prep_code_1[]={
	{0x37, {LCM_SEND(2), {0x1, 0}}},
};

static LCM_Force_Cmd_Code rd_prep_code_3[]={
	{0x37, {LCM_SEND(2), {0x3, 0}}},
};

static int32_t hx8369b_mipi_write(struct panel_spec *self, LCM_Init_Code *cmd, uint32_t len)
{
	int32_t i;
	unsigned int tag;

	mipi_gen_write_t mipi_gen_write = self->info.mipi->ops->mipi_gen_write;

	for (i = 0; i < len; i++) {
		tag = (cmd->tag >> 24);
		if (tag & LCM_TAG_SEND) {
			mipi_gen_write(cmd->data, (cmd->tag & LCM_TAG_MASK));
			mdelay(1);
		} else if (tag & LCM_TAG_SLEEP) {
			udelay((cmd->tag & LCM_TAG_MASK) * 1000);
		}
		cmd++;
	}
	return 0;
}

#ifdef CONFIG_BACKLIGHT_USE_HX8369B_CABC_PWM
int32_t hx8369b_set_brightness(struct panel_spec *self, uint16_t brightness)
{
#if 1
	int min = 10;
	int def = 100;
	int max = 240;
#endif

	static LCM_Init_Code set_bl_level[] = {
		{LCM_SEND(2), {0x51, 0x00}},
	};

	static LCM_Init_Code set_bl_on[] = {
		{LCM_SEND(2), {0x53, 0x24}},
	};

	static LCM_Init_Code set_bl_off[] = {
		{LCM_SEND(2), {0x53, 0x00}},
	};

	if (brightness > 255 || brightness < 0) {
		printk("[backlight] out of range : %d\n", brightness);
		return 0;
	}

#if 1
	if (!brightness) {
		brightness = 0;
	} else if (brightness <= min) {
		brightness = 10;
	} else if (brightness <= def) {
		brightness = brightness * 115 / 100;
	} else {
		brightness = brightness * 240 / 255;
	}
#endif

	if (current_brightness == brightness) {
		printk("[backlight] same as before brightness : %d\n", brightness);
		return 0;
	}

	set_bl_level->data[1] = brightness;
	printk("set brightness : %d\n", brightness);

	if (!brightness) {
		hx8369b_mipi_write(self, set_bl_level, ARRAY_SIZE(set_bl_level));
		hx8369b_mipi_write(self, set_bl_off, ARRAY_SIZE(set_bl_off));
	} else {
		hx8369b_mipi_write(self, set_bl_level, ARRAY_SIZE(set_bl_level));
		if (!current_brightness)
			hx8369b_mipi_write(self, set_bl_on, ARRAY_SIZE(set_bl_on));
	}
	current_brightness = brightness;

	return 0;
}
#endif

static int32_t hx8369b_mipi_init(struct panel_spec *self)
{
	mipi_set_cmd_mode_t mipi_set_cmd_mode = self->info.mipi->ops->mipi_set_cmd_mode;

	printk("kernel hx8369b_mipi_init\n");

	mipi_set_cmd_mode();
#ifdef CONFIG_BACKLIGHT_USE_HX8369B_CABC_PWM
	current_brightness = 0;
	hx8369b_mipi_write(self, cabc_pwm_en, ARRAY_SIZE(cabc_pwm_en));
#endif
	hx8369b_mipi_write(self, init_data, ARRAY_SIZE(init_data));

	return 0;
}

#define HX8369B_MTP_ID1  0x55
#define HX8369B_MTP_ID2  0x49
#define HX8369B_MTP_ID3  0x90
static uint32_t hx8369b_readid(struct panel_spec *self)
{
#if 0	
		int32_t j =0;
		uint8_t read_data[4] = {0};
		int32_t read_rtn = 0;
		uint8_t param[2] = {0};
		mipi_set_cmd_mode_t mipi_set_cmd_mode = self->info.mipi->ops->mipi_set_cmd_mode;
		mipi_force_write_t mipi_force_write = self->info.mipi->ops->mipi_force_write;
		mipi_force_read_t mipi_force_read = self->info.mipi->ops->mipi_force_read;

		LCD_PRINT("kernel lcd_hx8369b_mipi read id!\n");
	
		mipi_set_cmd_mode();
	
		for(j = 0; j < 4; j++){
			param[0] = 0x01;
			param[1] = 0x00;
			mipi_force_write(0x37, param, 2);
			read_rtn = mipi_force_read(0xda,1,&read_data[0]);
			LCD_PRINT("lcd_hx8369b_mipi read id 0xda value is 0x%x!\n",read_data[0]);
	
			read_rtn = mipi_force_read(0xdb,1,&read_data[1]);
			LCD_PRINT("lcd_hx8369b_mipi read id 0xdb value is 0x%x!\n",read_data[1]);
	
			read_rtn = mipi_force_read(0xdc,1,&read_data[2]);
			LCD_PRINT("lcd_hx8369b_mipi read id 0xdc value is 0x%x!\n",read_data[2]);
	
			if((HX8369B_MTP_ID1 == read_data[0])&&(HX8369B_MTP_ID2 == read_data[1])&&(HX8369B_MTP_ID3 == read_data[2])){
					printk("lcd_hx8369b_mipi read id success!\n");
					return 0x8369;
				}
			}
#endif		
	return 0x10;
}


static int32_t hx8369b_enter_sleep(struct panel_spec *self, uint8_t is_sleep)
{
	int32_t i;
	LCM_Init_Code *sleep_in_out = NULL;
	unsigned int tag;
	int32_t size = 0;

	mipi_set_cmd_mode_t mipi_set_cmd_mode = self->info.mipi->ops->mipi_set_cmd_mode;
	mipi_gen_write_t mipi_gen_write = self->info.mipi->ops->mipi_gen_write;

	printk("kernel hx8369b_enter_sleep, is_sleep = %d\n", is_sleep);

	if(is_sleep){
#ifdef CONFIG_BACKLIGHT_USE_HX8369B_CABC_PWM
		if (current_brightness) {
			hx8369b_set_brightness(self, 0);
		}
#endif
		sleep_in_out = sleep_in;
		size = ARRAY_SIZE(sleep_in);
	}else{
		sleep_in_out = sleep_out;
		size = ARRAY_SIZE(sleep_out);
	}
	
	mipi_set_cmd_mode();	

	for(i = 0; i <size ; i++){
		tag = (sleep_in_out->tag >>24);
		if(tag & LCM_TAG_SEND){
			mipi_gen_write(sleep_in_out->data, (sleep_in_out->tag & LCM_TAG_MASK));
		}else if(tag & LCM_TAG_SLEEP){
			msleep(sleep_in_out->tag & LCM_TAG_MASK);
		}
		sleep_in_out++;
	}
	return 0;
}

static uint32_t hx8369b_readpowermode(struct panel_spec *self)
{
	int32_t i = 0;
	uint32_t j =0;
	LCM_Force_Cmd_Code * rd_prepare = rd_prep_code_1;
	uint8_t read_data[1] = {0};
	int32_t read_rtn = 0;
	unsigned int tag = 0;

	mipi_force_write_t mipi_force_write = self->info.mipi->ops->mipi_force_write;
	mipi_force_read_t mipi_force_read = self->info.mipi->ops->mipi_force_read;
	mipi_eotp_set_t mipi_eotp_set = self->info.mipi->ops->mipi_eotp_set;

//	pr_debug("lcd_hx8369b_mipi read power mode!\n");

	mipi_eotp_set(0,0);
	for(j = 0; j < 4; j++){
		rd_prepare = rd_prep_code_1;
		for(i = 0; i < ARRAY_SIZE(rd_prep_code_1); i++){
			tag = (rd_prepare->real_cmd_code.tag >> 24);
			if(tag & LCM_TAG_SEND){
				mipi_force_write(rd_prepare->datatype, rd_prepare->real_cmd_code.data, (rd_prepare->real_cmd_code.tag & LCM_TAG_MASK));
			}else if(tag & LCM_TAG_SLEEP){
				udelay((rd_prepare->real_cmd_code.tag & LCM_TAG_MASK) * 1000);
			}
			rd_prepare++;
		}
		read_rtn = mipi_force_read(0x0A, 1,(uint8_t *)read_data);
		//printk("lcd_hx8369b mipi read power mode 0x0A value is 0x%x! , read result(%d)\n", read_data[0], read_rtn);

		if((0x9c == read_data[0])  && (0 == read_rtn)){
			pr_debug("lcd_hx8369b_mipi read power mode success!\n");
			mipi_eotp_set(1,1);
			return 0x9c;
		}
	}
	mipi_eotp_set(1,1);
	return 0x0;
}

static int32_t hx8369b_check_esd(struct panel_spec *self)
{
	uint32_t power_mode;
	uint16_t work_mode = self->info.mipi->work_mode;

	mipi_set_lp_mode_t mipi_set_data_lp_mode = self->info.mipi->ops->mipi_set_data_lp_mode;
	mipi_set_hs_mode_t mipi_set_data_hs_mode = self->info.mipi->ops->mipi_set_data_hs_mode;
	mipi_set_lp_mode_t mipi_set_lp_mode = self->info.mipi->ops->mipi_set_lp_mode;
	mipi_set_hs_mode_t mipi_set_hs_mode = self->info.mipi->ops->mipi_set_hs_mode;

//	pr_debug("hx8369b_check_esd!\n");
	if(SPRDFB_MIPI_MODE_CMD==work_mode){
		mipi_set_lp_mode();
	}else{
		mipi_set_data_lp_mode();
	}
	power_mode =  hx8369b_readpowermode(self);
	//power_mode = 0x0;
	if(SPRDFB_MIPI_MODE_CMD==work_mode){
		mipi_set_hs_mode();
	}else{
		mipi_set_data_hs_mode();
	}
	if (power_mode == 0x9c) {
		//printk("hx8369b_check_esd OK!\n");
		return 1;
	} else {
		pr_debug("hx8369b_check_esd fail!(0x%x)\n", power_mode);
		return 0;
	}
}


static struct panel_operations lcd_hx8369b_mipi_operations = {
	.panel_init = hx8369b_mipi_init,
	.panel_readid = hx8369b_readid,
	.panel_enter_sleep = hx8369b_enter_sleep,
	.panel_esd_check = hx8369b_check_esd,
#ifdef CONFIG_BACKLIGHT_USE_HX8369B_CABC_PWM
	.panel_set_brightness = hx8369b_set_brightness,
#endif
};

static struct timing_rgb lcd_hx8369b_mipi_timing = {
	.hfp = 100, /* unit: pixel */
	.hbp = 70,
	.hsync = 32,
	.vfp = 20, /*unit: line*/
	.vbp = 21,
	.vsync = 2,
};


static struct info_mipi lcd_hx8369b_mipi_info = {
	.work_mode  = SPRDFB_MIPI_MODE_VIDEO,
	.video_bus_width = 24, /*18,16*/
	.lan_number = 	2,
	.phy_feq =500*1000,
	.h_sync_pol = SPRDFB_POLARITY_POS,
	.v_sync_pol = SPRDFB_POLARITY_POS,
	.de_pol = SPRDFB_POLARITY_POS,
	.te_pol = SPRDFB_POLARITY_POS,
	.color_mode_pol = SPRDFB_POLARITY_NEG,
	.shut_down_pol = SPRDFB_POLARITY_NEG,
	.timing = &lcd_hx8369b_mipi_timing,
	.ops = NULL,	
};

struct panel_spec lcd_hx8369b_mipi_spec = {
	.width = 480,
	.height = 800,
	.type = LCD_MODE_DSI,
	.direction = LCD_DIRECT_NORMAL,
	.info = {
		.mipi = &lcd_hx8369b_mipi_info
	},
	.ops = &lcd_hx8369b_mipi_operations,
};
struct panel_cfg lcd_hx8369b_mipi = {
	/* this panel can only be main lcd */
	.dev_id = SPRDFB_MAINLCD_ID,
	.lcd_id = 0x55AF90,
	.lcd_name = "lcd_hx8369b_mipi",
	.panel = &lcd_hx8369b_mipi_spec,
};

ssize_t lcd_panelName_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "hx8369b");
}

ssize_t lcd_MTP_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	printk("ldi mtpdata: %06x\n", lcd_hx8369b_mipi.lcd_id);
	return 3;
}

ssize_t lcd_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "INH_%06x", lcd_hx8369b_mipi.lcd_id);
}


static DEVICE_ATTR(lcd_MTP, S_IRUGO | S_IWUSR | S_IWGRP | S_IXOTH, lcd_MTP_show, NULL);
static DEVICE_ATTR(lcd_panelName, S_IRUGO | S_IWUSR | S_IWGRP | S_IXOTH, lcd_panelName_show, NULL);
static DEVICE_ATTR(lcd_type, S_IRUGO | S_IWUSR | S_IWGRP | S_IXOTH, lcd_type_show, NULL);


static int __init lcd_hx8369b_mipi_init(void)
{
	struct device *dev_t;
	struct class *lcd_class;

	lcd_class = class_create(THIS_MODULE, "lcd_status");

	if (IS_ERR(lcd_class)) {
		printk("Failed to create lcd_class!\n");
		return PTR_ERR( lcd_class );
	}

	dev_t = device_create( lcd_class, NULL, 0, "%s", "lcd_status");

	if (device_create_file(dev_t, &dev_attr_lcd_panelName) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_lcd_panelName.attr.name);
	if (device_create_file(dev_t, &dev_attr_lcd_MTP) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_lcd_MTP.attr.name);
	if (device_create_file(dev_t, &dev_attr_lcd_type) < 0)
		printk("Failed to create device file(%s)!\n", dev_attr_lcd_type.attr.name);    

	return sprdfb_panel_register(&lcd_hx8369b_mipi);
}

subsys_initcall(lcd_hx8369b_mipi_init);

