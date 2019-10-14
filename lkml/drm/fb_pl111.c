//#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/spi/spi.h>
#include <linux/printk.h>
#include <linux/dma-buf.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_modeset_helper.h>
#include <linux/platform_device.h>
#include <drm/drm_probe_helper.h>
#include <drm/drm_simple_kms_helper.h>
#include <drm/drm_damage_helper.h>

// #include <drm/tinydrm/mipi-dbi.h>
// #include <drm/tinydrm/tinydrm-helpers.h>
// #include <video/mipi_display.h>
//

DEFINE_DRM_GEM_CMA_FOPS(fb_fops);

/*
#define DRM_GEM_CMA_VMAP_DRIVER_OPS \
	.gem_create_object	= drm_cma_gem_create_object_default_funcs, \
	.dumb_create		= drm_gem_cma_dumb_create, \
	.prime_handle_to_fd	= drm_gem_prime_handle_to_fd, \
	.prime_fd_to_handle	= drm_gem_prime_fd_to_handle, \
	.gem_prime_import_sg_table = drm_gem_cma_prime_import_sg_table_vmap, \
	.gem_prime_mmap		= drm_gem_prime_mmap

*/

struct platform_device pdev;
struct platform_driver pdrv;

struct fb_device {
	struct drm_device ddev;
	struct drm_connector dconn;
	struct drm_encoder dencoder;
	struct drm_crtc	   dcrtc;
	struct drm_simple_display_pipe dpipe;
};

static struct fb_device fbdev;

static const struct drm_display_mode fb_pl111_mode = {
	        DRM_SIMPLE_MODE(240, 320, 37, 49),
};

static const uint32_t pl111_formats[] = {
        DRM_FORMAT_RGB565,
        DRM_FORMAT_XRGB8888,
};


static void fbdev_pipe_disable(struct drm_simple_display_pipe *pipe){
	pr_info("%s\n", __func__);
}

static void fbdev_pipe_enable(struct drm_simple_display_pipe *pipe, struct drm_crtc_state *crtc_state, struct drm_plane_state *plane_state){
	pr_info("%s\n", __func__);
}

static void fbdev_pipe_update(struct drm_simple_display_pipe *pipe, struct drm_plane_state *old_state){
	pr_info("%s\n", __func__);
}

static const struct drm_simple_display_pipe_funcs fbdev_pipe_funcs = {
        .enable         = fbdev_pipe_enable,
        .disable        = fbdev_pipe_disable,
        .update         = fbdev_pipe_update,
        .prepare_fb     = drm_gem_fb_simple_display_pipe_prepare_fb,
};



int get_modes (struct drm_connector *connector) {
	if(!connector){
		pr_err("connector is null in %s\n", __func__); 
	}
	struct drm_display_mode *mode;
	pr_info("%s 1 \n", __func__);
	mode = drm_mode_duplicate(connector->dev,&fb_pl111_mode);
	pr_info("%s 2 \n", __func__);
	//drm_mode_set_name(&mode);	
	pr_info("%s 3 \n", __func__);
	drm_mode_probed_add(connector, mode);	
	pr_info("%s 4 \n", __func__);
	//connector->display_info.width_mm = mode.width_mm;
        //connector->display_info.height_mm = mode.height_mm;
	return 1;
}

struct drm_connector_helper_funcs fbdev_conn_hfunc ={
	.get_modes = get_modes,
};

struct drm_connector_funcs fbdev_conn_func = {
	.reset = drm_atomic_helper_connector_reset,
	.destroy = drm_connector_cleanup,
	.fill_modes = drm_helper_probe_single_connector_modes,
	.atomic_duplicate_state = drm_atomic_helper_connector_duplicate_state,
	.atomic_destroy_state = drm_atomic_helper_connector_destroy_state,
};


void fb_release (struct drm_device *ptr){
	pr_info("%s\n", __func__);
}

static const struct drm_mode_config_funcs fbdev_drm_mode_config_funcs = {
        .fb_create = drm_gem_fb_create_with_dirty,
        .atomic_check = drm_atomic_helper_check,
        .atomic_commit = drm_atomic_helper_commit,
};


