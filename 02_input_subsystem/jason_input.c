#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/input.h>
#include <linux/time.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Jia");
MODULE_DESCRIPTION("A sh3001 input driver.");

struct input_dev* jason_input_dev;
struct timer_list vir_input_timer;

void vir_input_timer_handler(struct timer_list *timer)
{
    static int value = 0;
    value = value ? 0 : 1;

    input_report_key(jason_input_dev, KEY_1, value);
    input_sync(jason_input_dev);
    
    mod_timer(&vir_input_timer, msecs_to_jiffies(500) + jiffies);
}

static int __init jason_input_dev_init(void) {
    int ret;

    /* allocate memory for new input device */
    /* 1. 创建 `input_dev` 结构体变量 */
    jason_input_dev = input_allocate_device();
    if(jason_input_dev == NULL){
        pr_err("input_allocate_device err.\n");
    }

    /* 2. 初始化 `input_dev` 结构体变量 */
    jason_input_dev->name = "jason_input";
    /* 2.1 设置事件类型 */
    __set_bit(EV_KEY, jason_input_dev->evbit);
    __set_bit(EV_SYN, jason_input_dev->evbit);


    /* 2.2 设置具体类型 */
    __set_bit(KEY_1, jason_input_dev->keybit);

    /* 3. 注册input_dev结构体变量 */
    ret = input_register_device(jason_input_dev);
    if(ret < 0){
        pr_err("input_register_device err. \n");
        goto error;
    }

    pr_info("init timer.\n");
    timer_setup(&vir_input_timer, vir_input_timer_handler, 0);
    mod_timer(&vir_input_timer, msecs_to_jiffies(2000) + jiffies);

    printk(KERN_INFO "Jason input dev module loaded.\n");
    return 0;

error:
    input_free_device(jason_input_dev);
    return ret;
}

static void __exit jason_input_dev_exit(void) {
    del_timer(&vir_input_timer);
    input_unregister_device(jason_input_dev); 
    printk(KERN_INFO "Jason input dev module unloaded.\n");
}

module_init(jason_input_dev_init);
module_exit(jason_input_dev_exit);
