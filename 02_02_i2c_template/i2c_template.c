#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/of_device.h>

// jason_sh3001_gyro 设备的初始化函数
int jason_sh3001_gyro_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    dev_info(&client->dev, "This is jason_sh3001_gyro acc probe.\n");
    return 0;
}

// jason_sh3001_gyro 设备的移除函数
int jason_sh3001_gyro_remove(struct i2c_client *client) {
    dev_info(&client->dev, "This is jason_sh3001_gyro acc remove.\n");
    return 0;
}

// 设备树匹配表
static const struct of_device_id jason_sh3001_gyro_id[] = {
    { .compatible = "jason_sh3001_gyro" },
    { }, 
};

// jason_sh3001_gyro设备驱动结构体
static struct i2c_driver jason_sh3001_gyro_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "jason_sh3001_gyro",
        .of_match_table = jason_sh3001_gyro_id, // 添加设备树匹配表
    },    
    .probe = jason_sh3001_gyro_probe,
    .remove = jason_sh3001_gyro_remove,
};

module_i2c_driver(jason_sh3001_gyro_driver);

MODULE_LICENSE("GPL");