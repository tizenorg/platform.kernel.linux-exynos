/*
 * dvb_videobuf2.h
 *
 * Copyright (C) 2015 Kim Geunyoung <nenggun.kim@samsung.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __DVB_VIDEOBUF2_H__
#define __DVB_VIDEOBUF2_H__

#include <media/videobuf2-core.h>
#include <media/videobuf2-vmalloc.h>

/* This enums will be replaced. */
typedef enum {
	DVB_VIDEOBUF2_QCTX_STATE_NONE,
	DVB_VIDEOBUF2_QCTX_STATE_REQUESTED,
	DVB_VIDEOBUF2_QCTX_STATE_QUERIED,
	DVB_VIDEOBUF2_QCTX_STATE_ERROR,
} qctx_state_t;

typedef enum {
	DVB_VIDEOBUF2_QCTX_TYPE_DEMUX,
	DVB_VIDEOBUF2_QCTX_TYPE_DVR,
} qctx_type_t;

struct dvb_videobuf2_buffer {
	struct vb2_buffer vb;

	struct list_head queued_entry;
	struct list_head used_entry;

	void *mem;
	unsigned int bytesused;
	unsigned int length;
};

/* each process has their own queue context */
struct dvb_videobuf2_qctx {
	struct vb2_queue queue;

	int users;
	int streaming_count;

	/* buffer size */
	unsigned int buf_size;

	/* actually, pid has tgid value in struct task_struct */
	pid_t pid;

	struct list_head queued_list;
	struct list_head used_list;

	struct list_head qctx_entry;

	struct mutex mutex;
	spinlock_t lock;

	qctx_state_t state;
	qctx_type_t type;

	void *priv;
};

struct dvb_videobuf2_devctx {
	struct dvb_videobuf2_qctx *qctx;
	struct dvb_videobuf2_buffer *buf;

	__u16 pid;
};

int dvb_videobuf2_reqbufs(struct dvb_videobuf2_qctx *qctx, void *parg);
int dvb_videobuf2_querybuf(struct dvb_videobuf2_qctx *qctx, void *parg);
int dvb_videobuf2_qbuf(struct dvb_videobuf2_qctx *qctx, void *parg);
int dvb_videobuf2_dqbuf(struct dvb_videobuf2_qctx *qctx, void *parg,
		bool nonblocking);
int dvb_videobuf2_mmap(struct dvb_videobuf2_qctx *qctx,
		struct vm_area_struct *vma);
int dvb_videobuf2_streamon(struct dvb_videobuf2_qctx *qctx);
int dvb_videobuf2_streamoff(struct dvb_videobuf2_qctx *qctx);

int dvb_videobuf2_buffer_write(struct dvb_videobuf2_devctx *devctx,
		const u8 *src, size_t len);

int dvb_videobuf2_createq(struct dvb_videobuf2_qctx **ret_qctx,
		const pid_t pid, qctx_type_t type, void *priv);
int dvb_videobuf2_removeq(struct dvb_videobuf2_qctx *qctx);
int dvb_videobuf2_init(void);

static inline bool dvb_videobuf2_is_streaming(struct dvb_videobuf2_qctx *qctx)
{
	return vb2_is_streaming(&qctx->queue);
}

#endif	/* __DVB_VIDEOBUF2_H__ */


