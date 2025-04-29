# RK3568 Driver Modules


# 1. 文件描述

| 文件夹 | 描述 |
| ---- | ---- |
| `01_test` | 最简单的驱动模板文件 |
| `02_input_subsystem` | linux 输入子系统 |
| `02_01_class_attribute` | /sys/class/ 文件夹下类属性设置示例 |
| `02_02_i2c_template` | i2c驱动最简单模板 |
| `03_sh3001` | 六轴 IMU SH3001 驱动 |
| `04_gpio_irq` | gpio 中断驱动程序 |
| `05_gpio_irq_tasklet` | 使用 tasklet 作为中断下文的 gpio 中断驱动程序 |
| `06_gpio_irq_softirq` | 使用 softirq 作为中断下文的 gpio 中断驱动程序 |



# 2. 实现现象

## 2.1 04_gpio_irq实现现象

```bash
board@linux:~/Codes/Modules/04_gpio_irq$ sudo insmod gpio_irq.ko
[  656.333363] gpio_irq: loading out-of-tree module taints kernel.
[  656.333888] Initializing GPIO Interrupt Driver
[  656.333965] GPIO 104 mapped to IRQ 116
board@linux:~/Codes/Modules/04_gpio_irq$ [  833.324363] Interrupt occurred on GPIO 104.
[  833.324438] This is irq_handler.
[  833.324515] Interrupt occurred on GPIO 104.
[  833.324541] This is irq_handler.
[  833.324587] Interrupt occurred on GPIO 104.
board@linux:~/Codes/Modules/04_gpio_irq$ sudo rmmod gpio_irq.ko
[  681.569378] GPIO Interrupt Driver exited successfully
```