/*
 * Copyright (C) 2011 Samsung Electronics Co.Ltd
 * Authors: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <drm/drmP.h>

#include <drm/exynos_drm.h>
#include <drm/drm_plane_helper.h>
#include "exynos_drm_drv.h"
#include "exynos_drm_crtc.h"
#include "exynos_drm_fb.h"
#include "exynos_drm_gem.h"
#include "exynos_drm_plane.h"
#include "exynos_trace.h"

static const uint32_t formats[] = {
	DRM_FORMAT_XRGB8888,
	DRM_FORMAT_ARGB8888,
	DRM_FORMAT_NV12,
};

/*
 * This function is to get X or Y size shown via screen. This needs length and
 * start position of CRTC.
 *
 *      <--- length --->
 * CRTC ----------------
 *      ^ start        ^ end
 *
 * There are six cases from a to f.
 *
 *             <----- SCREEN ----->
 *             0                 last
 *   ----------|------------------|----------
 * CRTCs
 * a -------
 *        b -------
 *        c --------------------------
 *                 d --------
 *                           e -------
 *                                  f -------
 */
static int exynos_plane_get_size(int start, unsigned length, unsigned last)
{
	int end = start + length;
	int size = 0;

	if (start <= 0) {
		if (end > 0)
			size = min_t(unsigned, end, last);
	} else if (start <= last) {
		size = min_t(unsigned, last - start, length);
	}

	return size;
}

int exynos_check_plane(struct drm_plane *plane, struct drm_framebuffer *fb)
{
	struct exynos_drm_plane *exynos_plane = to_exynos_plane(plane);
	int nr;
	int i;

	nr = exynos_drm_fb_get_buf_cnt(fb);
	for (i = 0; i < nr; i++) {
		struct exynos_drm_gem_buf *buffer = exynos_drm_fb_buffer(fb, i);

		if (!buffer) {
			DRM_DEBUG_KMS("buffer is null\n");
			return -EFAULT;
		}

		exynos_plane->dma_addr[i] = buffer->dma_addr;

		DRM_DEBUG_KMS("buffer: %d, dma_addr = 0x%lx\n",
				i, (unsigned long)exynos_plane->dma_addr[i]);
	}

	return 0;
}

void exynos_plane_mode_set(struct drm_plane *plane, struct drm_crtc *crtc,
			  struct drm_framebuffer *fb, int crtc_x, int crtc_y,
			  unsigned int crtc_w, unsigned int crtc_h,
			  uint32_t src_x, uint32_t src_y,
			  uint32_t src_w, uint32_t src_h)
{
	struct exynos_drm_plane *exynos_plane = to_exynos_plane(plane);
	struct exynos_drm_crtc *exynos_crtc = to_exynos_crtc(crtc);
	unsigned int actual_w;
	unsigned int actual_h;

	actual_w = exynos_plane_get_size(crtc_x, crtc_w, crtc->mode.hdisplay);
	actual_h = exynos_plane_get_size(crtc_y, crtc_h, crtc->mode.vdisplay);

	if (crtc_x < 0) {
		if (actual_w)
			src_x -= crtc_x;
		crtc_x = 0;
	}

	if (crtc_y < 0) {
		if (actual_h)
			src_y -= crtc_y;
		crtc_y = 0;
	}

	/* set drm framebuffer data. */
	exynos_plane->fb_x = src_x;
	exynos_plane->fb_y = src_y;
	exynos_plane->fb_width = fb->width;
	exynos_plane->fb_height = fb->height;
	exynos_plane->src_width = src_w;
	exynos_plane->src_height = src_h;
	exynos_plane->bpp = fb->bits_per_pixel;
	exynos_plane->pitch = fb->pitches[0];
	exynos_plane->pixel_format = fb->pixel_format;

	/* set plane range to be displayed. */
	exynos_plane->crtc_x = crtc_x;
	exynos_plane->crtc_y = crtc_y;
	exynos_plane->crtc_width = actual_w;
	exynos_plane->crtc_height = actual_h;

	/* set drm mode data. */
	exynos_plane->mode_width = crtc->mode.hdisplay;
	exynos_plane->mode_height = crtc->mode.vdisplay;
	exynos_plane->refresh = crtc->mode.vrefresh;
	exynos_plane->scan_flag = crtc->mode.flags;

	DRM_DEBUG_KMS("plane : offset_x/y(%d,%d), width/height(%d,%d)",
			exynos_plane->crtc_x, exynos_plane->crtc_y,
			exynos_plane->crtc_width, exynos_plane->crtc_height);

	plane->crtc = crtc;

	if (exynos_crtc->ops->win_mode_set)
		exynos_crtc->ops->win_mode_set(exynos_crtc, exynos_plane);
}

