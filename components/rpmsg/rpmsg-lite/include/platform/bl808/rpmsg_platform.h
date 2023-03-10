/*
 * Copyright (c) 2016 Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef RPMSG_PLATFORM_H_
#define RPMSG_PLATFORM_H_

#include <stdint.h>

/*
 * No need to align the VRING as defined in Linux because k32l3a6 is not intended
 * to run the Linux
 */
#ifndef VRING_ALIGN
#define VRING_ALIGN (0x1000U)
//#define VRING_ALIGN (0x10U)
#endif

/* contains pool of descriptos and two circular buffers */
#if 1
#ifndef VRING_SIZE
#define VRING_SIZE (0x4000UL)
#endif
#else
#define VRING_SIZE1 (RL_BUFFER_COUNT * sizeof(struct vring_desc))
#define VRING_SIZE2 (VRING_SIZE1 + sizeof(struct vring_avail) + (RL_BUFFER_COUNT * sizeof(uint16_t)) + sizeof(uint16_t))
#define VRING_SIZE3 ((VRING_SIZE2 + VRING_ALIGN - 1UL) & ~(VRING_ALIGN - 1UL))
#define VRING_SIZE4 (VRING_SIZE3 + sizeof(struct vring_used) + (RL_BUFFER_COUNT * sizeof(struct vring_used_elem)) + sizeof(uint16_t))
#define VRING_SIZE (((int32_t)VRING_SIZE4))
#endif

/* size of shared memory + 2*VRING size */
#define RL_VRING_OVERHEAD (2UL * VRING_SIZE)

#define RL_GET_VQ_ID(link_id, queue_id) (((queue_id)&0x1U) | (((link_id) << 1U) & 0xFFFFFFFEU))
#define RL_GET_LINK_ID(id)              (((id)&0xFFFFFFFEU) >> 1U)
#define RL_GET_Q_ID(id)                 ((id)&0x1U)

#define RL_PLATFORM_BL808_M0_LINK_ID (0U)
#define RL_PLATFORM_HIGHEST_LINK_ID  (0U)


/* platform interrupt related functions */
int32_t platform_init_interrupt(uint32_t vector_id, void *isr_data);
int32_t platform_deinit_interrupt(uint32_t vector_id);
int32_t platform_interrupt_enable(uint32_t vector_id);
int32_t platform_interrupt_disable(uint32_t vector_id);
int32_t platform_in_isr(void);
void platform_notify(uint32_t vector_id);

/* platform low-level time-delay (busy loop) */
void platform_time_delay(uint32_t num_msec);

/* platform memory functions */
void platform_map_mem_region(uint32_t vrt_addr, uint32_t phy_addr, uint32_t size, uint32_t flags);
void platform_cache_all_flush_invalidate(void);
void platform_cache_disable(void);
uint32_t platform_vatopa(void *addr);
void *platform_patova(uintptr_t addr);

/* platform init/deinit */
int32_t platform_init(void);
int32_t platform_deinit(void);

#endif /* RPMSG_PLATFORM_H_ */
