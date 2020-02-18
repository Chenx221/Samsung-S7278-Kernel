/*
 * sound/soc/sprd/dai/vbc/sprd-vbc-pcm.h
 *
 * SpreadTrum VBC for the pcm stream.
 *
 * Copyright (C) 2012 SpreadTrum Ltd.
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
#ifndef __SPRD_VBC_PCM_H
#define __SPRD_VBC_PCM_H

#include <mach/dma.h>

#define VBC_PCM_FORMATS			SNDRV_PCM_FMTBIT_S16_LE
#define VBC_FIFO_FRAME_NUM	  	160

struct sprd_pcm_dma_params {
	char *name;		/* stream identifier */
	int channels[2];	/* channel id */
	int workmode;		/* dma work type */
	int irq_type;		/* dma interrupt type */
	struct sprd_dma_channel_desc desc;	/* dma description struct */
	u32 dev_paddr[2];	/* device physical address for DMA */
};

#endif /* __SPRD_VBC_PCM_H */