#ifdef CONFIG_DRM_DMA_SYNC
static void exynos_plane_update_cb(struct drm_reservation_cb *rcb, void *params)
{
	struct exynos_drm_plane *exynos_plane = params;
	struct exynos_drm_crtc *exynos_crtc =
					to_exynos_crtc(exynos_plane->base.crtc);

	trace_exynos_update_cb(exynos_crtc, exynos_plane);
	if (exynos_crtc->ops->win_commit)
		exynos_crtc->ops->win_commit(exynos_crtc,
					     exynos_plane->zpos);

	if (exynos_plane->pending_fence) {
		drm_fence_signal_and_put(&exynos_plane->fence);
		exynos_plane->fence = exynos_plane->pending_fence;
		exynos_plane->pending_fence = NULL;
	}

	/* TODO */
}
#endif

#ifdef CONFIG_DRM_DMA_SYNC
static int exynos_plane_fence(struct exynos_drm_plane *plane,
			      struct exynos_drm_gem_obj *obj)
{
	struct drm_crtc *crtc = plane->base.crtc;
	struct exynos_drm_crtc *exynos_crtc;
	struct reservation_object *resv;
	struct fence *fence;
	int ret;

	exynos_crtc = to_exynos_crtc(crtc);
	resv = obj->base.dma_buf->resv;

	ww_mutex_lock(&resv->lock, NULL);

	ret = reservation_object_reserve_shared(resv);
	if (ret < 0) {
		DRM_ERROR("Reserving space for shared fence failed: %d\n", ret);
		goto err_mutex;
	}

	fence = drm_sw_fence_new(plane->fence_context,
				 atomic_add_return(1, &plane->fence_seqno));
	if (IS_ERR(fence)) {
		ret = PTR_ERR(fence);
		DRM_ERROR("Failed to create fence: %d\n", ret);
		goto err_mutex;
	}

	plane->pending_fence = fence;

	trace_exynos_add_shared_fence(exynos_crtc, plane);
	reservation_object_add_shared_fence(resv, plane->pending_fence);

	if (!reservation_object_test_signaled_rcu(resv, false)) {
		drm_reservation_cb_init(&plane->rcb, exynos_plane_update_cb, plane);
		trace_exynos_cb_add(exynos_crtc, plane);
		ret = drm_reservation_cb_add(&plane->rcb, resv, false);
		if (ret < 0) {
			DRM_ERROR("Adding reservation to callback failed: %d\n", ret);
			goto err_fence;
		}

		trace_exynos_cb_done(exynos_crtc, plane);
		drm_reservation_cb_done(&plane->rcb);
	} else {
		trace_exynos_cb_fast_path(exynos_crtc, plane);
		exynos_plane_update_cb(&plane->rcb, plane);
	}

	ww_mutex_unlock(&resv->lock);

	return 0;

err_fence:
	fence_put(plane->pending_fence);
	plane->pending_fence = NULL;
err_mutex:
	ww_mutex_unlock(&resv->lock);

	return ret;
}
#endif

