/* arch/arm/mach-sc8825/board-logantd-battery.c
 *
 * Copyright (C) 2012 Samsung Electronics Co, Ltd.
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

#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/switch.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/regulator/machine.h>
#include <linux/platform_device.h>

#include <linux/battery/sec_battery.h>
#include <linux/battery/sec_fuelgauge.h>
#include <linux/battery/sec_charger.h>
#include <linux/gpio_event.h>

#include <mach/regs_ahb.h>
#include <mach/globalregs.h>
#include <mach/gpio.h>
#include <mach/usb.h>
#include <mach/adc.h>
#include <mach/sci.h>

#include "board-logan.h"

#define SEC_STBC_I2C_ID 3
#define SEC_STBC_I2C_SLAVEADDR 0x71
#define SEC_STBC_CHG_EN 202
#define ADC_CHANNEL_TEMP        0
#define STBC_LOW_BATT 59

#define SEC_BATTERY_PMIC_NAME ""

#define USB_DM_GPIO 215
#define USB_DP_GPIO 216

#define TA_ADC_LOW              800
#define TA_ADC_HIGH             2200

/* cable state */
bool is_cable_attached;
unsigned int lpcharge;
EXPORT_SYMBOL(lpcharge);

static struct s3c_adc_client *temp_adc_client;

static sec_bat_adc_table_data_t logantd_temp_table[] = {
        {450,  700},
        {540,  650},
        {640,  600},
        {750,  550},
        {885,  500},
        {1035, 450},
        {1130, 400},
        {1390, 350},
        {1590, 300},
        {1800, 250},
        {2020, 200},
        {2250, 150},
        {2490, 100},
        {2720, 50},
        {2920, 0},
        {3080, -50},
        {3300, -100},
};

static sec_bat_adc_region_t cable_adc_value_table[] = {
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_UNKNOWN */
	{ 0,    500 },  /* POWER_SUPPLY_TYPE_BATTERY */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_UPS */
	{ 1000, 1500 }, /* POWER_SUPPLY_TYPE_MAINS */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_USB */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_OTG */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_DOCK */
	{ 0,    0 },    /* POWER_SUPPLY_TYPE_MISC */
};

static sec_charging_current_t charging_current_table[] = {
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_UNKNOWN */
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_BATTERY */
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_UPS */
	{550,   550,    125,    50},     /* POWER_SUPPLY_TYPE_MAINS */
	{500,   500,    125,    50},     /* POWER_SUPPLY_TYPE_USB */
	{500,   500,    125,    50},     /* POWER_SUPPLY_TYPE_USB_DCP */
	{500,   500,    125,    50},     /* POWER_SUPPLY_TYPE_USB_CDP */
	{500,   500,    125,    50},     /* POWER_SUPPLY_TYPE_USB_ACA */
	{550,   500,    125,    50},     /* POWER_SUPPLY_TYPE_MISC */
	{0,     0,      0,      0},     /* POWER_SUPPLY_TYPE_CARDOCK */
	{500,   500,    125,    50},     /* POWER_SUPPLY_TYPE_WPC */
	{550,   500,    125,    50},     /* POWER_SUPPLY_TYPE_UARTOFF */
};

/* unit: seconds */
static int polling_time_table[] = {
	10,     /* BASIC */
	30,     /* CHARGING */
	30,     /* DISCHARGING */
	30,     /* NOT_CHARGING */
	1800,    /* SLEEP */
};

static struct power_supply *charger_supply;
static bool is_jig_on;
int current_cable_type = POWER_SUPPLY_TYPE_BATTERY;
EXPORT_SYMBOL(current_cable_type);
u8 attached_cable;

static bool sec_bat_gpio_init(void)
{
	return true;
}

static bool sec_fg_gpio_init(void)
{
	return true;
}

static bool sec_chg_gpio_init(void)
{
	return true;
}

/* Get LP charging mode state */

static int battery_get_lpm_state(char *str)
{
	get_option(&str, &lpcharge);
	pr_info("%s: Low power charging mode: %d\n", __func__, lpcharge);

	return lpcharge;
}
__setup("lpcharge=", battery_get_lpm_state);

static bool sec_bat_is_lpm(void)
{
	return lpcharge == 1 ? true : false;
}

