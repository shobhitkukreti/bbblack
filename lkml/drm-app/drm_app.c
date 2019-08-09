/*
 * Reference: http://betteros.org/tut/graphics1.php#dumb
 *
 */


#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
//------------------------------------------------------------------------------
//Opening the DRI device
//------------------------------------------------------------------------------

	int dri_fd  = open("/dev/dri/card0",O_RDWR | O_CLOEXEC);
	if(dri_fd<0){
		printf("Error in Opening Card0\n");
		exit(-1);
	}


/* for ref

struct drm_mode_card_res {
	__u64 fb_id_ptr;
	__u64 crtc_id_ptr;
	__u64 connector_id_ptr;
	__u64 encoder_id_ptr;
	__u32 count_fbs;
	__u32 count_crtcs;
	__u32 count_connectors;
	__u32 count_encoders;
	__u32 min_width, max_width;
	__u32 min_height, max_height;
};

*/

//------------------------------------------------------------------------------
//Kernel Mode Setting (KMS)
//------------------------------------------------------------------------------

	uint64_t res_fb_buf[10]={0},
			res_crtc_buf[10]={0},
			res_conn_buf[10]={0},
			res_enc_buf[10]={0};

	struct drm_mode_card_res res={0};

	//Become the "master" of the DRI device for
	// KMS settings
	
	if(ioctl(dri_fd, DRM_IOCTL_SET_MASTER, 0) <0){
		printf("DRM_IOCTL_SET_MASTER, ioctl failed and returned errno %s \n",strerror(errno));
	}

	//Get resource counts (connectors)
	if(ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res) <0){
		printf("DRM_IOCTL_MODE_GETRESOURCES 1, ioctl failed and returned errno %s \n",strerror(errno));
	}
	
	printf("Res Count--> fb: %d, crtc: %d, conn: %d, enc: %d\n",res.count_fbs,res.count_crtcs,res.count_connectors,res.count_encoders);

	// pass in pointers as unsigned ints
	res.fb_id_ptr=(uint64_t)res_fb_buf;
	res.crtc_id_ptr=(uint64_t)res_crtc_buf;
	res.connector_id_ptr=(uint64_t)res_conn_buf;
	res.encoder_id_ptr=(uint64_t)res_enc_buf;
	//Get resource IDs
	
	if(ioctl(dri_fd, DRM_IOCTL_MODE_GETRESOURCES, &res) <0){
		printf("DRM_IOCTL_MODE_GETRESOURCES 2, ioctl failed and returned errno %s \n",strerror(errno));
	}
	printf("Loop through all IDs\n");
	for (int i=0;i<res.count_connectors;i++){
			printf("fb_id: %u, crtc_id: %u, conn_id: %u, encoder_id: %u\n",
			res_fb_buf[i], res_crtc_buf[i],res_conn_buf[i], res_enc_buf[i]);
	}

	void *fb_base[50]={NULL};
	long fb_w[50];
	long fb_h[50];

	/* Get COnnectors 

	struct drm_mode_get_connector {
	__u64 encoders_ptr;
	__u64 modes_ptr;
	__u64 props_ptr;
	__u64 prop_values_ptr;

	__u32 count_modes;
	__u32 count_props;
	__u32 count_encoders;

	__u32 encoder_id; // Current Encoder
	__u32 connector_id; // Id
	__u32 connector_type;
	__u32 connector_type_id;

	__u32 connection;
	__u32 mm_width, mm_height; // HxW in millimeters
	__u32 subpixel;
};

 /// drm mode_info hxw
struct drm_mode_modeinfo
{
	__u32 clock;
	__u16 hdisplay, hsync_start, hsync_end, htotal, hskew;
	__u16 vdisplay, vsync_start, vsync_end, vtotal, vscan;

	__u32 vrefresh;

	__u32 flags;
	__u32 type;
	char name[DRM_DISPLAY_MODE_LEN];
}; 

*/

	//Loop though all available connectors
	
	for (int i=0;i<res.count_connectors;i++)
	{
		struct drm_mode_modeinfo conn_mode_buf[50]={0};
		uint64_t	conn_prop_buf[50]={0},
					conn_propval_buf[50]={0},
					conn_enc_buf[50]={0};

		struct drm_mode_get_connector conn={0};

		// for a single connector id get me all it's info
		conn.connector_id=res_conn_buf[i];
		
		if (ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) <0 ){ 
			//get connector resource counts
			printf("DRM_IOCTL_MODE_GETCONNECTOR 1, Expected: %u ConnID:%u\nioctl failed and returned errno %s \n",res_conn_buf[i], conn.connector_id, strerror(errno));
			//continue;
		}
		printf("ModeCnt %u, EncCntPerConn :%u, IsConnected: %u\n", conn.count_modes, conn.count_encoders, conn.connection);
		conn.modes_ptr=(uint64_t)conn_mode_buf;
		conn.props_ptr=(uint64_t)conn_prop_buf;
		conn.prop_values_ptr=(uint64_t)conn_propval_buf;
		conn.encoders_ptr=(uint64_t)conn_enc_buf;
		
		if (ioctl(dri_fd, DRM_IOCTL_MODE_GETCONNECTOR, &conn) <0){
			//get connector resources
			printf("DRM_IOCTL_MODE_GETCONNECTOR 2, ConnID:%u\nioctl failed and returned errno %s \n",conn.connector_id, strerror(errno));
		}	

		//Check if the connector is OK to use (connected to something)
		if (conn.count_encoders<1 || conn.count_modes<1 || !conn.encoder_id || !conn.connection)
		{
			//printf("Enc ID: %u, Connection: %u\nWidth: %u, Height: %u\n", conn.encoder_id, conn.connection, conn.mm_width, conn.mm_height);
			printf("Not connected\n");
			//continue;
		}
		
		
