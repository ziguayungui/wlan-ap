#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

static int __init example_init(void)
{
	int ret = 0;

    printk("I bear a charmed life.\n");
	return ret;
}
module_init(example_init);

static void __exit example_exit(void)
{
	printk("Out, out, brief candle.\n");
}
module_exit(example_exit);
MODULE_LICENSE("GPL");

