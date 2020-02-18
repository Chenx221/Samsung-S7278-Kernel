/*
 * Copyright (C) 2010-2011 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/**
 * @file mali_platform.c
 * Platform specific Mali driver functions for a default platform
 */

#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/workqueue.h>
#include <asm/system.h>
#include <asm/io.h>

#include <mach/hardware.h>
#include <mach/regs_glb.h>
#include <mach/regs_ahb.h>
#include <mach/sci.h>
#include <mach/memfreq_ondemand.h>
#include "mali_kernel_common.h"
#include "mali_osk.h"
#include "mali_platform.h"
#include "mali_device_pause_resume.h"

#define MALI_BANDWIDTH (1600*1024*1024)

u32 calculate_gpu_utilization(void* arg);
u32 mem_flag = 0;

static unsigned int mali_memfreq_demand(struct memfreq_dbs *h)
{
	u32 bw = calculate_gpu_utilization(NULL);
	return (MALI_BANDWIDTH>>8)*bw;
}

static struct memfreq_dbs mali_memfreq_desc = {
	.level = 0,
	.memfreq_demand = mali_memfreq_demand,
};

#define GPU_GLITCH_FREE_DFS 0
#define GPU_FREQ_CONTROL	1

#define GPU_MIN_DIVISION	1
#define GPU_MAX_DIVISION	3

#define GPU_LEVEL0_MAX		250
#define GPU_LEVEL0_MIN		(GPU_LEVEL0_MAX/GPU_MAX_DIVISION)
#define GPU_LEVEL1_MAX		350
#define GPU_LEVEL1_MIN		(GPU_LEVEL1_MAX/GPU_MAX_DIVISION)
#define GPU_LEVEL2_MAX		400
#define GPU_LEVEL2_MIN		(GPU_LEVEL2_MAX/GPU_MAX_DIVISION)

extern int gpuinfo_min_freq;
extern int gpuinfo_max_freq;
extern int gpuinfo_transition_latency;
extern int scaling_min_freq;
extern int scaling_max_freq;
extern int scaling_cur_freq;

extern int gpu_level;

static struct clk* gpu_clock = NULL;
static int max_div = GPU_MAX_DIVISION;
const int min_div = GPU_MIN_DIVISION;
static int gpu_clock_div = 1;
static int old_gpu_clock_div = 1;
static int gpu_freq_select = 1;
static int old_gpu_freq_select = 1;

static int gpu_power_on = 0;
static int gpu_clock_on = 0;

#if !GPU_GLITCH_FREE_DFS
static struct workqueue_struct *gpu_dfs_workqueue = NULL;
#endif
static void gpu_change_freq_div(void);

