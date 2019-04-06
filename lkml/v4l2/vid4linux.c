#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <media/v4l2-device.h>
#include <media/v4l2-dev.h>
#include <linux/slab.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fh.h>
#include <media/videobuf2-v4l2.h>
#include <media/v4l2-event.h>
#include <media/v4l2-ctrls.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <media/videobuf2-vmalloc.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shobhit K");
MODULE_DESCRIPTION("Fake V4L2 Driver");


// dma_queue/wait queue
struct fake_vid_dmaq {
	struct list_head active;
	struct task_struct *kthread;
	//	wait_queue_head_t wq;
	/* counters for fps */
	int frame;
	int jiffies;
};

//Buffer for a single frame of image
struct single_frame {
	struct vb2_buffer vb;
	struct list_head list;
	struct fake_vid_fmt *fmt;

};

//parent struct
struct fake_vid_device {
	struct list_head fake_vid_list;
	struct v4l2_device v4l2dev;
	struct video_device vdev;
	struct vb2_queue vidqueue;
	struct v4l2_ctrl *brightness;
	struct v4l2_ctrl *contrast;
	struct v4l2_ctrl *saturation;
	struct v4l2_ctrl *hue;
	struct v4l2_ctrl *volume;
	struct v4l2_ctrl *boolean;
	spinlock_t slock;
	struct mutex mutex;
	struct fake_vid_fmt *fmt;
	unsigned int width, height, pixelsize;
	struct fake_vid_dmaq dmaq;

};