static bool sec_bat_check_jig_status(void)
{
	return is_jig_on;
}

void sec_charger_cb(u8 attached)
{
	return 0;
}

static int sec_bat_check_cable_callback(void)
{
	int ta_nconnected;
	union power_supply_propval value;
	struct power_supply *psy = power_supply_get_by_name("battery");
	int ret;

	return current_cable_type;
}

#define EIC_KEY_POWER_2                (A_EIC_START + 7) 

static void sec_bat_initial_check(void)
{
	return;
}

static bool sec_bat_check_cable_result_callback(int cable_type)
{
	bool ret = true;
	current_cable_type = cable_type;

	switch (cable_type) {
	case POWER_SUPPLY_TYPE_USB:
		pr_info("%s set vbus applied\n",
				__func__);
		break;
	case POWER_SUPPLY_TYPE_BATTERY:
		pr_info("%s set vbus cut\n",
				__func__);
		break;
	case POWER_SUPPLY_TYPE_MAINS:
		break;
	default:
		pr_err("%s cable type (%d)\n",
				__func__, cable_type);
		ret = false;
		break;
	}
	/* omap4_tab3_tsp_ta_detect(cable_type); */

	return ret;
}

/* callback for battery check
 * return : bool
 * true - battery detected, false battery NOT detected
 */
static bool sec_bat_check_callback(void) { return true; }
static bool sec_bat_check_result_callback(void) { return true; }

/* callback for OVP/UVLO check
 * return : int
 * battery health
 */
static int sec_bat_ovp_uvlo_callback(void)
{
	int health;
	health = POWER_SUPPLY_HEALTH_GOOD;

	return health;
}

static bool sec_bat_ovp_uvlo_result_callback(int health) { return true; }

/*
 * val.intval : temperature
 */
static bool sec_bat_get_temperature_callback(
		enum power_supply_property psp,
		union power_supply_propval *val) { return true; }

static bool sec_fg_fuelalert_process(bool is_fuel_alerted) { return true; }

#define CHECK_BATTERY_INTERVAL 358

int battery_updata(void)
{
	pr_info("[%s] start\n", __func__);

	int ret_val;
	static int BatteryCheckCount = 0;

	if ((current_cable_type == POWER_SUPPLY_TYPE_USB) ||
	    (current_cable_type == POWER_SUPPLY_TYPE_MAINS)) {
		ret_val = 1;
	} else if(BatteryCheckCount > CHECK_BATTERY_INTERVAL) {
		ret_val = 1;
	} else {
		ret_val = 0;
	}

	return ret_val;
}

static struct battery_data_t stbcfg01_battery_data[] = {
	{
		.Alm_en = 1,	         /* SOC and VBAT alarms enable, 0=disabled, 1=enabled */
		.Alm_SOC = 1,         /* SOC alarm level % */
		.Alm_Vbat = 3350,      /* Vbat alarm level mV */
		.VM_cnf = 210,         /* nominal VM_cnf for discharging, coming from battery characterisation */
		.VM_cnf_chg = 540,     /* nominal VM_cnf for charging, coming from battery characterisation */
		.Cnom = 1500,          /* nominal capacity in mAh, coming from battery characterisation */

		.CapDerating[6] = 0,   /* capacity derating in 0.1%, for temp = -20deg */
		.CapDerating[5] = 0,   /* capacity derating in 0.1%, for temp = -10deg */
		.CapDerating[4] = 0,   /* capacity derating in 0.1%, for temp = 0deg */
		.CapDerating[3] = 0,   /* capacity derating in 0.1%, for temp = 10deg */
		.CapDerating[2] = 0,   /* capacity derating in 0.1%, for temp = 25deg */
		.CapDerating[1] = 0,   /* capacity derating in 0.1%, for temp = 40deg */
		.CapDerating[0] = 0,   /* capacity derating in 0.1%, for temp = 60deg */

