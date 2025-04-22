#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include "jason_sh3001.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Jia");
MODULE_DESCRIPTION("A driver for sh3001 acc.");

/* Standard driver model interfaces */
static int sh3001_acc_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    pr_info("sh3001_acc driver module loaded.\n");
    return 0;
}

static int sh3001_acc_remove(struct i2c_client *client)
{
    pr_info("sh3001_acc driver module unloaded.\n");
    return 0;
}

static const struct i2c_device_id sh3001_acc_id_table[] = {
    {"jason_sh3001_acc"},
    {},
};

static struct i2c_driver sh3001_acc_driver = {
	/* Standard driver model interfaces */
	.probe = sh3001_acc_probe, 
	.remove = sh3001_acc_remove, 
    .id_table = sh3001_acc_id_table,
    .driver = {
        .name = "jason_sh3001_acc",
        .owner = THIS_MODULE,
    },
};

/* This will create the init and exit function automatically */
module_i2c_driver(sh3001_acc_driver);
