#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <media/v4l2-device.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-dev.h>
#include <media/v4l2-common.h>
#include <linux/mutex.h>
#include <linux/font.h>
#include <linux/slab.h>
#include <linux/sched.h>

#define V4L2_MOD_NAME "v4l2-skuk"

MODULE_DESCRIPTION("Fake V4L2 Driver");
MODULE_AUTHOR("Shobhit K");
MODULE_LICENSE("GPL");

struct v4l2_device v4l2dev;
struct video_device *vdev;
static int v4l2_init(void){

memcpy(v4l2dev.name, "skuk-v4l2-dev", 20);

if(v4l2_device_register(NULL, &v4l2dev)<0)
	printk(KERN_ERR "Error in Registering v4l2_device\n");

vdev = video_device_alloc();

if(vdev==NULL)
	printk(KERN_ERR, "Video Device Register Failed\n)");

vdev->v4l2_dev = &v4l2dev;
video_register_device(vdev, VFL_TYPE_GRABBER, 1);

return 0;
}


void v4l2_exit(void){
	video_unregister_device(vdev);
	v4l2_device_unregister(&v4l2dev);
}

module_init(v4l2_init);
module_exit(v4l2_exit);