struct drm_driver fb_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | /*DRIVER_PRIME |*/ DRIVER_ATOMIC,
	.fops = &fb_fops, 
	.release = fb_release,
	DRM_GEM_CMA_VMAP_DRIVER_OPS,
	.name = "FB_DRIVER_SKUKRETI",
	.desc = "Fake DRM-FB Driver",
	.date = "20190614",
	.major = 1,
	.minor = 0,
};

static int pdrv_probe(struct platform_device *pdev){

	int ret;
	struct device *dev = &pdev->dev;
	if (!dev->coherent_dma_mask) {
                ret = dma_coerce_mask_and_coherent(dev, DMA_BIT_MASK(32));
                if (ret) {
                        dev_warn(dev, "Failed to set dma mask %d\n", ret);
                        return ret;
                }
        }

	ret = devm_drm_dev_init(&pdev->dev, &fbdev.ddev, &fb_driver);
	
	if(ret < 0)
		printk("%s, devm_drm_dev_init, error: %d", __func__, ret);
	drm_mode_config_init(&fbdev.ddev);
	drm_connector_helper_add(&fbdev.dconn, &fbdev_conn_hfunc);

	static const uint64_t modifiers[] = {
	                 DRM_FORMAT_MOD_LINEAR,
	                 DRM_FORMAT_MOD_INVALID
	};  
	ret = drm_connector_init(&fbdev.ddev, &fbdev.dconn, &fbdev_conn_func, DRM_MODE_CONNECTOR_Unknown); 
	if(ret < 0)
		printk("%s, drm_connector_init, error: %d", __func__, ret);
	
	ret = drm_simple_display_pipe_init(&fbdev.ddev, &fbdev.dpipe, &fbdev_pipe_funcs, pl111_formats, ARRAY_SIZE(pl111_formats),
                                           modifiers, &fbdev.dconn);	
	if(ret < 0)
		pr_err("%s, drm_simple_display_pipe_init, error: %d", __func__, ret);

	fbdev.ddev.mode_config.funcs = &fbdev_drm_mode_config_funcs;
	fbdev.ddev.mode_config.min_width = 24;
	fbdev.ddev.mode_config.max_width = 240;
        fbdev.ddev.mode_config.min_height = 37;
        fbdev.ddev.mode_config.max_height = 320;
	drm_plane_enable_fb_damage_clips(&fbdev.dpipe.plane);
	drm_mode_config_reset(&fbdev.ddev);
	ret = drm_dev_register(&fbdev.ddev, 0);
	if(ret < 0)
		printk("%s, drm_dev_register, error: %d", __func__, ret);
	
	//ret = drm_fbdev_generic_setup(&fbdev.ddev, 32);
	if(ret < 0)
		printk("%s, drm_fbdev_generic_setup, error: %d", __func__, ret);

	return ret;


}
int pdrv_remove(struct platform_device *pdev){

return 0;
}
static const struct drm_mode_config_funcs pl111_mode_config_funcs = {
        .fb_create = drm_gem_fb_create_with_dirty,
        .atomic_check = drm_atomic_helper_check,
        .atomic_commit = drm_atomic_helper_commit,
};



static int __init fb_init(void) {
	
	pdev.name = "PL111";
	platform_device_register(&pdev);
	pdrv.probe = pdrv_probe;
	pdrv.remove = pdrv_remove;
	pdrv.driver.name = "PL111";
	pdrv.driver.owner = THIS_MODULE;
	platform_driver_register(&pdrv);

	/*
	devm_drm_dev_init(&fbdev.dev, &fbdev.ddev, &fb_driver);
	drm_mode_config_reset(&fbdev.ddev);
	drm_dev_register(&fbdev.ddev, 0);
	drm_fbdev_generic_setup(&fbdev.ddev, 0);
	*/
	return 0;



}

static void fb_exit(void) {

	platform_driver_unregister(&pdrv);		
}

module_init(fb_init);
module_exit(fb_exit);
MODULE_LICENSE("GPL");
