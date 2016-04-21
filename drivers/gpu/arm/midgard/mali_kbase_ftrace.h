/* exynos_trace.h
 *
 * Copyright (c) 2016 Samsung Electronics Co., Ltd.
 * Authors:
 *	Inki Dae <inki.dae@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */

#if !defined(_MALI_TRACE_H_) || defined(TRACE_HEADER_MULTI_READ)
#define _MALI_TRACE_H_

#include <linux/stringify.h>
#include <linux/types.h>
#include <linux/tracepoint.h>
#include <linux/fence.h>

#undef TRACE_SYSTEM
#define TRACE_SYSTEM mali
#define TRACE_SYSTEM_STRING __stringify(TRACE_SYSTEM)
#define TRACE_INCLUDE_FILE mali_kbase_ftrace

#ifdef CONFIG_DRM_DMA_SYNC
DECLARE_EVENT_CLASS(mali_fence,
	    TP_PROTO(void *katom),

	    TP_ARGS(katom),

	    TP_STRUCT__entry(
		    __field(void *, ptr)
		    ),

	    TP_fast_assign(
		    __entry->ptr = (katom);
		    ),

	    TP_printk("katom(%p)", __entry->ptr)
);

DEFINE_EVENT(mali_fence, mali_submit_atom,
	    TP_PROTO(void *katom),

	    TP_ARGS(katom)
);

DEFINE_EVENT(mali_fence, mali_run_atom,
	    TP_PROTO(void *katom),

	    TP_ARGS(katom)
);

DEFINE_EVENT(mali_fence, mali_dep_callback,
	    TP_PROTO(void *katom),

	    TP_ARGS(katom)
);
#endif /* CONFIG_DRM_DMA_SYNC */

#endif /* _MALI_TRACE_H_ */

/* This part must be outside protection */
#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#include <trace/define_trace.h>
