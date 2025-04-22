#include <linux/module.h>
#include <linux/kernel.h>
#include "jason_sh3001.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Jia");
MODULE_DESCRIPTION("A sh3001 gyro driver.");

static int __init driver_init(void) {
    printk(KERN_INFO "Driver module loaded\n");
    return 0;
}

static void __exit driver_exit(void) {
    printk(KERN_INFO "Driver module unloaded\n");
}

module_init(driver_init);
module_exit(driver_exit);