_mali_osk_errcode_t mali_platform_init(void)
{
	gpu_clock = clk_get(NULL, "clk_gpu_axi");

	gpuinfo_min_freq=250;
	gpuinfo_max_freq=350;
	gpuinfo_transition_latency=300;
	scaling_max_freq=350;
	scaling_min_freq=116;

	MALI_DEBUG_ASSERT(gpu_clock);
	if (mem_flag == 0)
	{
		register_memfreq_ondemand (&mali_memfreq_desc);
		mem_flag = 1;
	}

	if(!gpu_power_on)
	{
		gpu_power_on = 1;
		sci_glb_clr(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
		while(sci_glb_read(REG_GLB_G3D_PWR_CTL, BITS_PD_G3D_STATUS(0x1f))) udelay(100);
		msleep(5);
	}
	if(!gpu_clock_on)
	{
		gpu_clock_on = 1;
		clk_enable(gpu_clock);
		udelay(300);
	}
#if !GPU_GLITCH_FREE_DFS
	if(gpu_dfs_workqueue == NULL)
	{
		gpu_dfs_workqueue = create_singlethread_workqueue("gpu_dfs");
	}
#endif
	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_deinit(void)
{
#if !GPU_GLITCH_FREE_DFS
	if(gpu_dfs_workqueue)
	{
		destroy_workqueue(gpu_dfs_workqueue);
		gpu_dfs_workqueue = NULL;
	}
#endif
	if(gpu_clock_on)
	{
		gpu_clock_on = 0;
		clk_disable(gpu_clock);
	}
	if(gpu_power_on)
	{
		gpu_power_on = 0;
		sci_glb_set(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
	}
	if (mem_flag == 1)
	{
		unregister_memfreq_ondemand (&mali_memfreq_desc);
		mem_flag = 0;
	}
	MALI_SUCCESS;
}

_mali_osk_errcode_t mali_platform_power_mode_change(mali_power_mode power_mode)
{
	switch(power_mode)
	{
	case MALI_POWER_MODE_ON:
		if(!gpu_power_on)
		{
			gpu_power_on = 1;
			sci_glb_clr(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
			while(sci_glb_read(REG_GLB_G3D_PWR_CTL, BITS_PD_G3D_STATUS(0x1f))) udelay(100);
			msleep(5);
		}
		if(!gpu_clock_on)
		{
			gpu_clock_on = 1;
			clk_enable(gpu_clock);
			udelay(300);
		}
		break;
	case MALI_POWER_MODE_LIGHT_SLEEP:
		if(gpu_clock_on)
		{
			gpu_clock_on = 0;
			clk_disable(gpu_clock);
		}
		break;
	case MALI_POWER_MODE_DEEP_SLEEP:
		if(gpu_clock_on)
		{
			gpu_clock_on = 0;
			clk_disable(gpu_clock);
		}
		if(gpu_power_on)
		{
			gpu_power_on = 0;
			sci_glb_set(REG_GLB_G3D_PWR_CTL, BIT_G3D_POW_FORCE_PD);
		}
		break;
	};
	MALI_SUCCESS;
}

static inline void mali_set_div(int clock_div)
{
	MALI_DEBUG_PRINT(3,("GPU_DFS clock_div %d\n",clock_div));
	sci_glb_write(REG_GLB_GEN2, BITS_CLK_GPU_AXI_DIV(clock_div-1), BITS_CLK_GPU_AXI_DIV(7));
}

static inline void mali_set_freq(u32 gpu_freq)
{
	int val=0;
	MALI_DEBUG_PRINT(3,("GPU_DFS gpu_freq %u\n",gpu_freq));
	val = sci_glb_raw_read(REG_GLB_G_PLL_CTL);
	if ((val & MASK_GPLL_N) == gpu_freq)
		goto exit;
	val = (val & ~MASK_GPLL_N) | gpu_freq;
	sci_glb_set(REG_GLB_GEN1, BIT_MPLL_CTL_WE);	/* gpll unlock */
	sci_glb_write(REG_GLB_G_PLL_CTL, val, MASK_GPLL_N);
	sci_glb_clr(REG_GLB_GEN1, BIT_MPLL_CTL_WE);
	val = sci_glb_raw_read(REG_GLB_G_PLL_CTL);
exit:
	return;
}
#if !GPU_GLITCH_FREE_DFS
static void gpu_dfs_func(struct work_struct *work);
static DECLARE_WORK(gpu_dfs_work, &gpu_dfs_func);

static void gpu_dfs_func(struct work_struct *work)
{
	gpu_change_freq_div();
}
#endif

void mali_gpu_utilization_handler(u32 utilization)
{
#if GPU_FREQ_CONTROL
	MALI_DEBUG_PRINT(3,("GPU_DFS  gpu_level:%d\n",gpu_level));
	switch(gpu_level)
	{
		case 3:
			scaling_max_freq=GPU_LEVEL1_MAX;
			scaling_min_freq=GPU_LEVEL1_MAX;
			max_div=GPU_MIN_DIVISION;
			gpu_freq_select=1;
			gpu_level=1;
			break;
		case 2:
			scaling_max_freq=GPU_LEVEL1_MAX;
			scaling_min_freq=GPU_LEVEL1_MIN;
			max_div=GPU_MIN_DIVISION;
			gpu_freq_select=1;
			gpu_level=1;
			break;
		case 0:
			if(scaling_max_freq>GPU_LEVEL0_MAX)
			{
				scaling_max_freq=GPU_LEVEL1_MAX;
				gpu_freq_select=1;
				max_div=GPU_MAX_DIVISION;
			}
			else
			{
				scaling_max_freq=GPU_LEVEL0_MAX;
				gpu_freq_select=0;
				max_div=GPU_MAX_DIVISION;
			}
			gpu_level=0;
			break;
		case 1:
		default:
			scaling_max_freq=GPU_LEVEL1_MAX;
			scaling_min_freq=GPU_LEVEL1_MIN;
			max_div=GPU_MAX_DIVISION;
			gpu_freq_select=1;
			gpu_level=0;
			break;
	}
#endif

	// if the loading ratio is greater then 90%, switch the clock to the maximum
	if(utilization >= (256*9/10))
	{
		gpu_clock_div = min_div;
	}
	else
	{
		if(utilization == 0)
		{
			utilization = 1;
		}

		// the absolute loading ratio is 1/gpu_clock_div * utilization/256
		// to keep the loading ratio above 70% at a certain level,
		// the absolute loading level is ceil(1/(1/gpu_clock_div * utilization/256 / (7/10)))
		gpu_clock_div = gpu_clock_div*(256*7/10)/utilization + 1;

		// if the 90% of max loading ratio of new level is smaller than the current loading ratio, shift up
		// 1/old_div * utilization/256 > 1/gpu_clock_div * 90%
		if(gpu_clock_div*utilization > old_gpu_clock_div*256*9/10)
			gpu_clock_div--;

		if(gpu_clock_div < min_div) gpu_clock_div = min_div;
		if(gpu_clock_div > max_div) gpu_clock_div = max_div;
	}
#if GPU_FREQ_CONTROL
	MALI_DEBUG_PRINT(3,("GPU_DFS gpu util %d: old %d-> div %d max_div:%d  old_freq %d ->new_freq %d \n", utilization,old_gpu_clock_div, gpu_clock_div,max_div,old_gpu_freq_select,gpu_freq_select));
	if((gpu_clock_div != old_gpu_clock_div)||(old_gpu_freq_select!=gpu_freq_select))
#else
	MALI_DEBUG_PRINT(3,("GPU_DFS gpu util %d: old %d-> div %d max_div:%d \n", utilization,old_gpu_clock_div, gpu_clock_div,max_div));
	if(gpu_clock_div != old_gpu_clock_div)
#endif
	{
#if !GPU_GLITCH_FREE_DFS
		if(gpu_dfs_workqueue)
			queue_work(gpu_dfs_workqueue, &gpu_dfs_work);
#else
		gpu_change_freq();
		mali_set_div(gpu_clock_div);
#endif
	}
}

void set_mali_parent_power_domain(void* dev)
{
}

#if GPU_FREQ_CONTROL
static void gpu_change_freq_div(void)
{
	mali_bool power_on;
	mali_dev_pause(&power_on);
	if(old_gpu_freq_select!=gpu_freq_select)
	{
		old_gpu_freq_select=gpu_freq_select;

		if(power_on)
		{
			clk_disable(gpu_clock);
		}
		switch(gpu_freq_select)
		{
			case 0:
				scaling_max_freq=GPU_LEVEL0_MAX;
				mali_set_freq(0x3F);
				break;
			case 2:
				scaling_max_freq=GPU_LEVEL2_MAX;
				mali_set_freq(0x64);
				break;
			case 1:
			default:
				scaling_max_freq=GPU_LEVEL1_MAX;
				mali_set_freq(0x58);
				break;
		}

		if(1!=gpu_clock_div)
		{
			old_gpu_clock_div=1;
			gpu_clock_div=1;
			mali_set_div(gpu_clock_div);
		}
		udelay(100);
		while(0x20000!=sci_glb_read(REG_GLB_GEN1,BIT_GPLL_CNT_DONE))
		{
			MALI_DEBUG_PRINT(3,("GPU_DFS BIT_GPLL_CNT_DONE  0x%x\n",sci_glb_read(REG_GLB_GEN1,BIT_GPLL_CNT_DONE)));
			udelay(100);
		}

		if(power_on)
		{
			clk_enable(gpu_clock);
		}
	}
	else
	{
		old_gpu_clock_div = gpu_clock_div;
		mali_set_div(gpu_clock_div);
		udelay(100);
	}
	scaling_cur_freq=scaling_max_freq/gpu_clock_div;
	mali_dev_resume();
}
#else
static void gpu_change_freq_div(void)
{
	mali_bool power_on;
	mali_dev_pause(&power_on);
	if(gpu_power_on)
	{
		old_gpu_clock_div = gpu_clock_div;
		mali_set_div(gpu_clock_div);
		udelay(100);
	}
	mali_dev_resume();
}
#endif
