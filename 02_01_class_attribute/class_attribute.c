#include <linux/module.h>
#include <linux/fs.h>
#include <linux/device.h>

static struct class *my_class;
static int my_value = 42;

// sysfs 属性读函数
static ssize_t value_show(struct class *cls, struct class_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", my_value);
}

// sysfs 属性写函数
static ssize_t value_store(struct class *cls, struct class_attribute *attr, const char *buf, size_t count) {
    if (kstrtoint(buf, 10, &my_value))
        return -EINVAL;
    return count;
}

// 定义类属性
static struct class_attribute my_class_attr = {
    .attr = {.name = "value", .mode = 0666},
    .show = value_show,
    .store = value_store,
};

static int __init my_init(void) {
    int ret;

    // 创建设备类
    my_class = class_create(THIS_MODULE, "my_class");
    if (IS_ERR(my_class)) {
        pr_err("Failed to create class\n");
        return PTR_ERR(my_class);
    }

    // 创建 sysfs 属性文件 /sys/class/my_class/value
    ret = class_create_file(my_class, &my_class_attr);
    if (ret) {
        pr_err("Failed to create class attribute\n");
        class_destroy(my_class);
        return ret;
    }

    pr_info("Module loaded\n");
    return 0;
}

static void __exit my_exit(void) {
    // 移除 sysfs 属性文件
    class_remove_file(my_class, &my_class_attr);
    // 销毁设备类
    class_destroy(my_class);
    pr_info("Module unloaded\n");
}

module_init(my_init);
module_exit(my_exit);
MODULE_LICENSE("GPL");