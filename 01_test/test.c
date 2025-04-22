#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Jia");
MODULE_DESCRIPTION("A simple test module");

static int __init test_init(void) {
    printk(KERN_INFO "Test module loaded\n");
    return 0;
}

static void __exit test_exit(void) {
    printk(KERN_INFO "Test module unloaded\n");
}

module_init(test_init);
module_exit(test_exit);