static int frame_generation_thread(void *data)
{
	struct fake_vid_device *dev = data;
	struct fake_vid_dmaq *dmaq = &dev->dmaq;
	struct single_frame *sbuf = NULL;
	void *vbuf = NULL;
	//fake image.
	char img[128];
	/*
	 * set_freezable();
	 * current task can be suspended and wont keep the cpu awake
	 */

	unsigned int timeout = 0;

	timeout = msecs_to_jiffies((200));
	while (1) {
		if (kthread_should_stop())
			break;

		spin_lock(&dev->slock);
		if (!list_empty(&dmaq->active)) {
			sbuf = list_entry(dmaq->active.next,
					struct single_frame, list);
			list_del(&sbuf->list);
			// once done with buffer, dequeue it.
			spin_unlock(&dev->slock);
			if (sbuf == NULL) {
				pr_err("---Fake Vid Linux SBUF is
					NULL or List is NULL\n");
				return -1;
			}
		} else {
			pr_err("------ Fake Vid Device Buffer List
				is Empty, Abort\n");
			spin_unlock(&dev->slock);
			return -1;
		}
		vbuf = vb2_plane_vaddr(&sbuf->vb, 0);
		if (vbuf == NULL) {
			pr_err("---Fake VID Linux K VADDR Not allocated\n");
			return -1;
		}
		pr_info("VBUF ADDR :%p\n", vbuf);
		sprintf(img, "Text Masquerading as RGB Data");
		memcpy(vbuf, img, 128);
		sbuf->vb.timestamp = 0x555555;
		vb2_buffer_done(&sbuf->vb, VB2_BUF_STATE_DONE);
		pr_info("----Fake Vid Linux, Memset the Buffer\n");
		schedule_timeout_interruptible(timeout);
	}

	return 0;
}


static int start_frame_generation(struct fake_vid_device *dev)
{
	struct fake_vid_dmaq *dmaq = &dev->dmaq;

	dmaq->kthread = kthread_run(frame_generation_thread, dev,
			dev->v4l2dev.name);
	if (IS_ERR(dmaq->kthread)) {
		pr_err("--------Fake Vid Device Kthread RUn Failed\n");
		return PTR_ERR(dmaq->kthread);
	}

	pr_info("-------- Fake Vid Device Kthread Started\n");
	return 0;
}


/* buffer queue mgt ops */

static int queue_setup(struct vb2_queue *vq, unsigned int *num_buffers,
	unsigned int *num_planes, unsigned int sizes[],
	struct device *alloc_devs[])
{

	pr_info("Fake Vid Linux NBuffers:: %d\n", *num_buffers);
	*num_planes = 1;
	/* fixed 640 * 480 * 3 rgb image size for testing */
	sizes[0] = 640*480*3;
	return 0;
}


static int buf_init(struct vb2_buffer *vb)
{

	pr_info("Fake Vid Device Buf Init Called for IO Operations\n
		Index Number %d\n", vb->index);
	return 0;
}

static int buf_prepare(struct vb2_buffer *vb)
{

	struct fake_vid_device *dev = vb2_get_drv_priv(vb->vb2_queue);
	struct single_frame *sbuf = container_of(vb, struct single_frame, vb);

	unsigned int size = dev->width * dev->height * dev->pixelsize;

	if (vb2_plane_size(vb, 0) < size) {
		pr_err(" Fake Vid Linux, Buffer Size !=Image Size\n");
		return -ENOMEM;
	}
	pr_info("--- Buffer Prepare Sz: %u\n", size);
	vb2_set_plane_payload(&sbuf->vb, 0, size);
	sbuf->fmt = dev->fmt;
	return 0;
}

static void buf_queue(struct vb2_buffer *vb)
{
	struct fake_vid_device *dev = vb2_get_drv_priv(vb->vb2_queue);
	struct single_frame *sbuf = container_of(vb, struct single_frame, vb);
	struct fake_vid_dmaq *dmaq = &dev->dmaq;

	pr_info("---- Fake Vid Device QBUF\n");

	spin_lock(&dev->slock);
	list_add_tail(&sbuf->list, &dmaq->active);
	if (list_empty(&dmaq->active))
		pr_err("-----Fake Vid Linux, QBUF list operation failed\n");
	spin_unlock(&dev->slock);
}


static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
	int ret = 0;
	struct fake_vid_device *dev = vb2_get_drv_priv(vq);

	pr_info("------ Fake Vid Linux Start Streaming\n");
	ret = start_frame_generation(dev);

	return ret;
}

static void stop_streaming(struct vb2_queue *vq)
{
	struct fake_vid_device *dev = vb2_get_drv_priv(vq);
	struct fake_vid_dmaq *dmaq = &dev->dmaq;
	struct single_frame *sbuf = NULL;

	if (dmaq->kthread) {
		kthread_stop(dmaq->kthread);
		dmaq->kthread = NULL;

	}
	while (!list_empty(&dmaq->active)) {
		sbuf = list_entry(dmaq->active.next, struct single_frame, list);
		list_del(&sbuf->list);
		vb2_buffer_done(&sbuf->vb, VB2_BUF_STATE_ERROR);

	}

pr_info("Stopping Kthread\n");
}

static struct vb2_ops fake_video_ops = {
	.queue_setup		= &queue_setup,
	.buf_prepare		= &buf_prepare,
	.buf_queue		= &buf_queue,
	.start_streaming	= &start_streaming,
	.stop_streaming		= &stop_streaming,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
	.buf_init		= &buf_init
};

struct fake_vid_fmt {
	char *name;
	u32 fourcc;
	int depth;
};

static struct fake_vid_fmt formats[] = {
	{
		.name = "RGB24",
		.fourcc = V4L2_PIX_FMT_RGB24,
		.depth = 24,
	},

	{
		.name = "RGB555",
		.fourcc = V4L2_PIX_FMT_RGB555,
		.depth = 16,

	},

};


//char driver fops
static const struct v4l2_file_operations vid4linux_fops = {
	.owner = THIS_MODULE,
	/* v4l2 file operation helpers from videobuf2-core.h */
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.read = vb2_fop_read,
	.poll = vb2_fop_poll,
	.mmap = vb2_fop_mmap,
	.unlocked_ioctl = video_ioctl2,
};

static LIST_HEAD(fake_vid_list);
static int vidioc_querycap(struct file *file, void  *priv,
		struct v4l2_capability *cap)
{
	struct fake_vid_device *dev = video_drvdata(file);

	pr_info("---Fake Vid Device Query Cap\n");
	strcpy(cap->driver, "fake_vid_device");
	strcpy(cap->card, "fake_vid_device");
	strlcpy(cap->bus_info, dev->v4l2dev.name, sizeof(cap->bus_info));
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
		V4L2_CAP_READWRITE;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

// get video format enum
static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
		struct v4l2_fmtdesc *f)
{
	struct fake_vid_fmt *fmt;

	if (f->index >= ARRAY_SIZE(formats))
		return -EINVAL;

	fmt = &formats[f->index];

	strlcpy(f->description, fmt->name, sizeof(f->description));
	f->pixelformat = fmt->fourcc;
	return 0;
}

//get video format height width
static int vidioc_g_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	//struct file contains dentry for struct video_device
	// private member of video_device is struct fake_vid_device
	struct fake_vid_device *dev = video_drvdata(file);

	f->fmt.pix.width        = dev->width;
	f->fmt.pix.height       = dev->height;
	f->fmt.pix.field        = V4L2_FIELD_INTERLACED;
	f->fmt.pix.pixelformat  = dev->fmt->fourcc;
	f->fmt.pix.bytesperline =
		(f->fmt.pix.width * dev->fmt->depth) >> 3;
	f->fmt.pix.sizeimage =
		f->fmt.pix.height * f->fmt.pix.bytesperline;
	if (dev->fmt->fourcc == V4L2_PIX_FMT_YUYV ||
			dev->fmt->fourcc == V4L2_PIX_FMT_UYVY)
		f->fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;
	else
		f->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
	return 0;
}
static int vidioc_s_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	pr_info("-----Fake Vid Device VIDIOC_S_FMT_VID_CAP");


	return -1;
}

