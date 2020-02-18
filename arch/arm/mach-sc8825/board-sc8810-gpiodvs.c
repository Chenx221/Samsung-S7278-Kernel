/* board-sc8810-gpio.c
 *
 * Copyright (C) 2013, Samsung Electronics.
 *
 * This program is free software. You can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation
 */
#include <linux/io.h>
#include <linux/secgpio_dvs.h>
#include <linux/platform_device.h>
#include <mach/globalregs.h>
#include <mach/gpio.h>
#include <mach/pinmap.h>

struct sc8810_gpio {
	int	gpio;
	unsigned	offset;
	int	gpio_value;
};

static struct sc8810_gpio gpio_table[] = {
	{ 16,  REG_PIN_SIMCLK0,	0x3 },
	{ 17,  REG_PIN_SIMDA0,	0x3 },
	{ 18,  REG_PIN_SIMRST0,	0x3 },
	{ 19,  REG_PIN_SIMCLK1,	0x3 },
	{ 20,  REG_PIN_SIMDA1,	0x3 },
	{ 21,  REG_PIN_SIMRST1,	0x3 },
	{ 22,  REG_PIN_SD0_CMD,	0x3 },
	{ 23,  REG_PIN_SD0_D0,	0x3 },
	{ 24,  REG_PIN_SD0_D1,	0x3 },
	{ 25,  REG_PIN_SD0_D2,	0x3 },
	{ 26,  REG_PIN_SD0_D3,	0x3 },
	{ 27,  REG_PIN_CCIRCK,	0x3 },
	{ 28,  REG_PIN_CCIRHS,	0x3 },
	{ 29,  REG_PIN_CCIRVS,	0x3 },
	{ 30,  REG_PIN_CCIRMCLK,	0x3 },
	{ 31,  REG_PIN_CCIRD0,	0x3 },
	{ 32,  REG_PIN_CCIRD1,	0x3 },
	{ 33,  REG_PIN_CCIRD2,	0x3 },
	{ 34,  REG_PIN_CCIRD3,	0x3 },
	{ 35,  REG_PIN_CCIRD4,	0x3 },
	{ 36,  REG_PIN_CCIRD5,	0x3 },
	{ 37,  REG_PIN_CCIRD6,	0x3},
	{ 38,  REG_PIN_CCIRD7,	0x3 },
	{ 39,  REG_PIN_CCIRD8,	0x3 },
	{ 40,  REG_PIN_CCIRD9,	0x3 },
	{ 41,  REG_PIN_CCIRRST,	0x3 },
	{ 42,  REG_PIN_CCIRPD1,	0x3 },
	{ 43,  REG_PIN_CCIRPD0,	0x3 },
	{ 44,  REG_PIN_SCL1,	0x3 },
	{ 45,  REG_PIN_SDA1,	0x3 },
	{ 46,  REG_PIN_KEYOUT0,	0x3 },
	{ 47,  REG_PIN_KEYOUT1,	0x3 },
	{ 48,  REG_PIN_KEYOUT2,	0x3 },
	{ 49,  REG_PIN_KEYOUT3,	0x3 },
	{ 50,  REG_PIN_KEYOUT4,	0x3 },
	{ 51,  REG_PIN_KEYOUT5,	0x3 },
	{ 52,  REG_PIN_KEYOUT6,	0x3 },
	{ 53,  REG_PIN_KEYOUT7,	0x3 },
	{ 54,  REG_PIN_KEYIN0,	0x3 },
	{ 55,  REG_PIN_KEYIN1,	0x3 },
	{ 56,  REG_PIN_KEYIN2,	0x3 },
	{ 57,  REG_PIN_KEYIN3,	0x3 },
	{ 58,  REG_PIN_KEYIN4,	0x3 },
	{ 59,  REG_PIN_KEYIN5,	0x3 },
	{ 60,  REG_PIN_NFWPN,	0x3 },
	{ 61,  REG_PIN_NFRB,	0x3 },
	{ 62,  REG_PIN_NFCLE,	0x3 },
	{ 63,  REG_PIN_NFALE,	0x3 },
	{ 64,  REG_PIN_NFCEN,	0x3 },
	{ 65,  REG_PIN_NFREN,	0x3 },
	{ 66,  REG_PIN_NFWEN,	0x3 },
	{ 67,  REG_PIN_NFD0,	0x3 },
	{ 68,  REG_PIN_NFD1,	0x3 },
	{ 69,  REG_PIN_NFD2,	0x3 },
	{ 70,  REG_PIN_NFD3,	0x3 },
	{ 71,  REG_PIN_NFD4,	0x3 },
	{ 72,  REG_PIN_NFD5,	0x3 },
	{ 73,  REG_PIN_NFD6,	0x3 },
	{ 74,  REG_PIN_NFD7,	0x3 },
	{ 75,  REG_PIN_NFD8,	0x3 },
	{ 76,  REG_PIN_NFD9,	0x3 },
	{ 77,  REG_PIN_NFD10,	0x3 },
	{ 78,  REG_PIN_NFD11,	0x3 },
	{ 79,  REG_PIN_NFD12,	0x3 },
	{ 80,  REG_PIN_NFD13,	0x3 },
	{ 81,  REG_PIN_NFD14,	0x3 },
	{ 82,  REG_PIN_NFD15,	0x3 },
	{ 83,  REG_PIN_SD3_CLK,	0x3 },
	{ 84,  REG_PIN_SD3_CMD,	0x3 },
	{ 85,  REG_PIN_SD3_D0,	0x3 },
	{ 86,  REG_PIN_SD3_D1,	0x3 },
	{ 87,  REG_PIN_SD3_D2,	0x3 },
	{ 88,  REG_PIN_SD3_D3,	0x3 },
	{ 89,  REG_PIN_SD3_D4,	0x3 },
	{ 90,  REG_PIN_SD3_D5,	0x3 },
	{ 91,  REG_PIN_SD3_D6,	0x3 },
	{ 92,  REG_PIN_SD3_D7,	0x3 },
	{ 93,  REG_PIN_EMMC_RST,	0x3 },
	{ 94,  REG_PIN_SD1_CLK,	0x3 },
	{ 95,  REG_PIN_SD1_CMD,	0x3 },
	{ 96,  REG_PIN_SD1_D0,	0x3 },
	{ 97,  REG_PIN_SD1_D1,	0x3 },
	{ 98,  REG_PIN_SD1_D2,	0x3 },
	{ 99,  REG_PIN_SD1_D3,	0x3 },
	{ 100,  REG_PIN_LCD_CSN1,	0x3 },
	{ 101,  REG_PIN_LCD_RSTN,	0x3 },
	{ 102,  REG_PIN_LCD_CD,	0x3 },
	{ 103,  REG_PIN_LCD_D0,	0x3 },
	{ 104,  REG_PIN_LCD_D1,	0x3 },
	{ 105,  REG_PIN_LCD_D2,	0x3 },
	{ 106,  REG_PIN_LCD_D3,	0x3 },
	{ 107,  REG_PIN_LCD_D4,	0x3 },
	{ 108,  REG_PIN_LCD_D5,	0x3 },
	{ 109,  REG_PIN_LCD_D6,	0x3 },
	{ 110,  REG_PIN_LCD_D7,	0x3 },
	{ 111,  REG_PIN_LCD_D8,	0x3 },
	{ 112,  REG_PIN_LCD_WRN,	0x3 },
	{ 113,  REG_PIN_LCD_RDN,	0x3 },
	{ 114,  REG_PIN_LCD_CSN0,	0x3 },
	{ 115,  REG_PIN_LCD_D9,	0x3 },
	{ 116,  REG_PIN_LCD_D10,	0x3 },
	{ 117,  REG_PIN_LCD_D11,	0x3 },
	{ 118,  REG_PIN_LCD_D12,	0x3 },
	{ 119,  REG_PIN_LCD_D13,	0x3 },
	{ 120,  REG_PIN_LCD_D14,	0x3 },
	{ 121,  REG_PIN_LCD_D15,	0x3 },
	{ 122,  REG_PIN_LCD_D16,	0x3 },
	{ 123,  REG_PIN_LCD_D17,	0x3 },
	{ 124,  REG_PIN_LCD_D18,	0x3 },
	{ 125,  REG_PIN_LCD_D19,	0x3 },
	{ 126,  REG_PIN_LCD_D20,	0x3 },
	{ 127,  REG_PIN_LCD_D21,	0x3 },
	{ 128,  REG_PIN_LCD_D22,	0x3 },
	{ 129,  REG_PIN_LCD_D23,	0x3 },
	{ 130,  REG_PIN_LCD_FMARK,	0x3 },
	{ 131,  REG_PIN_SPI2_CSN,	0x3 },
	{ 132,  REG_PIN_SPI2_DO,	0x3 },
	{ 133,  REG_PIN_SPI2_DI,	0x3 },
	{ 134,  REG_PIN_SPI2_CLK, 0x3 },
	{ 135,  REG_PIN_GPIO135,	0x0 },
#ifdef CONFIG_MACH_LOGANU
	{ 136,  REG_PIN_GPIO136,	0x3 },
#else	
	{ 136,  REG_PIN_GPIO136,	0x0 },
#endif	
	{ 137,  REG_PIN_GPIO137,	0x0 },
	{ 138,  REG_PIN_GPIO138,	0x0 },
	{ 139,  REG_PIN_GPIO139,	0x0 },
	{ 140,  REG_PIN_GPIO140,	0x0 },
	{ 141,  REG_PIN_GPIO141,	0x0 },
	{ 142,  REG_PIN_GPIO142,	0x0 },
	{ 143,  REG_PIN_GPIO143,	0x0 },
	{ 144,  REG_PIN_GPIO144,	0x0 },
	{ 145,  REG_PIN_SCL0,	0x3  },
	{ 146,  REG_PIN_SDA0,	0x3  },
	{ 147,  REG_PIN_SCL2,	0x3  },
	{ 148,  REG_PIN_SDA2,	0x3  },
	{ 149,  REG_PIN_SCL3,	0x3  },
	{ 150,  REG_PIN_SDA3,	0x3  },
	{ 151,  REG_PIN_CLK_AUX0,	0x3  },
	{ 152,  REG_PIN_IIS0DI,	0x3  },
	{ 153,  REG_PIN_IIS0DO,	0x3  },
	{ 154,  REG_PIN_IIS0CLK,	0x3  },
	{ 155,  REG_PIN_IIS0LRCK,	0x3  },
	{ 156,  REG_PIN_SPI0_CSN,	0x3  },
	{ 157,  REG_PIN_SPI0_DO,	0x3  },
	{ 158,  REG_PIN_SPI0_DI,	0x3  },
	{ 159,  REG_PIN_SPI0_CLK,	0x3  },
	{ 160,  REG_PIN_MTDO,	0x3  },
	{ 161,  REG_PIN_MTDI,	0x3  },
	{ 162,  REG_PIN_MTCK,	0x3  },
	{ 163,  REG_PIN_MTMS,	0x3  },
	{ 164,  REG_PIN_MTRST_N,	0x3  },
	{ 165,  REG_PIN_TRACECLK,	0x3  },
	{ 166,  REG_PIN_TRACECTRL,	0x3  },
	{ 167,  REG_PIN_TRACEDAT0,	0x3  },
	{ 168,  REG_PIN_TRACEDAT1,	0x3  },
	{ 169,  REG_PIN_TRACEDAT2,	0x3  },
	{ 170,  REG_PIN_TRACEDAT3,	0x3  },
	{ 171,  REG_PIN_TRACEDAT4,	0x3  },
	{ 172,  REG_PIN_TRACEDAT5,	0x3  },
	{ 173,  REG_PIN_TRACEDAT6,	0x3  },
	{ 174,  REG_PIN_TRACEDAT7,	0x3  },
	{ 175,  REG_PIN_U0TXD,	0x3  },
	{ 176,  REG_PIN_U0RXD,	0x3  },
	{ 177,  REG_PIN_U0CTS,	0x3  },
	{ 178,  REG_PIN_U0RTS,	0x3  },
	{ 179,  REG_PIN_U1TXD,	0x3  },
	{ 180,  REG_PIN_U1RXD,	0x3  },
	{ 181,  REG_PIN_U2TXD,	0x3  },
	{ 182,  REG_PIN_U2RXD,	0x3  },
	{ 183,  REG_PIN_U2CTS,	0x3  },
	{ 184,  REG_PIN_U2RTS,	0x3  },
	{ 185,  REG_PIN_U3TXD,	0x3  },
	{ 186,  REG_PIN_U3RXD,	0x3  },
	{ 187,  REG_PIN_U3CTS,	0x3  },
	{ 188,  REG_PIN_U3RTS,	0x3  },
	{ 189,  REG_PIN_CLK_REQ1,	0x3  },
	{ 190,  REG_PIN_CLK_REQ2,	0x3  },
	{ 191,  REG_PIN_RFSDA0,	0x3  },
	{ 192,  REG_PIN_RFSCK0,	0x3  },
	{ 193,  REG_PIN_RFSEN0,	0x3  },
	{ 194,  REG_PIN_RFCTL0,	0x3  },
	{ 195,  REG_PIN_RFCTL1,	0x3  },
	{ 196,  REG_PIN_RFCTL2,	0x3  },
	{ 197,  REG_PIN_RFCTL3,	0x3  },
	{ 198,  REG_PIN_RFCTL4,	0x3  },
	{ 199,  REG_PIN_RFCTL5,	0x3  },
	{ 200,  REG_PIN_RFCTL6,	0x3  },
	{ 201,  REG_PIN_RFCTL7,	0x3  },
	{ 202,  REG_PIN_RFCTL8,	0x3  },
	{ 203,  REG_PIN_RFCTL9,	0x3  },
	{ 204,  REG_PIN_RFCTL10,	0x3  },
	{ 205,  REG_PIN_RFCTL11,	0x3  },
	{ 206,  REG_PIN_RFCTL12,	0x3  },
	{ 207,  REG_PIN_RFCTL13,	0x3  },
	{ 208,  REG_PIN_RFCTL14,	0x3  },
	{ 209,  REG_PIN_RFCTL15,	0x3  },
	{ 210,  REG_PIN_XTL_EN,	0x3  },
	{ 211,  REG_PIN_SD0_CLK,	0x3  },
	{ 212,  REG_PIN_KEYIN6,	0x3  },
	{ 213,  REG_PIN_KEYIN7,	0x3  },
	{ 214,  REG_PIN_IIS0MCK,	0x3  },
};

