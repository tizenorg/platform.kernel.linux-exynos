/*
 * Copyright (C) 2015 Samsung Electronics Co.Ltd
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <drm/drmP.h>
#include "exynos_drm_fence.h"

static const char *exynos_fence_get_driver_name(struct fence *fence)
{
	return "exynos";
}

static const char *exynos_fence_get_timeline_name(struct fence *fence)
{
	return "exynos.drm";
}

static bool exynos_fence_enable_signaling(struct fence *fence)
{
	return true;
}

static const struct fence_ops exynos_fence_ops = {
	.get_driver_name = exynos_fence_get_driver_name,
	.get_timeline_name = exynos_fence_get_timeline_name,
	.enable_signaling = exynos_fence_enable_signaling,
	.signaled = NULL,
	.wait = fence_default_wait,
	.release = NULL,
};

int exynos_fence_locks(struct list_head *list, struct ww_acquire_ctx *ctx)
{
	struct exynos_drm_resv_node *node;
	struct exynos_drm_resv_node *contended = NULL;
	struct reservation_object *slow_obj = NULL;
	int ret;

	ww_acquire_init(ctx, &reservation_ww_class);

retry:
	list_for_each_entry(node, list, head) {
		if (node->obj == slow_obj) {
			slow_obj = NULL;
			continue;
		}

		ret = ww_mutex_lock(&node->obj->lock, ctx);
		if (ret < 0) {
			contended = node;
			goto err;
		}
	}

	ww_acquire_done(ctx);
	return 0;

err:
	list_for_each_entry_continue_reverse(node, list, head)
		ww_mutex_unlock(&node->obj->lock);

	if (slow_obj)
		ww_mutex_unlock(&slow_obj->lock);

	if (ret == -EDEADLK) {
		ww_mutex_lock_slow(&contended->obj->lock, ctx);
		slow_obj = contended->obj;
		goto retry;
	}

	ww_acquire_fini(ctx);
	return ret;
}

void exynos_fence_unlocks(struct list_head *list, struct ww_acquire_ctx *ctx)
{
	struct exynos_drm_resv_node *node;

	list_for_each_entry(node, list, head)
		ww_mutex_unlock(&node->obj->lock);

	ww_acquire_fini(ctx);
}

int exynos_fence_wait(struct reservation_object *obj, bool exclusive)
{
	struct reservation_object_list *fobj;
	struct fence *excl;
	int ret;
	int i;

	excl = reservation_object_get_excl(obj);

	if (excl) {
		ret = fence_wait(excl, true);
		if (ret < 0)
			return ret;
	}

	if (!exclusive)
		return 0;

	fobj = reservation_object_get_list(obj);

	for (i = 0; fobj && i < fobj->shared_count; i++) {
		struct fence *fence;

		fence = rcu_dereference_protected(fobj->shared[i],
						  reservation_object_held(obj));

		ret = fence_wait(fence, true);
		if (ret < 0)
			return ret;
	}

	return 0;
}

void exynos_fence_signal(struct exynos_drm_fence *exynos_fence)
{
	int err;

	if (exynos_fence) {
		err = fence_signal(&exynos_fence->base);
		if (err < 0)
			DRM_DEBUG("Fail signal completion of fence: %d\n", err);

		/* Fence should be freed by fence_ops->release() later */
		fence_put(&exynos_fence->base);
	}
}

struct exynos_drm_fence *exynos_fence_create(unsigned int context,
					     unsigned int seqno)
{
	struct exynos_drm_fence *exynos_fence;

	exynos_fence = kzalloc(sizeof(*exynos_fence), GFP_KERNEL);
	if (!exynos_fence)
		return ERR_PTR(-ENOMEM);

	spin_lock_init(&exynos_fence->lock);

	fence_init(&exynos_fence->base, &exynos_fence_ops, &exynos_fence->lock,
			context, seqno);

	return exynos_fence;
}
