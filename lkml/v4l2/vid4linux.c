#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <media/v4l2-device.h>
#include <media/v4l2-dev.h>
#include <linux/slab.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-fh.h>
#include <media/videobuf2-v4l2.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shobhit K");
MODULE_DESCRIPTION("Fake V4L2 Driver");



static const struct v4l2_file_operations vid4linux_fops = {
        .owner = THIS_MODULE,
        .open = v4l2_fh_open,
        .release = vb2_fop_release,
        .read = vb2_fop_read,
        .poll = vb2_fop_poll,
        .mmap = vb2_fop_mmap,
        .unlocked_ioctl = video_ioctl2,
};


struct v4l2_device *v4l2dev;
struct video_device *vdev; 
static int __init v4l2_init(void)
{

	v4l2dev = kzalloc(sizeof(struct v4l2_device), GFP_KERNEL);
	if(!v4l2dev)
		return -ENOMEM;

	snprintf(v4l2dev->name, sizeof(v4l2dev->name), "sk-video1");

	if(v4l2_device_register(NULL, v4l2dev)<0){
		printk(KERN_ALERT "Failed to Register V4L2 Device\n");
		return -1;
	}

	printk(KERN_ALERT "Registered V4l2 Device\n");	
	vdev = video_device_alloc();
	printk(KERN_ALERT "Allocated Video Device Struct\n");
	if(!vdev){
		printk(KERN_ALERT "Video Device Alloc Failed\n");
		return -ENOMEM;
	}
	vdev->release = video_device_release;
	vdev->v4l2_dev = v4l2dev;
	vdev->fops = &vid4linux_fops;
//	vdev->name = "Fake_Video_Device";

	// need to add fops

	if(video_register_device(vdev, VFL_TYPE_GRABBER, 1)<0) {
		printk(KERN_ALERT, "Failed to Register Video Device\n");
		return -1;
	}

	return 0;
}

void v4l2_exit(void){
//video_unregister_device(vdev);
v4l2_device_unregister(v4l2dev);


}

module_init(v4l2_init);
module_exit(v4l2_exit);