//------------------------------------------------------------------------------
//Creating a dumb buffer
//------------------------------------------------------------------------------
		struct drm_mode_create_dumb create_dumb={0};
		struct drm_mode_map_dumb map_dumb={0};
		struct drm_mode_fb_cmd cmd_dumb={0};

		//If we create the buffer later, we can get the size of the screen first.
		//This must be a valid mode, so it's probably best to do this after we find
		//a valid crtc with modes.
		//create_dumb.width = conn_mode_buf[0].hdisplay;
		//create_dumb.height = conn_mode_buf[0].vdisplay;
		create_dumb.width = 3848;
		create_dumb.height = 1924;
		create_dumb.bpp = 32;
		create_dumb.flags = 0;
		create_dumb.pitch = 0;
		create_dumb.size = 0;
		create_dumb.handle = 0;
		if(ioctl(dri_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_dumb) <0){
     		printf("DRM_IOCTL_MODE_CREATE_DUMB, ioctl failed and returned errno %s \n",strerror(errno));
		}

		cmd_dumb.width=create_dumb.width;
		cmd_dumb.height=create_dumb.height;
		cmd_dumb.bpp=create_dumb.bpp;
		cmd_dumb.pitch=create_dumb.pitch;
		cmd_dumb.depth=24;
		cmd_dumb.handle=create_dumb.handle;
		if(ioctl(dri_fd,DRM_IOCTL_MODE_ADDFB,&cmd_dumb) < 0){
			printf("DRM_IOCTL_MODE_ADDFB, ioctl failed and returned errno %s \n",strerror(errno));	
		}

		map_dumb.handle=create_dumb.handle;
		if(ioctl(dri_fd,DRM_IOCTL_MODE_MAP_DUMB,&map_dumb) < 0){
			printf("DRM_IOCTL_MODE_MAP_DUMB, ioctl failed and returned errno %s \n",strerror(errno));	
		}

		fb_base[i] = mmap(0, create_dumb.size, PROT_READ | PROT_WRITE, MAP_SHARED, dri_fd, map_dumb.offset);
		fb_w[i]=create_dumb.width;
		fb_h[i]=create_dumb.height;

//------------------------------------------------------------------------------
//Kernel Mode Setting (KMS)
//------------------------------------------------------------------------------

		printf("Connection:%d ---> : mode: %d, prop: %d, enc: %d\n",conn.connection,conn.count_modes,conn.count_props,conn.count_encoders);
		//printf("modes: %dx%d FB: %x\n",conn_mode_buf[0].hdisplay,conn_mode_buf[0].vdisplay,fb_base[i]);

		struct drm_mode_get_encoder enc={0};

		enc.encoder_id=conn.encoder_id;
		if(ioctl(dri_fd, DRM_IOCTL_MODE_GETENCODER, &enc)<0){
			//get encoder
			printf("DRM_IOCTL_MODE_GETENCODER, ioctl failed and returned errno %s \n",strerror(errno));	

		}

		struct drm_mode_crtc crtc={0};
		crtc.crtc_id=enc.crtc_id;
		
		if(ioctl(dri_fd, DRM_IOCTL_MODE_GETCRTC, &crtc) <0){
			printf("DRM_IOCTL_MODE_GETCRTC, ioctl failed and returned errno %s \n",strerror(errno));	
		}
		
		crtc.fb_id=cmd_dumb.fb_id;
		crtc.set_connectors_ptr=(uint64_t)&res_conn_buf[i];
		crtc.count_connectors=1;
		crtc.mode=conn_mode_buf[0];
		crtc.mode_valid=1;
		if(ioctl(dri_fd, DRM_IOCTL_MODE_SETCRTC, &crtc) <0){
			printf("DRM_IOCTL_MODE_SETCRTC, ioctl failed and returned errno %s \n",strerror(errno));	
		}
	}
	printf("Scan Done\n");
	//Stop being the "master" of the DRI device
	if(ioctl(dri_fd, DRM_IOCTL_DROP_MASTER, 0) <0){
		printf("DRM_IOCTL_DROP_MASTER, ioctl failed and returned errno %s \n",strerror(errno));	
	}

	
	for (int i=0;i<500;i++){
		
		for (int j=0;j<res.count_connectors;j++){
			if(fb_base[j]==NULL)
				continue;	
			int col=(rand()%0x00ffffff)&0x00eeeeee;
			for (int height=0; height < fb_h[j]; height++)
				for (int width=0; width < fb_w[j]; width++){
					 uint64_t location=height*(fb_w[j]) + width;
					*(((uint32_t*)fb_base[j])+location) = col;
				}
		}
		usleep(100000);
	}

	return 0;
}
