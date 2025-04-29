#include <linux/module.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

/* GPIO3_B0 引脚编号：3 * 32 + (1 * 8 + 0) = 104 */
#define GPIO_PIN 104

#define DEBOUNCE_TIME msecs_to_jiffies(80) // 80ms 去抖时间
/* 定时器间隔：1秒 */
#define TIMER_INTERVAL msecs_to_jiffies(2000)

void timer_callback(struct timer_list *t);

/* 定义定时器 */
static struct timer_list gpio_timer;

/* 定义工作 */
void test_work_func(struct work_struct *work);
struct work_struct test_work;

void test_work_func(struct work_struct *work)
{
    msleep(1000);
    printk("This is test_work.\n");
}

/**
 * 定时器回调函数
 */
void timer_callback(struct timer_list *t)
{
    pr_err("Timer interrupt triggered at jiffies=%lu\n", jiffies);

    schedule_work(&test_work);
    /* 重新设置定时器，使其周期性触发 */
    // mod_timer(&gpio_timer, jiffies + TIMER_INTERVAL);
}

// 中断处理函数
static irqreturn_t gpio_irq_handler(int irq, void *dev_id)
{
    static unsigned long last_interrupt_time;
    unsigned long current_time = jiffies;

    printk(KERN_INFO "Interrupt occurred on GPIO %d.\n", GPIO_PIN);

    if (time_before(current_time, last_interrupt_time + DEBOUNCE_TIME)) {
        pr_info("Ignoring debounce interrupt\n");
        return IRQ_HANDLED;
    }
    last_interrupt_time = current_time;
    

    return IRQ_HANDLED;
}

static int __init interrupt_init(void)
{
    int irq_num;

    printk(KERN_INFO "Initializing GPIO Interrupt Driver\n");

    // 将GPIO引脚映射到中断号
    irq_num = gpio_to_irq(GPIO_PIN);
    printk(KERN_INFO "GPIO %d mapped to IRQ %d\n", GPIO_PIN, irq_num);

    // 请求中断
    if (request_irq(irq_num, gpio_irq_handler, IRQF_TRIGGER_RISING, "gpio_irq_test", NULL) != 0){
        printk(KERN_ERR "Failed to request IRQ %d\n", irq_num);

        // 请求中断失败，释放GPIO引脚
        gpio_free(GPIO_PIN);
        return -ENODEV;
    }

    /* 初始工作 */
    INIT_WORK(&test_work, test_work_func);

    /* 初始化定时器 */
    timer_setup(&gpio_timer, timer_callback, 0);
    mod_timer(&gpio_timer, jiffies + TIMER_INTERVAL);
    pr_info("Timer initialized, interval=%lu jiffies\n", TIMER_INTERVAL);

    return 0;
}

static void __exit interrupt_exit(void)
{
    int irq_num = gpio_to_irq(GPIO_PIN);

    /* 删除定时器 */
    del_timer_sync(&gpio_timer);
    pr_info("Timer deleted\n");

    // 释放中断
    free_irq(irq_num, NULL);
    printk(KERN_INFO "GPIO Interrupt Driver exited successfully\n");
}

module_init(interrupt_init);
module_exit(interrupt_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("topeet");