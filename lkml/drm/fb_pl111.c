#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/spi/spi.h>
#include <linux/printk.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_modeset_helper.h>
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

struct fb_device {
	struct device dev;
	struct drm_device ddev;
};

static struct fb_device fbdev;

struct drm_driver fb_driver = {
	.driver_features = DRIVER_GEM | DRIVER_MODESET | DRIVER_PRIME | DRIVER_ATOMIC,
	.fops = &fb_fops, 
	.release = NULL,
	DRM_GEM_CMA_VMAP_DRIVER_OPS,
	.name = "FB_DRIVER_SKUKRETI",
	.desc = "Fake DRM-FB Driver",
	.date = "20190614",
	.major = 1,
	.minor = 0,
};

static const struct drm_display_mode fb_pl111__mode = {
	        DRM_SIMPLE_MODE(240, 320, 37, 49),
};


static const struct drm_mode_config_funcs pl111_mode_config_funcs = {
        .fb_create = drm_gem_fb_create_with_dirty,
        .atomic_check = drm_atomic_helper_check,
        .atomic_commit = drm_atomic_helper_commit,
};

static const uint32_t pl111_formats[] = {
        DRM_FORMAT_RGB565,
        DRM_FORMAT_XRGB8888,
};


static int __init fb_init(void) {
	
	devm_drm_dev_init(&fbdev.dev, &fbdev.ddev, &fb_driver);
	drm_mode_config_reset(&fbdev.ddev);
	drm_dev_register(&fbdev.ddev, 0);
	drm_fbdev_generic_setup(&fbdev.ddev, 0);
	return 0;



}

static void fb_exit(void) {


}

module_init(fb_init);
module_exit(fb_exit);
MODULE_LICENSE("GPL");
