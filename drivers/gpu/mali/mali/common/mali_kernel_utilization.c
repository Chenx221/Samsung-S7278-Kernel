/*
 * Copyright (C) 2010-2012 ARM Limited. All rights reserved.
 * 
 * This program is free software and is provided to you under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation, and any use by you of this program is subject to the terms of such GNU licence.
 * 
 * A copy of the licence is included with the program, and can also be obtained from Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <linux/kernel.h>
#include "mali_kernel_utilization.h"
#include "mali_osk.h"
#include "mali_platform.h"
#include "mali_kernel_common.h"

/* Define how often to calculate and report GPU utilization, in milliseconds */
#define MALI_GPU_UTILIZATION_TIMEOUT 300

static _mali_osk_lock_t *time_data_lock;

static u64 period_start_time = 0;
static u64 work_start_time_gp = 0;
static u64 work_start_time_pp = 0;
static u64 accumulated_work_time_gp = 0;
static u64 accumulated_work_time_pp = 0;

#ifdef MALI_LOCAL_TIMER
static _mali_osk_timer_t *utilization_timer = NULL;
static mali_bool timer_running = MALI_FALSE;
#endif


u32 calculate_gpu_utilization(void* arg)
{
	u64 time_now;
	u64 time_period;
	u64 max_accumulated_work_time;
	u32 leading_zeroes;
	u32 shift_val;
	u32 work_normalized;
	u32 period_normalized;
	u32 utilization;

	if(time_data_lock == NULL) return 0;

	_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

	if( (accumulated_work_time_gp == 0 && work_start_time_gp == 0)&& (accumulated_work_time_pp == 0 && work_start_time_pp == 0))
	{
#ifdef MALI_LOCAL_TIMER
		/* Don't reschedule timer, this will be started if new work arrives */
		timer_running = MALI_FALSE;
#else
period_start_time = _mali_osk_time_get_ns(); /* starting a new period */
#endif
		_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		/* No work done for this period, report zero usage */
		mali_gpu_utilization_handler(0);

		return 0;
	}

	time_now = _mali_osk_time_get_ns();
	time_period = time_now - period_start_time;

	/* If we are currently busy, update working period up to now */
	if (work_start_time_gp!= 0)
	{
		accumulated_work_time_gp += (time_now - work_start_time_gp);
		work_start_time_gp = time_now;
	}
	if (work_start_time_pp != 0)
	{
		accumulated_work_time_pp += (time_now - work_start_time_pp);
		work_start_time_pp = time_now;
	}
	if(accumulated_work_time_pp>accumulated_work_time_gp)
		max_accumulated_work_time=accumulated_work_time_pp;
	else
		max_accumulated_work_time=accumulated_work_time_gp;
	/*
	 * We have two 64-bit values, a dividend and a divisor.
	 * To avoid dependencies to a 64-bit divider, we shift down the two values
	 * equally first.
	 * We shift the dividend up and possibly the divisor down, making the result X in 256.
	 */

	/* Shift the 64-bit values down so they fit inside a 32-bit integer */
	leading_zeroes = _mali_osk_clz((u32)(time_period >> 32));
	shift_val = 32 - leading_zeroes;
	work_normalized = (u32)(max_accumulated_work_time >> shift_val);
	period_normalized = (u32)(time_period >> shift_val);

	/*
	 * Now, we should report the usage in parts of 256
	 * this means we must shift up the dividend or down the divisor by 8
	 * (we could do a combination, but we just use one for simplicity,
	 * but the end result should be good enough anyway)
	 */
	if (period_normalized > 0x00FFFFFF)
	{
		/* The divisor is so big that it is safe to shift it down */
		period_normalized >>= 8;
	}
	else
	{
		/*
		 * The divisor is so small that we can shift up the dividend, without loosing any data.
		 * (dividend is always smaller than the divisor)
		 */
		work_normalized <<= 8;
	}

	utilization = work_normalized / period_normalized;

	accumulated_work_time_gp = 0;
	accumulated_work_time_pp = 0;
	period_start_time = time_now; /* starting a new period */

	_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);

#ifdef MALI_LOCAL_TIMER
	_mali_osk_timer_add(utilization_timer, _mali_osk_time_mstoticks(MALI_GPU_UTILIZATION_TIMEOUT));
#endif

	mali_gpu_utilization_handler(utilization);
	return utilization;
}

