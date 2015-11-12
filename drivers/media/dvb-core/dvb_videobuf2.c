/*
 * dvb_videobuf2.c
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

#include <linux/slab.h>
#include "dvb_videobuf2.h"

static struct list_head qctx_list;
static struct dvb_videobuf2_qctx *get_qctx(const pid_t pid, qctx_type_t type)
{
	struct dvb_videobuf2_qctx *qctx;

	list_for_each_entry(qctx, &qctx_list, qctx_entry) {
		if (qctx->pid == pid && qctx->type == type)
			return qctx;
	}

	return NULL;
}

/* queue operations */
static int queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
		unsigned int *nbuffers, unsigned int *nplanes,
		unsigned int sizes[], void *alloc_ctxs[])
{
	struct dvb_videobuf2_qctx *qctx = vb2_get_drv_priv(vq);

	*nplanes = 1;
	sizes[0] = qctx->buf_size;

	return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
	/* should I call the 'vb2_set_plane_payload' function here? */

	return 0;
}

static void buffer_queue(struct vb2_buffer *vb)
{
	struct dvb_videobuf2_qctx *qctx;
	struct dvb_videobuf2_buffer *buf;
	unsigned long flags;

	qctx = vb2_get_drv_priv(vb->vb2_queue);
	buf = container_of(vb, struct dvb_videobuf2_buffer, vb);

	buf->mem = vb2_plane_vaddr(vb, 0);
	buf->bytesused = 0;
	buf->length = vb2_plane_size(vb, 0);

	spin_lock_irqsave(&qctx->lock, flags);
	list_add_tail(&buf->queued_entry, &qctx->queued_list);
	spin_unlock_irqrestore(&qctx->lock, flags);
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	return 0;
}

static void stop_streaming(struct vb2_queue *vq)
{
	struct dvb_videobuf2_qctx *qctx;
	struct dvb_videobuf2_buffer *buf, *tmp;
	unsigned long flags;

	qctx = vb2_get_drv_priv(vq);

	spin_lock_irqsave(&qctx->lock, flags);
	list_for_each_entry_safe(buf, tmp, &qctx->queued_list,
			queued_entry) {
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
		list_del(&buf->queued_entry);
	}

	list_for_each_entry_safe(buf, tmp, &qctx->used_list,
			used_entry) {
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
		list_del(&buf->used_entry);
	}
	spin_unlock_irqrestore(&qctx->lock, flags);
}

static struct vb2_ops dvb_videobuf2_qops = {
	.queue_setup = queue_setup,
	.buf_prepare = buffer_prepare,
	.buf_queue = buffer_queue,
	.start_streaming = start_streaming,
	.stop_streaming = stop_streaming,
};

int dvb_videobuf2_reqbufs(struct dvb_videobuf2_qctx *qctx, void *parg)
{
	int ret;

	mutex_lock(&qctx->mutex);
	if (qctx->state == DVB_VIDEOBUF2_QCTX_STATE_REQUESTED) {
		mutex_unlock(&qctx->mutex);
		return 0;
	}

	ret = vb2_reqbufs(&qctx->queue, parg);
	qctx->state = ret ? DVB_VIDEOBUF2_QCTX_STATE_ERROR :\
		      DVB_VIDEOBUF2_QCTX_STATE_REQUESTED;
	mutex_unlock(&qctx->mutex);

	return ret;
}

int dvb_videobuf2_querybuf(struct dvb_videobuf2_qctx *qctx, void *parg)
{
	return vb2_querybuf(&qctx->queue, parg);
}

int dvb_videobuf2_qbuf(struct dvb_videobuf2_qctx *qctx, void *parg)
{
	return vb2_qbuf(&qctx->queue, parg);
}

int dvb_videobuf2_dqbuf(struct dvb_videobuf2_qctx *qctx, void *parg,
		bool nonblocking)
{
	return vb2_dqbuf(&qctx->queue, parg, nonblocking);
}

int dvb_videobuf2_mmap(struct dvb_videobuf2_qctx *qctx, struct vm_area_struct *vma)
{
	return vb2_mmap(&qctx->queue, vma);
}

int dvb_videobuf2_streamon(struct dvb_videobuf2_qctx *qctx)
{
	int ret;

	mutex_lock(&qctx->mutex);
	ret = vb2_is_streaming(&qctx->queue) ? 0 : \
	      vb2_streamon(&qctx->queue, qctx->queue.type);
	qctx->streaming_count++;
	mutex_unlock(&qctx->mutex);

	return ret;
}