		.VM_cnf_comp[6] = 1050, /* VM_cnf temperature compensation multiplicator in %, for temp = -20deg */
		.VM_cnf_comp[5] = 550, /* VM_cnf temperature compensation multiplicator in %, for temp = -10deg */
		.VM_cnf_comp[4] = 400, /* VM_cnf temperature compensation multiplicator in %, for temp = 0deg */
		.VM_cnf_comp[3] = 200, /* VM_cnf temperature compensation multiplicator in %, for temp = 10deg */
		.VM_cnf_comp[2] = 100, /* VM_cnf temperature compensation multiplicator in %, for temp = 25deg */
		.VM_cnf_comp[1] = 82,  /* VM_cnf temperature compensation multiplicator in %, for temp = 40deg */
		.VM_cnf_comp[0] = 64,  /* VM_cnf temperature compensation multiplicator in %, for temp = 60deg */

		.OCVOffset[15] = 20,    /* OCV curve adjustment */
		.OCVOffset[14] = -18,  /* OCV curve adjustment */
		.OCVOffset[13] = -23,  /* OCV curve adjustment */
		.OCVOffset[12] = -12,  /* OCV curve adjustment */
		.OCVOffset[11] = -20,  /* OCV curve adjustment */
		.OCVOffset[10] = -23,  /* OCV curve adjustment */
		.OCVOffset[9] = -15,   /* OCV curve adjustment */
		.OCVOffset[8] = -10,   /* OCV curve adjustment */
		.OCVOffset[7] = -8,    /* OCV curve adjustment */
		.OCVOffset[6] = -7,    /* OCV curve adjustment */
		.OCVOffset[5] = -22,   /* OCV curve adjustment */
		.OCVOffset[4] = -35,   /* OCV curve adjustment */
		.OCVOffset[3] = -46,   /* OCV curve adjustment */
		.OCVOffset[2] = -82,   /* OCV curve adjustment */
		.OCVOffset[1] = -103,  /* OCV curve adjustment */
		.OCVOffset[0] = 83,     /* OCV curve adjustment */

		.SysCutoffVolt = 3000, /* system cut-off voltage, system does not work under this voltage (platform UVLO) in mV */
		.EarlyEmptyCmp = 0,	   /* difference vs SysCutoffVolt to start early empty compensation in mV, typically 200, 0 for disable */

		.IFast = 2,            /* fast charge current, range 550-1250mA, value = IFast*100+550 mA */
		.IFast_usb = 0,        /* fast charge current (another conf. for USB), range 550-1250mA, value = IFast*100+550 mA */
		.IPre = 0,             /* pre-charge current, 0=450 mA, 1=100 mA */
		.ITerm = 4,            /* termination current, range 50-300 mA, value = ITerm*25+50 mA */
		.VFloat = 41,          /* floating voltage, range 3.52-4.78 V, value = VFloat*0.02+3.52 V */
		.ARChg = 0,            /* automatic recharge, 0=disabled, 1=enabled */
		.Iin_lim = 4,          /* input current limit, 0=100mA 1=500mA 2=800mA 3=1.2A 4=no limit */
		.DICL_en = 1,          /* dynamic input current limit, 0=disabled, 1=enabled */
		.VDICL = 2,            /* dynamic input curr lim thres, range 4.00-4.75V, value = VDICL*0.25+4.00 V */
		.DIChg_adj = 1,        /* dynamic charging current adjust enable, 0=disabled, 1=enabled */
		.TPre = 0,             /* pre-charge timer, 0=disabled, 1=enabled */
		.TFast = 0,            /* fast charge timer, 0=disabled, 1=enabled */
		.PreB_en = 1,          /* pre-bias function, 0=disabled, 1=enabled */
		.LDO_UVLO_th = 0,      /* LDO UVLO threshold, 0=3.5V 1=4.65V */
		.LDO_en = 1,           /* LDO output, 0=disabled, 1=enabled */
		.IBat_lim = 2,         /* OTG average battery current limit, 0=350mA 1=450mA 2=550mA 3=950mA */
		.WD = 0,               /* watchdog timer function, 0=disabled, 1=enabled */
		.FSW = 0,              /* switching frequency selection, 0=2MHz, 1=3MHz */

		.GPIO_cd = SEC_STBC_CHG_EN, /* charger disable GPIO */
		.GPIO_shdn = 0,                          /* all charger circuitry shutdown GPIO or 0=not used */