int
exynos_update_plane(struct drm_plane *plane, struct drm_crtc *crtc,
		     struct drm_framebuffer *fb, int crtc_x, int crtc_y,
		     unsigned int crtc_w, unsigned int crtc_h,
		     uint32_t src_x, uint32_t src_y,
		     uint32_t src_w, uint32_t src_h)
{

	struct exynos_drm_crtc *exynos_crtc = to_exynos_crtc(crtc);
	struct exynos_drm_plane *exynos_plane = to_exynos_plane(plane);
	struct exynos_drm_gem_obj *obj;
	int ret;

	ret = exynos_check_plane(plane, fb);
	if (ret < 0)
		return ret;

	exynos_plane_mode_set(plane, crtc, fb, crtc_x, crtc_y,
			      crtc_w, crtc_h, src_x >> 16, src_y >> 16,
			      src_w >> 16, src_h >> 16);

	obj = exynos_drm_fb_gem_obj(fb, 0);
	if (IS_ERR(obj)) {
		/* there should be no a such case. */
		WARN_ON(1);
		return PTR_ERR(obj);
	}

#ifdef CONFIG_DRM_DMA_SYNC
	if (!obj->base.dma_buf || !obj->base.dma_buf->resv) {
		if (exynos_crtc->ops->win_commit)
			exynos_crtc->ops->win_commit(exynos_crtc,
						     exynos_plane->zpos);
		return 0;
	}

	return exynos_plane_fence(exynos_plane, obj);
#else
	if (exynos_crtc->ops->win_commit)
		exynos_crtc->ops->win_commit(exynos_crtc, exynos_plane->zpos);

	return 0;
#endif
}

static int exynos_disable_plane(struct drm_plane *plane)
{
	struct exynos_drm_plane *exynos_plane = to_exynos_plane(plane);
	struct exynos_drm_crtc *exynos_crtc = to_exynos_crtc(plane->crtc);

	if (exynos_crtc && exynos_crtc->ops->win_disable)
		exynos_crtc->ops->win_disable(exynos_crtc,
					      exynos_plane->zpos);

	return 0;
}

static void exynos_plane_destroy(struct drm_plane *plane)
{
	struct exynos_drm_plane *exynos_plane = to_exynos_plane(plane);

	exynos_disable_plane(plane);
	drm_plane_cleanup(plane);
	kfree(exynos_plane);
}

static int exynos_plane_set_property(struct drm_plane *plane,
				     struct drm_property *property,
				     uint64_t val)
{
	struct drm_device *dev = plane->dev;
	struct exynos_drm_plane *exynos_plane = to_exynos_plane(plane);
	struct exynos_drm_private *dev_priv = dev->dev_private;

	if (property == dev_priv->plane_zpos_property) {
		exynos_plane->zpos = val;
		return 0;
	}

	return -EINVAL;
}

static struct drm_plane_funcs exynos_plane_funcs = {
	.update_plane	= exynos_update_plane,
	.disable_plane	= exynos_disable_plane,
	.destroy	= exynos_plane_destroy,
	.set_property	= exynos_plane_set_property,
};

static void exynos_plane_attach_zpos_property(struct drm_plane *plane)
{
	struct drm_device *dev = plane->dev;
	struct exynos_drm_private *dev_priv = dev->dev_private;
	struct drm_property *prop;

	prop = dev_priv->plane_zpos_property;
	if (!prop) {
		prop = drm_property_create_range(dev, 0, "zpos", 0,
						 MAX_PLANE - 1);
		if (!prop)
			return;

		dev_priv->plane_zpos_property = prop;
	}

	drm_object_attach_property(&plane->base, prop, 0);
}

struct drm_plane *exynos_plane_init(struct drm_device *dev,
				    unsigned long possible_crtcs,
				    enum drm_plane_type type)
{
	struct exynos_drm_plane *exynos_plane;
	int err;

	exynos_plane = kzalloc(sizeof(struct exynos_drm_plane), GFP_KERNEL);
	if (!exynos_plane)
		return ERR_PTR(-ENOMEM);

	err = drm_universal_plane_init(dev, &exynos_plane->base, possible_crtcs,
				       &exynos_plane_funcs, formats,
				       ARRAY_SIZE(formats), type);
	if (err) {
		DRM_ERROR("failed to initialize plane\n");
		kfree(exynos_plane);
		return ERR_PTR(err);
	}

	if (type == DRM_PLANE_TYPE_PRIMARY)
		exynos_plane->zpos = DEFAULT_ZPOS;
	else
		exynos_plane_attach_zpos_property(&exynos_plane->base);

#ifdef CONFIG_DRM_DMA_SYNC
	exynos_plane->fence_context = fence_context_alloc(1);
	atomic_set(&exynos_plane->fence_seqno, 0);
#endif

	return &exynos_plane->base;
}
