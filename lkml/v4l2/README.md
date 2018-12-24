#Experiments in V4L2 Driver Development

V4L2 Drivers are primarily represented by a parent v4l2_dev struct.
The parent struct can contain a video_device for a camera sensor. 

The video_device struct requires file operations struct for read,write operations and another struct video ioctl ops for v4l2 ioctls for operations such as querying device capabilities, getting and setting formats, reqbuffs, qbuf,dqbuf etc. 

These ioctls internally call video queue operations such as queue_setup, buf_init, buf_prepare. 
