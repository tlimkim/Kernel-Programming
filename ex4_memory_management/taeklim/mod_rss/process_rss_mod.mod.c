#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

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
	{ 0xbe103c69, __VMLINUX_SYMBOL_STR(module_layout) },
	{ 0xe676e1e0, __VMLINUX_SYMBOL_STR(param_ops_int) },
	{ 0xdb7305a1, __VMLINUX_SYMBOL_STR(__stack_chk_fail) },
	{ 0xff211664, __VMLINUX_SYMBOL_STR(vma_kernel_pagesize) },
	{ 0x502bd9e2, __VMLINUX_SYMBOL_STR(walk_page_vma) },
	{ 0x27e1a049, __VMLINUX_SYMBOL_STR(printk) },
	{ 0xd318253a, __VMLINUX_SYMBOL_STR(pid_task) },
	{ 0x8c3bf18, __VMLINUX_SYMBOL_STR(find_vpid) },
	{ 0x31b97d9b, __VMLINUX_SYMBOL_STR(linear_hugepage_index) },
	{ 0xc7981afc, __VMLINUX_SYMBOL_STR(__put_page) },
	{ 0x3b4f9a02, __VMLINUX_SYMBOL_STR(pmd_clear_bad) },
	{ 0x1e8faba7, __VMLINUX_SYMBOL_STR(swp_swapcount) },
	{ 0x6cb3f648, __VMLINUX_SYMBOL_STR(_vm_normal_page) },
	{ 0xb843fab5, __VMLINUX_SYMBOL_STR(follow_trans_huge_pmd) },
	{ 0x1d0eae99, __VMLINUX_SYMBOL_STR(find_get_entry) },
	{ 0xe259ae9e, __VMLINUX_SYMBOL_STR(_raw_spin_lock) },
	{ 0x7cd8d75e, __VMLINUX_SYMBOL_STR(page_offset_base) },
	{ 0x97651e6c, __VMLINUX_SYMBOL_STR(vmemmap_base) },
	{ 0xb802f94c, __VMLINUX_SYMBOL_STR(pv_mmu_ops) },
	{ 0xa1c76e0a, __VMLINUX_SYMBOL_STR(_cond_resched) },
	{ 0xce31b8a1, __VMLINUX_SYMBOL_STR(pv_lock_ops) },
	{ 0xea35f8da, __VMLINUX_SYMBOL_STR(__pmd_trans_huge_lock) },
	{ 0x6f9943f8, __VMLINUX_SYMBOL_STR(__page_mapcount) },
	{ 0xbdfb6dbb, __VMLINUX_SYMBOL_STR(__fentry__) },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "22BCA313F75357D05FB73CD");