//ioctl ops
static const struct v4l2_ioctl_ops fake_ioctl_ops = {
	.vidioc_querycap      = vidioc_querycap,
	.vidioc_enum_fmt_vid_cap  = vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = vidioc_g_fmt_vid_cap,
	//        .vidioc_try_fmt_vid_cap   = vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = vidioc_s_fmt_vid_cap,
	//        .vidioc_querystd      = vidioc_querystd,
	//        .vidioc_g_std         = vidioc_g_std,
	//        .vidioc_s_std         = vidioc_s_std,
	//        .vidioc_enum_input    = vidioc_enum_input,
	//        .vidioc_g_input       = vidioc_g_input,
	//        .vidioc_s_input       = vidioc_s_input,

	/* vb2 takes care of these, Helper functions
	 * are already provided in videobuf2-core.h
	 */
	.vidioc_reqbufs       = vb2_ioctl_reqbufs,
	.vidioc_querybuf      = vb2_ioctl_querybuf,
	.vidioc_qbuf          = vb2_ioctl_qbuf,
	.vidioc_dqbuf         = vb2_ioctl_dqbuf,
	.vidioc_streamon      = vb2_ioctl_streamon,
	.vidioc_streamoff     = vb2_ioctl_streamoff,
	.vidioc_expbuf        = vb2_ioctl_expbuf,

	.vidioc_log_status  = v4l2_ctrl_log_status,
	.vidioc_subscribe_event = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,

#ifdef CONFIG_VIDEO_ADV_DEBUG
	.vidioc_g_register = vidioc_g_register,
	.vidioc_s_register = vidioc_s_register,
#endif
};



static int __init v4l2_init(void)
{

	struct v4l2_device *v4l2dev;
	struct video_device *vdev;
	struct vb2_queue *vq;
	struct fake_vid_device *dev;

	dev = kzalloc(sizeof(struct fake_vid_device), GFP_KERNEL);
	v4l2dev = &dev->v4l2dev;
	vdev = &dev->vdev;
	vq = &dev->vidqueue;
	memset(vq, 0, sizeof(*vq));

	dev->fmt = &formats[0];
	dev->width = 640;
	dev->height = 480;
	dev->pixelsize = 3;

	vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
	vq->drv_priv = dev;
	vq->buf_struct_size = sizeof(struct single_frame);
	vq->mem_ops = &vb2_vmalloc_memops;
	vq->ops = &fake_video_ops;
	vq->timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	vq->min_buffers_needed = 1;
	vq->lock = &dev->mutex;
	vb2_queue_init(vq);
	spin_lock_init(&dev->slock);
	mutex_init(&dev->mutex);
	INIT_LIST_HEAD(&dev->dmaq.active);
	//	init_waitqueue_head(&dev->dmaq.wq);

	if (!v4l2dev)
		return -ENOMEM;

	snprintf(v4l2dev->name, sizeof(v4l2dev->name), "sk-video1");

	if (v4l2_device_register(NULL, v4l2dev) < 0) {
		pr_alert("Failed to Register V4L2 Device\n");
		return -1;
	} else
		pr_alert("Registered V4l2 Device\n");

	vdev = video_device_alloc();
	if (!vdev) {
		pr_alert("Video Device Alloc Failed\n");
		return -ENOMEM;
	} else
		pr_alert("Allocated Video Device Struct\n");

	vdev->release = video_device_release;
	vdev->v4l2_dev = v4l2dev;
	vdev->fops = &vid4linux_fops;
	vdev->ioctl_ops = &fake_ioctl_ops;
	vdev->queue = vq;
	video_set_drvdata(vdev, dev);
	memcpy(vdev->name, "Fake_Video_Device", sizeof(vdev->name));

	// need to add fops

	if (video_register_device(vdev, VFL_TYPE_GRABBER, -1) < 0) {
		pr_alert("-----Fake Vid Device Failed to Register Video Device\n");
		return -1;
	}
	list_add_tail(&dev->fake_vid_list, &fake_vid_list);

	return 0;
}

void v4l2_exit(void)
{
	struct list_head *list;
	struct fake_vid_device *dev;

	while (!list_empty(&fake_vid_list)) {
		list = fake_vid_list.next;
		list_del(list);
		//copy of container of
		// finds the parent structure from the member of the structure
		dev = list_entry(list, struct fake_vid_device, fake_vid_list);
		pr_info("Deregistering V4L2 Device\n");
		video_unregister_device(&dev->vdev);
		v4l2_device_unregister(&dev->v4l2dev);
		kfree(dev);
	}

}

module_init(v4l2_init);
module_exit(v4l2_exit);
