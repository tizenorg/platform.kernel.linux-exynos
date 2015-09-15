/*
 * Copyright (C) 2015 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef _EXYNOS_DRM_FENCE_H_
#define _EXYNOS_DRM_FENCE_H_

#include <linux/reservation.h>

struct exynos_drm_fence {
	struct fence base;
	spinlock_t lock;
};

struct exynos_drm_resv_node {
	struct list_head head;
	struct reservation_object *obj;
};

int exynos_fence_locks(struct list_head *list, struct ww_acquire_ctx *ctx);
void exynos_fence_unlocks(struct list_head *list, struct ww_acquire_ctx *ctx);
int exynos_fence_wait(struct reservation_object *obj, bool exclusive);
void exynos_fence_signal(struct exynos_drm_fence *exynos_fence);
struct exynos_drm_fence *exynos_fence_create(unsigned int context,
					     unsigned int seqno);

#endif