_mali_osk_errcode_t mali_utilization_init(void)
{
	time_data_lock = _mali_osk_lock_init(_MALI_OSK_LOCKFLAG_ORDERED | _MALI_OSK_LOCKFLAG_SPINLOCK_IRQ |
	                     _MALI_OSK_LOCKFLAG_NONINTERRUPTABLE, 0, _MALI_OSK_LOCK_ORDER_UTILIZATION);

	if (NULL == time_data_lock)
	{
		return _MALI_OSK_ERR_FAULT;
	}

#ifdef MALI_LOCAL_TIMER
	utilization_timer = _mali_osk_timer_init();
	if (NULL == utilization_timer)
	{
		_mali_osk_lock_term(time_data_lock);
		return _MALI_OSK_ERR_FAULT;
	}
	_mali_osk_timer_setcallback(utilization_timer, calculate_gpu_utilization, NULL);
#endif
	return _MALI_OSK_ERR_OK;
}

void mali_utilization_suspend(void)
{
#ifdef MALI_LOCAL_TIMER
	if (NULL != utilization_timer)
	{
		_mali_osk_timer_del(utilization_timer);
		timer_running = MALI_FALSE;
	}
#endif
}

void mali_utilization_term(void)
{
#ifdef MALI_LOCAL_TIMER
	if (NULL != utilization_timer)
	{
		_mali_osk_timer_del(utilization_timer);
		timer_running = MALI_FALSE;
		_mali_osk_timer_term(utilization_timer);
		utilization_timer = NULL;
	}
#endif

	_mali_osk_lock_term(time_data_lock);
}

void mali_utilization_core_start(u64 time_now,enum mali_core_event core_event,u32 active_gps,u32 active_pps)
{
	if((MALI_CORE_EVENT_GP_START==core_event)&&(1==active_gps))
	{
		/*
		 * We went from zero cores working, to one core working,
		 * we now consider the entire GPU for being busy
		 */

		_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		if (time_now < period_start_time)
		{
			/*
			 * This might happen if the calculate_gpu_utilization() was able
			 * to run between the sampling of time_now and us grabbing the lock above
			 */
			time_now = period_start_time;
		}
		work_start_time_gp = time_now;
#ifdef MALI_LOCAL_TIMER
		if (timer_running != MALI_TRUE)
		{
			timer_running = MALI_TRUE;
			period_start_time = work_start_time_gp; /* starting a new period */

			_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);

			_mali_osk_timer_add(utilization_timer, _mali_osk_time_mstoticks(MALI_GPU_UTILIZATION_TIMEOUT));
		}
		else
#endif
		{
			_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);
		}

	}

	else if((MALI_CORE_EVENT_PP_START==core_event)&&(1==active_pps))
	{
		/*
		 * We went from zero cores working, to one core working,
		 * we now consider the entire GPU for being busy
		 */

		_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		if (time_now < period_start_time)
		{
			/*
			 * This might happen if the calculate_gpu_utilization() was able
			 * to run between the sampling of time_now and us grabbing the lock above
			 */
			time_now = period_start_time;
		}
		work_start_time_pp = time_now;
#ifdef MALI_LOCAL_TIMER
		if (timer_running != MALI_TRUE)
		{
			timer_running = MALI_TRUE;
			period_start_time = work_start_time_pp; /* starting a new period */

			_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);

			_mali_osk_timer_add(utilization_timer, _mali_osk_time_mstoticks(MALI_GPU_UTILIZATION_TIMEOUT));
		}
		else
#endif			
		{
			_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);
		}
	}
}

void mali_utilization_core_end(u64 time_now,enum mali_core_event core_event,u32 active_gps,u32 active_pps)
{
	if((MALI_CORE_EVENT_GP_STOP==core_event)&&(0==active_gps))
	{
		/*
		 * No more cores are working, so accumulate the time we was busy.
		 */
		_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		if (time_now < work_start_time_gp)
		{
			/*
			 * This might happen if the calculate_gpu_utilization() was able
			 * to run between the sampling of time_now and us grabbing the lock above
			 */
			time_now = work_start_time_gp;
		}
		accumulated_work_time_gp+= (time_now - work_start_time_gp);
		work_start_time_gp=0;

		_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);
	}

	else if((MALI_CORE_EVENT_PP_STOP==core_event)&&(0==active_pps))
	{
		/*
		 * No more cores are working, so accumulate the time we was busy.
		 */
		_mali_osk_lock_wait(time_data_lock, _MALI_OSK_LOCKMODE_RW);

		if (time_now < work_start_time_pp)
		{
			/*
			 * This might happen if the calculate_gpu_utilization() was able
			 * to run between the sampling of time_now and us grabbing the lock above
			 */
			time_now = work_start_time_pp;
		}
		accumulated_work_time_pp+= (time_now - work_start_time_pp);
		work_start_time_pp=0;

		_mali_osk_lock_signal(time_data_lock, _MALI_OSK_LOCKMODE_RW);
	}
}
