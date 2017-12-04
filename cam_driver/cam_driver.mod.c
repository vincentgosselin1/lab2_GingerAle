#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

__visible struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0xf4edea6c, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0x29ab8277, __VMLINUX_SYMBOL_STR(class_destroy) },
	{ 0x433a3e9e, __VMLINUX_SYMBOL_STR(device_destroy) },
	{ 0x7485e15e, __VMLINUX_SYMBOL_STR(unregister_chrdev_region) },
	{ 0xfbbd2ff7, __VMLINUX_SYMBOL_STR(cdev_del) },
	{ 0x2deae9aa, __VMLINUX_SYMBOL_STR(usb_deregister) },
	{ 0xc9f929cc, __VMLINUX_SYMBOL_STR(cdev_add) },
	{ 0xc13c3f5e, __VMLINUX_SYMBOL_STR(cdev_init) },
	{ 0x3e224927, __VMLINUX_SYMBOL_STR(device_create) },
	{ 0x662dbccf, __VMLINUX_SYMBOL_STR(__class_create) },
	{ 0x29537c9e, __VMLINUX_SYMBOL_STR(alloc_chrdev_region) },
	{ 0x20a26e80, __VMLINUX_SYMBOL_STR(usb_register_driver) },
	{ 0xe44b33e7, __VMLINUX_SYMBOL_STR(usb_set_interface) },
	{ 0x4307f807, __VMLINUX_SYMBOL_STR(usb_register_dev) },
	{ 0x701d4f6, __VMLINUX_SYMBOL_STR(usb_get_dev) },
	{ 0xedc1a25a, __VMLINUX_SYMBOL_STR(kmem_cache_alloc_trace) },
	{ 0x10380b27, __VMLINUX_SYMBOL_STR(kmalloc_caches) },
	{ 0x7da51b40, __VMLINUX_SYMBOL_STR(usb_deregister_dev) },
	{ 0x68af7749, __VMLINUX_SYMBOL_STR(usb_free_coherent) },
	{ 0x3e370c99, __VMLINUX_SYMBOL_STR(usb_kill_urb) },
	{ 0x4f8b5ddb, __VMLINUX_SYMBOL_STR(_copy_to_user) },
	{ 0x82f9fe5c, __VMLINUX_SYMBOL_STR(wait_for_completion) },
	{ 0xaf505491, __VMLINUX_SYMBOL_STR(usb_alloc_coherent) },
	{ 0xca1384bd, __VMLINUX_SYMBOL_STR(usb_alloc_urb) },
	{ 0xffc37f20, __VMLINUX_SYMBOL_STR(usb_free_urb) },
	{ 0xddf6296f, __VMLINUX_SYMBOL_STR(__init_waitqueue_head) },
	{ 0x4c4fef19, __VMLINUX_SYMBOL_STR(kernel_stack) },
	{ 0x473fe3a3, __VMLINUX_SYMBOL_STR(usb_control_msg) },
	{ 0x71b32384, __VMLINUX_SYMBOL_STR(complete) },
	{ 0x69acdf38, __VMLINUX_SYMBOL_STR(memcpy) },
	{ 0xa4f45c01, __VMLINUX_SYMBOL_STR(usb_submit_urb) },
	{ 0xdf497eac, __VMLINUX_SYMBOL_STR(usb_find_interface) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";

MODULE_ALIAS("usb:v046Dp0994d*dc*dsc*dp*ic*isc*ip*in*");