int dvb_videobuf2_streamoff(struct dvb_videobuf2_qctx *qctx)
{
	int ret;

	mutex_lock(&qctx->mutex);
	qctx->streaming_count--;
	ret = (vb2_is_streaming(&qctx->queue) && qctx->streaming_count == 0) ? \
	      vb2_streamoff(&qctx->queue, qctx->queue.type) : 0;
	mutex_unlock(&qctx->mutex);

	return ret;
}

int dvb_videobuf2_buffer_write(struct dvb_videobuf2_devctx *devctx,
		const u8 *src, size_t len)
{
	struct dvb_videobuf2_qctx *qctx;
	struct dvb_videobuf2_buffer *buf;
	unsigned long flags;

	size_t todo = len;
	size_t copied;

	qctx = devctx->qctx;

	if (!qctx)
		return 0;

	if (!vb2_is_streaming(&qctx->queue))
		return 0;

	while (todo) {
		buf = devctx->buf;

		if (!buf) {
			spin_lock_irqsave(&qctx->lock, flags);
			buf = list_first_entry_or_null(&qctx->queued_list,
					struct dvb_videobuf2_buffer, queued_entry);

			if (!buf) {
				spin_unlock_irqrestore(&qctx->lock, flags);
				return 0;
				/*
				return -EOVERFLOW;
				*/
			}

			list_add_tail(&buf->used_entry, &qctx->used_list);
			list_del(&buf->queued_entry);
			spin_unlock_irqrestore(&qctx->lock, flags);

			devctx->buf = buf;
			buf->vb.v4l2_buf.reserved = devctx->pid;
		}

		copied = min_t(size_t, todo, (buf->length - buf->bytesused));
		memcpy((u8 *)buf->mem + buf->bytesused, src + (len - todo), copied);

		buf->bytesused += copied;
		todo -= copied;

		if (buf->bytesused >= buf->length) {
			vb2_buffer_done(&buf->vb, VB2_BUF_STATE_DONE);

			spin_lock_irqsave(&qctx->lock, flags);
			list_del(&buf->used_entry);
			spin_unlock_irqrestore(&qctx->lock, flags);

			devctx->buf = NULL;
		}
	}

	return len;
}

#define DVB_VIDEOBUF2_BUFFER_SIZE (16384)
int dvb_videobuf2_createq(struct dvb_videobuf2_qctx **ret_qctx,
		const pid_t pid, qctx_type_t type, void *priv)
{
	struct dvb_videobuf2_qctx *qctx;
	struct vb2_queue *q;
	int ret;

	qctx = get_qctx(pid, type);

	if (!qctx) {
		/* make a new queue context */
		qctx = kzalloc(sizeof(struct dvb_videobuf2_qctx), GFP_KERNEL);
		if (!qctx)
			return -ENOMEM;

		q = &qctx->queue;
		q->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		q->io_modes = VB2_MMAP;
		q->drv_priv = qctx;
		q->buf_struct_size = sizeof(struct dvb_videobuf2_buffer);
		q->ops = &dvb_videobuf2_qops;
		q->mem_ops = &vb2_vmalloc_memops;
		q->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
		q->min_buffers_needed = 1;
		q->streaming_for_demux = (type == DVB_VIDEOBUF2_QCTX_TYPE_DEMUX) ?\
					 1 : 0;

		ret = vb2_queue_init(q);
		if (ret) {
			kfree(qctx);
			return ret;
		}

		qctx->buf_size = DVB_VIDEOBUF2_BUFFER_SIZE;
		qctx->pid = pid;

		INIT_LIST_HEAD(&qctx->queued_list);
		INIT_LIST_HEAD(&qctx->used_list);

		mutex_init(&qctx->mutex);
		spin_lock_init(&qctx->lock);

		qctx->state = DVB_VIDEOBUF2_QCTX_STATE_NONE;
		qctx->type = type;
		qctx->priv = priv;

		list_add_tail(&qctx->qctx_entry, &qctx_list);
	}

	qctx->users++;
	(*ret_qctx) = qctx;

	pr_info("videobuf2 queue type(%d) for pid(%d) users(+): %d\n",
			type, pid, qctx->users);

	return 0;
}

int dvb_videobuf2_removeq(struct dvb_videobuf2_qctx *qctx)
{
	qctx->users--;
	pr_info("videobuf2 queue type(%d) for pid(%d) users(-): %d\n",
			qctx->type, qctx->pid, qctx->users);

	if (qctx->users == 0) {
		vb2_queue_release(&qctx->queue);

		list_del(&qctx->qctx_entry);
		kfree(qctx);
	}

	return 0;
}

int dvb_videobuf2_init(void)
{
	INIT_LIST_HEAD(&qctx_list);

	return 0;
}

