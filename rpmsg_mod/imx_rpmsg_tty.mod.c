#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x98d724a6, "module_layout" },
	{ 0xa04a0b9a, "unregister_rpmsg_driver" },
	{ 0xadfa49aa, "__register_rpmsg_driver" },
	{ 0x60cdf30, "tty_register_driver" },
	{ 0xfed9624a, "tty_port_init" },
	{ 0xf65df4be, "tty_set_operations" },
	{ 0x67b27ec1, "tty_std_termios" },
	{ 0xfb384d37, "kasprintf" },
	{ 0xf072d07c, "__tty_alloc_driver" },
	{ 0x135c2961, "devm_kmalloc" },
	{ 0xf6ffa5bc, "tty_port_install" },
	{ 0x895ed7fb, "tty_port_open" },
	{ 0xef3f5322, "tty_port_close" },
	{ 0xc5850110, "printk" },
	{ 0xd923aad0, "rpmsg_send" },
	{ 0x27c82479, "tty_port_destroy" },
	{ 0x64e3d7d2, "tty_driver_kref_put" },
	{ 0x37a0cba, "kfree" },
	{ 0x3a5c355f, "tty_unregister_driver" },
	{ 0x8bd4a5d5, "_dev_info" },
	{ 0x86332725, "__stack_chk_fail" },
	{ 0x9dc8dafe, "_dev_err" },
	{ 0x3ec80fa0, "_raw_spin_unlock_bh" },
	{ 0xa92db401, "tty_flip_buffer_push" },
	{ 0x98c13a83, "vfs_llseek" },
	{ 0xdf57eeb7, "filp_close" },
	{ 0xb44b911b, "kernel_write" },
	{ 0xbdca05ef, "filp_open" },
	{ 0xe7e4d52a, "_raw_spin_lock_bh" },
	{ 0x8f678b07, "__stack_chk_guard" },
	{ 0xefd6cf06, "__aeabi_unwind_cpp_pr0" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "28B2DEFDFEB5E03FF5CEDE4");
