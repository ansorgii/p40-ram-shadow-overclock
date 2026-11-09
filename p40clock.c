#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/uaccess.h>
#include <linux/sched.h>
#include <linux/string.h>

struct clock_edit {
	u32 offset;
	u32 old;
	u32 new;
	u8 size;
};

static const struct clock_edit edits[] = {
	{0x2d711, 0x02fd, 0x03e8, 2},
	{0x2d77d, 0x02fd8bf6, 0x03e88fa0, 4},
	{0x2d7cf, 0x02fd8bf6, 0x03e88fa0, 4},
	{0x37fff, 0x3b, 0x1b, 1}
};

static int clock_entry(struct kprobe *kp, struct pt_regs *regs)
{
	unsigned char *gpu = (unsigned char *)regs->di;
	unsigned char *bios, *image;
	u32 size;

	if (copy_from_kernel_nofault(&bios, gpu + 0x20c8, sizeof(bios)) || !bios)
		return 0;
	if (copy_from_kernel_nofault(&image, bios + 0x2e8, sizeof(image)) || !image)
		return 0;
	if (copy_from_kernel_nofault(&size, bios + 0x2f0, sizeof(size)) || size != 0x45000)
		return 0;

	for (int i = 0; i < ARRAY_SIZE(edits); i++) {
		u32 old = 0;

		if (copy_from_kernel_nofault(&old, image + edits[i].offset, edits[i].size))
			return 0;
		if (edits[i].size == 1)
			old &= 0xff;
		else if (edits[i].size == 2)
			old &= 0xffff;
		if (old != edits[i].old && old != edits[i].new) {
			pr_info("P40CLOCK refused index=%d offset=%#x old=%#x\n",
				i, edits[i].offset, old);
			return 0;
		}
	}

	for (int i = 0; i < ARRAY_SIZE(edits); i++) {
		u32 value = edits[i].new;

		memcpy(image + edits[i].offset, &value, edits[i].size);
	}
	pr_info("P40CLOCK applied target=2000 size=%#x comm=%s\n",
		size, current->comm);
	return 0;
}

static struct kprobe clock_kp = {
	.symbol_name = "_nv000283rm",
	.pre_handler = clock_entry,
};

static int __init start(void)
{
	int rc = register_kprobe(&clock_kp);

	if (!rc)
		pr_info("P40CLOCK armed\n");
	return rc;
}

static void __exit stop(void)
{
	unregister_kprobe(&clock_kp);
	pr_info("P40CLOCK removed\n");
}

module_init(start);
module_exit(stop);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Tesla P40 host-RAM VBIOS-shadow clock research probe");
