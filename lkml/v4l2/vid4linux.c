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

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shobhit K");
MODULE_DESCRIPTION("Fake V4L2 Driver");

/*
enum vb2_buffer_state {
	VB2_BUF_STATE_DEQUEUED,
	VB2_BUF_STATE_PREPARED,
	VB2_BUF_STATE_QUEUED,
	VB2_BUF_STATE_ACTIVE,
	VB2_BUF_STATE_DONE,
	VB2_BUF_STATE_ERROR,
};
*/

/* buffer mgt ops */
static struct vb2_ops fake_video_ops = {
	.queue_setup		= NULL,
	.buf_prepare		= NULL,
	.buf_queue		= NULL,
	.start_streaming	= NULL,
	.stop_streaming		= NULL,
	.wait_prepare		= NULL,
	.wait_finish		= NULL,
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


struct single_frame {
	struct vb2_buffer v4l2_buf;
//	struct vb2_queue *vb2_queue;
//	enum vb2_buffer_state state;
	struct fake_vid_fmt *fmt;

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
	struct vb2_queue fake_vid_queue;
	unsigned width, height;

};

static int vidioc_querycap(struct file *file, void  *priv,
					struct v4l2_capability *cap)
{
	struct fake_vid_device *dev = video_drvdata(file);

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

//ioctl ops
static const struct v4l2_ioctl_ops fake_ioctl_ops = {
        .vidioc_querycap      = vidioc_querycap,
        .vidioc_enum_fmt_vid_cap  = vidioc_enum_fmt_vid_cap,
        .vidioc_g_fmt_vid_cap     = vidioc_g_fmt_vid_cap,
//        .vidioc_try_fmt_vid_cap   = vidioc_try_fmt_vid_cap,
//        .vidioc_s_fmt_vid_cap     = vidioc_s_fmt_vid_cap,
//        .vidioc_querystd      = vidioc_querystd,
//        .vidioc_g_std         = vidioc_g_std,
//        .vidioc_s_std         = vidioc_s_std,
//        .vidioc_enum_input    = vidioc_enum_input,
//        .vidioc_g_input       = vidioc_g_input,
//        .vidioc_s_input       = vidioc_s_input,

        /* vb2 takes care of these, Helper functions are already provided in videobuf2-core.h*/
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

	vq->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	vq->io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
	vq->drv_priv = dev;
	vq->buf_struct_size = sizeof(struct single_frame);
	vq->mem_ops = &vb2_vmalloc_memops;
	vq->ops = &fake_video_ops;
	spin_lock_init(&dev->slock);
	mutex_init(&dev->mutex);

	if(!v4l2dev)
		return -ENOMEM;

	snprintf(v4l2dev->name, sizeof(v4l2dev->name), "sk-video1");

	if(v4l2_device_register(NULL, v4l2dev)<0){
		printk(KERN_ALERT "Failed to Register V4L2 Device\n");
		return -1;
	}
	else 
		printk(KERN_ALERT "Registered V4l2 Device\n");
	
	vdev = video_device_alloc();
	if(!vdev){
		printk(KERN_ALERT "Video Device Alloc Failed\n");
		return -ENOMEM;
	}
	else
		printk(KERN_ALERT "Allocated Video Device Struct\n");

	vdev->release = video_device_release;
	vdev->v4l2_dev = v4l2dev;
	vdev->fops = &vid4linux_fops;
	vdev->ioctl_ops = &fake_ioctl_ops;
	vdev->queue = vq;
	video_set_drvdata(vdev, dev);
	memcpy(vdev->name, "Fake_Video_Device", sizeof(vdev->name));

	// need to add fops

	if(video_register_device(vdev, VFL_TYPE_GRABBER, -1)<0) {
		printk(KERN_ALERT, "Failed to Register Video Device\n");
		return -1;
	}
	list_add_tail(&dev->fake_vid_list, &fake_vid_list);

	return 0;
}

void v4l2_exit(void){
struct list_head *list;
struct fake_vid_device *dev;

while(!list_empty(&fake_vid_list)) {
	list = fake_vid_list.next;
	list_del(list);
	//copy of container of 
	// finds the parent structure from the member of the structure
	dev = list_entry(list, struct fake_vid_device, fake_vid_list);
	printk(KERN_INFO "Deregistering V4L2 Device\n");
	video_unregister_device(&dev->vdev);
	v4l2_device_unregister(&dev->v4l2dev);
	kfree(dev);
}

}

module_init(v4l2_init);
module_exit(v4l2_exit);