		.ExternalTemperature = 250,   /* external temperature function, return in 0.1deg */
		.power_supply_register = NULL,           /* custom power supply structure registration function */
		.power_supply_unregister = NULL,         /* custom power supply structure unregistration function */
	}
};

static void sec_bat_check_vf_callback(void)
{
	return;
}

static bool sec_bat_adc_none_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_none_exit(void) { return true; }
static int sec_bat_adc_none_read(unsigned int channel) { return 0; }

static bool sec_bat_adc_ap_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_ap_exit(void) { return true; }
static int sec_bat_adc_ap_read(unsigned int channel)
{
        int data;
        int ret = 0;

	data = sci_adc_get_value(ADC_CHANNEL_TEMP, false);
	return data;
}
static bool sec_bat_adc_ic_init(struct platform_device *pdev) { return true; }
static bool sec_bat_adc_ic_exit(void) { return true; }
static int sec_bat_adc_ic_read(unsigned int channel) { return 0; }

sec_battery_platform_data_t sec_battery_pdata = {
	/* NO NEED TO BE CHANGED */
	.initial_check = sec_bat_initial_check,
	.bat_gpio_init = sec_bat_gpio_init,
	.fg_gpio_init = sec_fg_gpio_init,
	.chg_gpio_init = sec_chg_gpio_init,

	.is_lpm = sec_bat_is_lpm,
	.check_jig_status = sec_bat_check_jig_status,
	.check_cable_callback =
		sec_bat_check_cable_callback,
	.check_cable_result_callback =
		sec_bat_check_cable_result_callback,
	.check_battery_callback =
		sec_bat_check_callback,
	.check_battery_result_callback =
		sec_bat_check_result_callback,
	.ovp_uvlo_callback = sec_bat_ovp_uvlo_callback,
	.ovp_uvlo_result_callback =
	sec_bat_ovp_uvlo_result_callback,
	
	.fuelalert_process = sec_fg_fuelalert_process,
	.get_temperature_callback =
		sec_bat_get_temperature_callback,

	.adc_api[SEC_BATTERY_ADC_TYPE_NONE] = {
		.init = sec_bat_adc_none_init,
		.exit = sec_bat_adc_none_exit,
		.read = sec_bat_adc_none_read
	},
	.adc_api[SEC_BATTERY_ADC_TYPE_AP] = {
		.init = sec_bat_adc_ap_init,
		.exit = sec_bat_adc_ap_exit,
		.read = sec_bat_adc_ap_read
	},
	.adc_api[SEC_BATTERY_ADC_TYPE_IC] = {
		.init = sec_bat_adc_ic_init,
		.exit = sec_bat_adc_ic_exit,
		.read = sec_bat_adc_ic_read
	},
	.cable_adc_value = cable_adc_value_table,
	.charging_current = charging_current_table,
	.polling_time = polling_time_table,
	/* NO NEED TO BE CHANGED */

	.pmic_name = SEC_BATTERY_PMIC_NAME,

	.adc_check_count = 5,

	.adc_type = {
		SEC_BATTERY_ADC_TYPE_NONE,	/* CABLE_CHECK */
		SEC_BATTERY_ADC_TYPE_NONE,      /* BAT_CHECK */
		SEC_BATTERY_ADC_TYPE_AP,	/* TEMP */
		SEC_BATTERY_ADC_TYPE_NONE,	/* TEMP_AMB */
		SEC_BATTERY_ADC_TYPE_NONE,      /* FULL_CHECK */
	},

	/* Battery */
	.vendor = "SDI SDI",
	.technology = POWER_SUPPLY_TECHNOLOGY_LION,
	.battery_data = (void *)stbcfg01_battery_data,
	.bat_polarity_ta_nconnected = 1,        /* active HIGH */
	.bat_irq_attr = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.cable_check_type =
		SEC_BATTERY_CABLE_CHECK_INT,
	.cable_source_type = SEC_BATTERY_CABLE_SOURCE_CALLBACK |
	SEC_BATTERY_CABLE_SOURCE_EXTERNAL,

	.event_check = false,
	.event_waiting_time = 60,

	/* Monitor setting */
	.polling_type = SEC_BATTERY_MONITOR_WORKQUEUE,
	.monitor_initial_count = 3,

	/* Battery check */
	.battery_check_type = SEC_BATTERY_CHECK_FUELGAUGE,
	.check_count = 3,

	/* Battery check by ADC */
	.check_adc_max = 0,
	.check_adc_min = 0,

	/* OVP/UVLO check */
	.ovp_uvlo_check_type = SEC_BATTERY_OVP_UVLO_CHGPOLLING,

	.thermal_source = SEC_BATTERY_THERMAL_SOURCE_ADC,
	.temp_check_type = SEC_BATTERY_TEMP_CHECK_TEMP,
	.temp_adc_table = logantd_temp_table,
	.temp_adc_table_size =
	sizeof(logantd_temp_table)/sizeof(sec_bat_adc_table_data_t),
	.temp_amb_adc_table = logantd_temp_table,
	.temp_amb_adc_table_size =
		sizeof(logantd_temp_table)/sizeof(sec_bat_adc_table_data_t),

        .temp_check_count = 1,
        .temp_high_threshold_event = 600,
        .temp_high_recovery_event = 400,
        .temp_low_threshold_event = -50,
        .temp_low_recovery_event = 0,
        .temp_high_threshold_normal = 600,
        .temp_high_recovery_normal = 400,
        .temp_low_threshold_normal = -50,
        .temp_low_recovery_normal = 0,
        .temp_high_threshold_lpm = 600,
        .temp_high_recovery_lpm = 400,
        .temp_low_threshold_lpm = -50,
        .temp_low_recovery_lpm = 0,

	.full_check_type = SEC_BATTERY_FULLCHARGED_CHGPSY,
	.full_check_type_2nd = SEC_BATTERY_FULLCHARGED_CHGPSY,
	.full_check_count = 1,
	.chg_polarity_full_check = 1,
	.full_condition_type = SEC_BATTERY_FULL_CONDITION_NOTIMEFULL |
		SEC_BATTERY_FULL_CONDITION_SOC |
		SEC_BATTERY_FULL_CONDITION_OCV,
	.full_condition_soc = 95,
	.full_condition_ocv = 4300,

	.recharge_condition_type =
                SEC_BATTERY_RECHARGE_CONDITION_VCELL,

	.recharge_condition_soc = 98,
	.recharge_condition_avgvcell = 4300,
	.recharge_condition_vcell = 4300,

	.charging_total_time = 6 * 60 * 60,
        .recharging_total_time = 90 * 60,
        .charging_reset_time = 0,

	/* Fuel Gauge */
	.fg_irq = STBC_LOW_BATT,
	.fg_irq_attr = IRQF_TRIGGER_FALLING |
	IRQF_TRIGGER_RISING,
	.fuel_alert_soc = 1,
	.repeated_fuelalert = false,
	.capacity_calculation_type =
	SEC_FUELGAUGE_CAPACITY_TYPE_DYNAMIC_SCALE,
	.capacity_max = 1000,
	.capacity_min = 0,
	.capacity_max_margin = 30,

	/* Charger */
	.chg_polarity_en = 0,   /* active LOW charge enable */
	.chg_polarity_status = 0,
	.chg_irq_attr = IRQF_TRIGGER_RISING,

	.chg_float_voltage = 4350,
};


static struct stbcfg01_platform_data stbc_battery_pdata = {
	.charger_data = &sec_battery_pdata,
	.fuelgauge_data = &sec_battery_pdata,
};

static struct platform_device sec_device_battery = {
	.name = "sec-battery",
	.id = -1,
	.dev = {
		.platform_data = &sec_battery_pdata,
	},
};

static struct i2c_board_info sec_brdinfo_stbc[] __initdata = {
	{
		I2C_BOARD_INFO("sec-stbc",
				SEC_STBC_I2C_SLAVEADDR),
		.platform_data = &stbc_battery_pdata,
	},
};

static struct platform_device *sec_battery_devices[] __initdata = {
	&sec_device_battery,
};


void __init sprd_battery_init(void)
{
	pr_info("%s: logantd battery init\n", __func__);

	i2c_register_board_info(SEC_STBC_I2C_ID, sec_brdinfo_stbc,
			ARRAY_SIZE(sec_brdinfo_stbc));

	platform_add_devices(sec_battery_devices,
		ARRAY_SIZE(sec_battery_devices));

}

