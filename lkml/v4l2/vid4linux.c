#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <media/v4l2-device.h>
#include <media/v4l2-dev.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Shobhit K");
MODULE_DESCRIPTION("Fake V4L2 Driver");


struct v4l2_device *v4l2dev;
struct video_device *vdev; 
static int __init v4l2_init(void)
{

	v4l2dev = kzalloc(sizeof(struct v4l2_device), GFP_KERNEL);

	if(v4l2_device_register(NULL, v4l2dev)<0)
		printk(KERN_ERR "Failed to Register V4L2 Device\n");
	
	vdev = video_device_alloc();
	vdev->release = video_device_release;
	if(video_register_device(vdev, VFL_TYPE_GRABBER, -1)<0)
		printk(KERN_ERR, "Failed to Register Video Device\n");

	return 0;
}

void v4l2_exit(void){
video_unregister_device(vdev);
v4l2_device_unregister(v4l2dev);


}

module_init(v4l2_init);
module_exit(v4l2_exit);