static DEFINE_SPINLOCK(gpio_spin_lock);
/****************************************************************/
/* Define value in accordance with
	the specification of each BB vendor. */
#define AP_GPIO_COUNT	ARRAY_SIZE(gpio_table)
/****************************************************************/

#define GET_RESULT_GPIO(a, b, c)	\
	((a<<4 & 0xF0) | (b<<1 & 0xE) | (c & 0x1))

/****************************************************************/
/* Pre-defined variables. (DO NOT CHANGE THIS!!) */
static unsigned char checkgpiomap_result[GDVS_PHONE_STATUS_MAX][AP_GPIO_COUNT];
static struct gpiomap_result gpiomap_result = {
	.init = checkgpiomap_result[PHONE_INIT],
	.sleep = checkgpiomap_result[PHONE_SLEEP]
};
/****************************************************************/

#define gpio_control_readl(off)			\
	__raw_readl(CTL_PIN_BASE + (off))

static unsigned gpio_control_read(int offset)
{
	unsigned long val, flags;

	BUG_ON(offset < 0);

	spin_lock_irqsave(&gpio_spin_lock, flags);
	val = gpio_control_readl(offset);
	spin_unlock_irqrestore(&gpio_spin_lock, flags);

	return val;
}

static void sc_check_gpio_configuration(unsigned char phonestate)
{
	struct sc8810_gpio *p = &gpio_table[0];
	int pin, cfg, status;
	u32 con_value, pin_sel;
	u8 temp_io = 0, temp_pdpu = 0, temp_lh = 0;

	pr_info("[secgpio_dvs][%s] state : %s\n", __func__,
		(phonestate == PHONE_INIT) ? "init" : "sleep");

	for (pin=0; pin < AP_GPIO_COUNT; pin++)
		checkgpiomap_result[phonestate][pin] = 0xff;

	if (phonestate == PHONE_INIT) {
		for (pin=0; pin < ARRAY_SIZE(gpio_table); pin++, p++) {
			con_value = gpio_control_read(p->offset);
			pin_sel = (con_value >> 4) & 0x3;

			if ( pin_sel == p->gpio_value ) {
				cfg = gpio_get_config(p->gpio);
				if ( cfg == 0 ) temp_io = 0x01; /* 1: Input */
				else temp_io = 0x02; /* 2: Output */
				status = gpio_request(p->gpio, NULL);
				temp_lh = gpio_get_value(p->gpio); /* 0: L, 1: H */
				if (!status)
					gpio_free(p->gpio);
			}
			else { /* FUNC */
				temp_io = 0;
				temp_lh = 0;
			}
			temp_pdpu = (con_value >> 6) & 0x3; /* 0: NP, 1: PD, 2: PU */

			checkgpiomap_result[phonestate][p->gpio-16] =
				GET_RESULT_GPIO(temp_io, temp_pdpu, temp_lh);
//			pr_info("GPIO[%d]Con Add[0x%x], Data[0x%x], GI[0x%x]\n", p->gpio, (CTL_PIN_BASE + (p->offset)), con_value, GET_RESULT_GPIO(temp_io, temp_pdpu, temp_lh));
		}
	}
	else {
		for (pin=0; pin < ARRAY_SIZE(gpio_table); pin++, p++) {
			con_value = gpio_control_read(p->offset);
			pin_sel = (con_value >> 4) & 0x3;

			if ( pin_sel == p->gpio_value ) {
#if 0	/* if use SLP_IE andOE */
				cfg = (con_value >> 0) & 0x3; /* 0: Hi-Z, 1: Output, 2: Input */
				if ( cfg == 2 ) temp_io = 0x01; /* 1: Input */
				else if (cfg == 1) temp_io = 0x02; /* 2: Output */
#else
				cfg = gpio_get_config(p->gpio);
				if ( cfg == 0 ) temp_io = 0x01; /* 1: Input */
				else temp_io = 0x02; /* 2: Output */
#endif
				status = gpio_request(p->gpio, NULL);
				temp_lh = gpio_get_value(p->gpio); /* 0: L, 1: H */
				if (!status)
					gpio_free(p->gpio);
			}
			else { /* FUNC */
				temp_io = 0;
				temp_lh = 0;
			}
			temp_pdpu = (con_value >> 2) & 0x3; /* 0: NP, 1: PD, 2: PU */

			checkgpiomap_result[phonestate][p->gpio-16] =
				GET_RESULT_GPIO(temp_io, temp_pdpu, temp_lh);
//			pr_info("GPIO[%d]Con Add[0x%x], Data[0x%x], GS[0x%x]\n", p->gpio, (CTL_PIN_BASE + (p->offset)), con_value, GET_RESULT_GPIO(temp_io, temp_pdpu, temp_lh));
		}
	}

	pr_info("[secgpio_dvs][%s]-\n", __func__);

	return;
}

/****************************************************************/
/* Define appropriate variable in accordance with
	the specification of each BB vendor */
static struct gpio_dvs sc8810_gpio_dvs = {
	.result = &gpiomap_result,
	.check_gpio_status = sc_check_gpio_configuration,
	.count = AP_GPIO_COUNT,
};
/****************************************************************/

static struct platform_device secgpio_dvs_device = {
	.name	= "secgpio_dvs",
	.id		= -1,
/****************************************************************/
/* Designate appropriate variable pointer
	in accordance with the specification of each BB vendor. */
	.dev.platform_data = &sc8810_gpio_dvs,
/****************************************************************/
};

static struct platform_device *secgpio_dvs_devices[] __initdata = {
	&secgpio_dvs_device,
};

static int __init secgpio_dvs_device_init(void)
{
	return platform_add_devices(
		secgpio_dvs_devices, ARRAY_SIZE(secgpio_dvs_devices));
}
arch_initcall(secgpio_dvs_device_init);
