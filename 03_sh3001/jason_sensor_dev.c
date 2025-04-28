#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <asm/atomic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/proc_fs.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/module.h>
#include <linux/soc/rockchip/rk_vendor_storage.h>
#include <linux/regulator/consumer.h>
#include "jason_sensor_dev.h"
 
static struct class *jason_sensor_class;

/* 定义了一个数组用来存放 sensor 的私有数据，每个 sensor 都有对应的私有数据结构体 */
static struct sensor_private_data *g_sensor[SENSOR_NUM_TYPES];
static struct sensor_operate *sensor_ops[SENSOR_NUM_ID_HIGH];
static int sensor_probe_times[SENSOR_NUM_ID_HIGH];

/* 获取芯片ID */
static int sensor_get_id(struct i2c_client *client, int *value)
{
    struct sensor_private_data *sensor = (struct sensor_private_data *) i2c_get_clientdata(client);
    int result = 0;
    char temp = sensor->ops->id_reg;
    int i = 0;

    /* 读取芯片 id 寄存器 */
    if (sensor->ops->id_reg >= 0) {
        for (i = 0; i < 3; i++) {
            result = sensor_rx_data(client, &temp, 1);
            *value = temp;
            if (!result)
                break;
        }

        if (result)
            return result;

        if (*value != sensor->ops->id_data) {
            dev_err(&client->dev, "%s:id=0x%x is not 0x%x\n", __func__, *value, sensor->ops->id_data);
            result = -1;
        }
    }

    return result;
}

/**
 * 调用具体设备驱动中的 init 函数对设备进行初始化。
 */
static int sensor_initial(struct i2c_client *client)
{
    struct sensor_private_data *sensor = (struct sensor_private_data *) i2c_get_clientdata(client);
    int result = 0;

    /* register setting according to chip datasheet */
    result = sensor->ops->init(client);
    if (result < 0) {
        dev_err(&client->dev, "%s:fail to init sensor\n", __func__);
        return result;
    }

    return result;
}

/**
 * 检查传感器芯片是否可用，并对芯片进行初始化。
 */
static int sensor_chip_init(struct i2c_client *client)
{
    struct sensor_private_data *sensor = (struct sensor_private_data *) i2c_get_clientdata(client);
    struct sensor_operate *ops = sensor_ops[(int)sensor->i2c_id->driver_data];
    int result = 0;

    if (ops) {
        sensor->ops = ops;
    } else {
        dev_err(&client->dev, "%s:ops is null,sensor name is %s\n", __func__, sensor->i2c_id->name);
        result = -1;
        goto error;
    }

    if ((sensor->type != ops->type) || ((int)sensor->i2c_id->driver_data != ops->id_i2c)) {
        dev_err(&client->dev, "%s:type or id is different:type=%d,%d,id=%d,%d\n", __func__, sensor->type, ops->type, (int)sensor->i2c_id->driver_data, ops->id_i2c);
        result = -1;
        goto error;
    }

    if (!ops->init || !ops->active || !ops->report) {
        dev_err(&client->dev, "%s:error:some function is needed\n", __func__);
        result = -1;
        goto error;
    }

    result = sensor_get_id(sensor->client, &sensor->devid);
    if (result < 0) {
        dev_err(&client->dev, "%s:fail to read %s devid:0x%x\n", __func__, sensor->i2c_id->name, sensor->devid);
        result = -2;
        goto error;
    }

    dev_info(&client->dev, "%s:%s:devid=0x%x,ops=0x%p\n", __func__, sensor->i2c_id->name, sensor->devid, sensor->ops);

    result = sensor_initial(sensor->client);
    if (result < 0) {
        dev_err(&client->dev, "%s:fail to init sensor\n", __func__);
        result = -2;
        goto error;
    }
    return 0;

error:
    return result;
}

/**
 * 重新设置延迟工作队列的延迟时间
 */
static int sensor_reset_rate(struct i2c_client *client, int rate)
{
    struct sensor_private_data *sensor = (struct sensor_private_data *) i2c_get_clientdata(client);
    int result = 0;

    if (rate < 5)
        rate = 5;
    else if (rate > 200)
        rate = 200;

    dev_info(&client->dev, "set sensor poll time to %dms\n", rate);

    /* work queue is always slow, we need more quickly to match hal rate */
    if (sensor->pdata->poll_delay_ms == (rate - 4))
        return 0;

    sensor->pdata->poll_delay_ms = rate - 4;

    if (sensor->status_cur == SENSOR_ON) {
        if (!sensor->pdata->irq_enable) {
            sensor->stop_work = 1;
            cancel_delayed_work_sync(&sensor->delaywork);
        }
        sensor->ops->active(client, SENSOR_OFF, rate);
        result = sensor->ops->active(client, SENSOR_ON, rate);
        if (!sensor->pdata->irq_enable) {
            sensor->stop_work = 0;
            schedule_delayed_work(&sensor->delaywork, msecs_to_jiffies(sensor->pdata->poll_delay_ms));
        }
    }

    return result;
}

/**
 * 延迟工作函数，执行周期：sensor->pdata->poll_delay_ms
 */
static void  sensor_delaywork_func(struct work_struct *work)
{
    struct delayed_work *delaywork = container_of(work, struct delayed_work, work);
    struct sensor_private_data *sensor = container_of(delaywork, struct sensor_private_data, delaywork);
    struct i2c_client *client = sensor->client;
    int result;

    mutex_lock(&sensor->sensor_mutex);
    result = sensor->ops->report(client);
    if (result < 0)
        dev_err(&client->dev, "%s: Get data failed\n", __func__);
    mutex_unlock(&sensor->sensor_mutex);

    if ((!sensor->pdata->irq_enable) && (sensor->stop_work == 0))
        schedule_delayed_work(&sensor->delaywork, msecs_to_jiffies(sensor->pdata->poll_delay_ms));
}
 
 /*
  * This is a threaded IRQ handler so can access I2C/SPI.  Since all
  * interrupts are clear on read the IRQ line will be reasserted and
  * the physical IRQ will be handled again if another interrupt is
  * asserted while we run - in the normal course of events this is a
  * rare occurrence so we save I2C/SPI reads.  We're also assuming that
  * it's rare to get lots of interrupts firing simultaneously so try to
  * minimise I/O.
  */
 static irqreturn_t sensor_interrupt(int irq, void *dev_id)
 {
     struct sensor_private_data *sensor =
             (struct sensor_private_data *)dev_id;
     struct i2c_client *client = sensor->client;
 
     mutex_lock(&sensor->sensor_mutex);
     pm_stay_awake(&client->dev);
     if (sensor->ops->report(client) < 0)
         dev_err(&client->dev, "%s: Get data failed\n", __func__);
     pm_relax(&client->dev);
     mutex_unlock(&sensor->sensor_mutex);
 
     return IRQ_HANDLED;
 }

/**
 * 中断或延迟工作任务初始化
 */
static int sensor_irq_init(struct i2c_client *client)
{
    struct sensor_private_data *sensor =
            (struct sensor_private_data *) i2c_get_clientdata(client);
    int result = 0;
    int irq;

    if ((sensor->pdata->irq_enable || sensor->pdata->wake_enable) && (sensor->pdata->irq_flags != SENSOR_UNKNOW_DATA)) {
        if (sensor->pdata->poll_delay_ms <= 0)
            sensor->pdata->poll_delay_ms = 30;
        result = gpio_request(client->irq, sensor->i2c_id->name);
        if (result)
            dev_err(&client->dev, "%s:fail to request gpio :%d\n", __func__, client->irq);

        irq = gpio_to_irq(client->irq);
        result = devm_request_threaded_irq(&client->dev, irq, NULL, sensor_interrupt, sensor->pdata->irq_flags | IRQF_ONESHOT, sensor->ops->name, sensor);
        if (result) {
            dev_err(&client->dev, "%s:fail to request irq = %d, ret = 0x%x\n", __func__, irq, result);
            goto error;
        }

        client->irq = irq;
        disable_irq_nosync(client->irq);
        dev_info(&client->dev, "%s:use irq=%d\n", __func__, irq);
    }

    if (!sensor->pdata->irq_enable) {
        // 初始化 工作函数
        INIT_DELAYED_WORK(&sensor->delaywork, sensor_delaywork_func);
        sensor->stop_work = 1;
        if (sensor->pdata->poll_delay_ms <= 0)
            sensor->pdata->poll_delay_ms = 30;

        dev_info(&client->dev, "%s:use polling, delay=%d ms\n", __func__, sensor->pdata->poll_delay_ms);
    }

error:
    return result;
}
 
void jason_sensor_shutdown(struct i2c_client *client)
{

}
EXPORT_SYMBOL(jason_sensor_shutdown);
 
static int sensor_enable(struct sensor_private_data *sensor, int enable)
{
    int result = 0;
    struct i2c_client *client = sensor->client;

    if (enable == SENSOR_ON) {
        result = sensor->ops->active(client, 1, sensor->pdata->poll_delay_ms);
        if (result < 0) {
            dev_err(&client->dev, "%s:fail to active sensor,ret=%d\n", __func__, result);
            return result;
        }
        sensor->status_cur = SENSOR_ON;
        sensor->stop_work = 0;
        if (sensor->pdata->irq_enable)
            enable_irq(client->irq);
        else
            schedule_delayed_work(&sensor->delaywork, msecs_to_jiffies(sensor->pdata->poll_delay_ms));
        dev_info(&client->dev, "sensor on: starting poll sensor data %dms\n", sensor->pdata->poll_delay_ms);
    } else {
        sensor->stop_work = 1;
        if (sensor->pdata->irq_enable)
            disable_irq_nosync(client->irq);
        else
            cancel_delayed_work_sync(&sensor->delaywork);
        result = sensor->ops->active(client, 0, sensor->pdata->poll_delay_ms);
        if (result < 0) {
            dev_err(&client->dev, "%s:fail to disable sensor,ret=%d\n", __func__, result);
            return result;
        }
        sensor->status_cur = SENSOR_OFF;
    }

    return result;
}
 

static int gsensor_dev_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int gsensor_dev_release(struct inode *inode, struct file *file)
{
    return 0;
}
 
/* ioctl - I/O control */
static long gsensor_dev_ioctl(struct file *file,
            unsigned int cmd, unsigned long arg)
{
    struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_ACCEL];
    struct i2c_client *client = sensor->client;
    void __user *argp = (void __user *)arg;
    struct sensor_axis axis = {0};
    short rate;
    int result = 0;

    /* 如果 atomic_read(&sensor->is_factory) == 0 为真（即条件已经满足），进程不会睡眠，直接继续执行。
     * 如果条件为假，当前进程会被添加到 sensor->is_factory_ok 等待队列中，并进入可中断睡眠状态（TASK_INTERRUPTIBLE）；
     * 其他代码通过 wake_up() 或 wake_up_interruptible() 唤醒 sensor->is_factory_ok 上的等待进程，
     * 并且条件 atomic_read(&sensor->is_factory) == 0 为真。
    */
    wait_event_interruptible(sensor->is_factory_ok, (atomic_read(&sensor->is_factory) == 0));

    switch (cmd) {
    case SENSOR_ACCEL_IOCTL_START:
        mutex_lock(&sensor->operation_mutex);
        if (++sensor->start_count == 1) {
            if (sensor->status_cur == SENSOR_OFF) {
                sensor_enable(sensor, SENSOR_ON);
            }
        }
        mutex_unlock(&sensor->operation_mutex);
        break;

    case SENSOR_ACCEL_IOCTL_CLOSE:
        mutex_lock(&sensor->operation_mutex);
        if (--sensor->start_count == 0) {
            if (sensor->status_cur == SENSOR_ON) {
                sensor_enable(sensor, SENSOR_OFF);
            }
        }
        mutex_unlock(&sensor->operation_mutex);
        break;

    case SENSOR_ACCEL_IOCTL_SET_RATE:
        mutex_lock(&sensor->operation_mutex);
        result = sensor_reset_rate(client, rate);
        if (result < 0) {
            mutex_unlock(&sensor->operation_mutex);
            goto error;
        }
        mutex_unlock(&sensor->operation_mutex);
        break;

    case SENSOR_ACCEL_IOCTL_GETDATA:
        mutex_lock(&sensor->data_mutex);
        memcpy(&axis, &sensor->axis, sizeof(sensor->axis));
        mutex_unlock(&sensor->data_mutex);
        if (copy_to_user(argp, &axis, sizeof(axis))) {
            dev_err(&client->dev, "failed to copy sense data to user space.\n");
            result = -EFAULT;
            goto error;
        }
        break;

    default:
        result = -ENOTTY;
    goto error;
    }

error:
    return result;
}
 
static int compass_dev_open(struct inode *inode, struct file *file)
{
    struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_COMPASS];
    int flag = 0;

    flag = atomic_read(&sensor->flags.open_flag);
    if (!flag) {
        atomic_set(&sensor->flags.open_flag, 1);
        wake_up(&sensor->flags.open_wq);
    }

    return 0;
}
 
static int compass_dev_release(struct inode *inode, struct file *file)
{
    struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_COMPASS];
    int flag = 0;

    flag = atomic_read(&sensor->flags.open_flag);
    if (flag) {
        atomic_set(&sensor->flags.open_flag, 0);
        wake_up(&sensor->flags.open_wq);
    }

    return 0;
}

 
/* ioctl - I/O control */
static long compass_dev_ioctl(struct file *file,
            unsigned int cmd, unsigned long arg)
{
    struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_COMPASS];
    struct i2c_client *client = sensor->client;
    void __user *argp = (void __user *)arg;
    int result = 0;
    short flag;

    switch (cmd) {
    case ECS_IOCTL_APP_SET_MFLAG:
    case ECS_IOCTL_APP_SET_AFLAG:
    case ECS_IOCTL_APP_SET_MVFLAG:
        if (copy_from_user(&flag, argp, sizeof(flag)))
            return -EFAULT;
        if (flag < 0 || flag > 1)
            return -EINVAL;
        break;
    case ECS_IOCTL_APP_SET_DELAY:
        if (copy_from_user(&flag, argp, sizeof(flag)))
            return -EFAULT;
        break;
    default:
        break;
    }

    switch (cmd) {
    case ECS_IOCTL_APP_SET_MFLAG:
        atomic_set(&sensor->flags.m_flag, flag);
        break;
    case ECS_IOCTL_APP_GET_MFLAG:
        flag = atomic_read(&sensor->flags.m_flag);
        break;
    case ECS_IOCTL_APP_SET_AFLAG:
        atomic_set(&sensor->flags.a_flag, flag);
        break;
    case ECS_IOCTL_APP_GET_AFLAG:
        flag = atomic_read(&sensor->flags.a_flag);
        break;
    case ECS_IOCTL_APP_SET_MVFLAG:
        atomic_set(&sensor->flags.mv_flag, flag);
        break;
    case ECS_IOCTL_APP_GET_MVFLAG:
        flag = atomic_read(&sensor->flags.mv_flag);
        break;
    case ECS_IOCTL_APP_SET_DELAY:
        sensor->flags.delay = flag;
        mutex_lock(&sensor->operation_mutex);
        result = sensor_reset_rate(client, flag);
        if (result < 0) {
            mutex_unlock(&sensor->operation_mutex);
            return result;
        }
        mutex_unlock(&sensor->operation_mutex);
        break;
    case ECS_IOCTL_APP_GET_DELAY:
        flag = sensor->flags.delay;
        break;
    default:
        return -ENOTTY;
    }

    switch (cmd) {
    case ECS_IOCTL_APP_GET_MFLAG:
    case ECS_IOCTL_APP_GET_AFLAG:
    case ECS_IOCTL_APP_GET_MVFLAG:
    case ECS_IOCTL_APP_GET_DELAY:
        if (copy_to_user(argp, &flag, sizeof(flag)))
            return -EFAULT;
        break;
    default:
        break;
    }

    return result;
}
 
static int gyro_dev_open(struct inode *inode, struct file *file)
{
    return 0;
}


static int gyro_dev_release(struct inode *inode, struct file *file)
{
    return 0;
}
 
/* ioctl - I/O control */
static long gyro_dev_ioctl(struct file *file,
            unsigned int cmd, unsigned long arg)
{
    struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_GYROSCOPE];
    struct i2c_client *client = sensor->client;
    void __user *argp = (void __user *)arg;
    struct sensor_axis axis = {0};
    int result = 0;
    int rate;

    wait_event_interruptible(sensor->is_factory_ok, (atomic_read(&sensor->is_factory) == 0));

    switch (cmd) {
        case SENSOR_GYRO_IOCTL_START:
            mutex_lock(&sensor->operation_mutex);
            if (++sensor->start_count == 1) {
                if (sensor->status_cur == SENSOR_OFF) {
                    sensor_enable(sensor, SENSOR_ON);
                }
            }
            mutex_unlock(&sensor->operation_mutex);
            break;
    
        case SENSOR_GYRO_IOCTL_CLOSE:
            mutex_lock(&sensor->operation_mutex);
            if (--sensor->start_count == 0) {
                if (sensor->status_cur == SENSOR_ON) {
                    sensor_enable(sensor, SENSOR_OFF);
                }
            }
            mutex_unlock(&sensor->operation_mutex);
            break;
    
        case SENSOR_GYRO_IOCTL_SET_RATE:
            mutex_lock(&sensor->operation_mutex);
            result = sensor_reset_rate(client, rate);
            if (result < 0) {
                mutex_unlock(&sensor->operation_mutex);
                goto error;
            }
            mutex_unlock(&sensor->operation_mutex);
            break;
    
        case SENSOR_GYRO_IOCTL_GETDATA:
            mutex_lock(&sensor->data_mutex);
            memcpy(&axis, &sensor->axis, sizeof(sensor->axis));
            mutex_unlock(&sensor->data_mutex);
            if (copy_to_user(argp, &axis, sizeof(axis))) {
                dev_err(&client->dev, "failed to copy sense data to user space.\n");
                result = -EFAULT;
                goto error;
            }
            break;
    
        default:
            result = -ENOTTY;
        goto error;
        }
    
    error:
        return result;
}
 
 static int light_dev_open(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 static int light_dev_release(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 /* ioctl - I/O control */
 static long light_dev_ioctl(struct file *file,
               unsigned int cmd, unsigned long arg)
 {
     struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_LIGHT];
     struct i2c_client *client = sensor->client;
     void __user *argp = (void __user *)arg;
     int result = 0;
     short rate;
 
     switch (cmd) {
     case LIGHTSENSOR_IOCTL_SET_RATE:
         if (copy_from_user(&rate, argp, sizeof(rate))) {
             dev_err(&client->dev, "%s:failed to copy light sensor rate from user space.\n", __func__);
             return -EFAULT;
         }
         mutex_lock(&sensor->operation_mutex);
         result = sensor_reset_rate(client, rate);
         if (result < 0) {
             mutex_unlock(&sensor->operation_mutex);
             goto error;
         }
         mutex_unlock(&sensor->operation_mutex);
         break;
     case LIGHTSENSOR_IOCTL_GET_ENABLED:
         result = sensor->status_cur;
         if (copy_to_user(argp, &result, sizeof(result))) {
             dev_err(&client->dev, "%s:failed to copy light sensor status to user space.\n", __func__);
             return -EFAULT;
         }
         break;
     case LIGHTSENSOR_IOCTL_ENABLE:
         if (copy_from_user(&result, argp, sizeof(result))) {
             dev_err(&client->dev, "%s:failed to copy light sensor status from user space.\n", __func__);
             return -EFAULT;
         }
 
         mutex_lock(&sensor->operation_mutex);
         if (result) {
             if (sensor->status_cur == SENSOR_OFF)
                 sensor_enable(sensor, SENSOR_ON);
         } else {
             if (sensor->status_cur == SENSOR_ON)
                 sensor_enable(sensor, SENSOR_OFF);
         }
         mutex_unlock(&sensor->operation_mutex);
         break;
 
     default:
         break;
     }
 
 error:
     return result;
 }
 
 static int proximity_dev_open(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 static int proximity_dev_release(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 /* ioctl - I/O control */
 static long proximity_dev_ioctl(struct file *file,
               unsigned int cmd, unsigned long arg)
 {
     struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_PROXIMITY];
     void __user *argp = (void __user *)arg;
     int result = 0;
 
     switch (cmd) {
     case PSENSOR_IOCTL_GET_ENABLED:
         result = sensor->status_cur;
         if (copy_to_user(argp, &result, sizeof(result))) {
             dev_err(&sensor->client->dev, "%s:failed to copy psensor status to user space.\n", __func__);
             return -EFAULT;
         }
         break;
     case PSENSOR_IOCTL_ENABLE:
         if (copy_from_user(&result, argp, sizeof(result))) {
             dev_err(&sensor->client->dev, "%s:failed to copy psensor status from user space.\n", __func__);
             return -EFAULT;
         }
         mutex_lock(&sensor->operation_mutex);
         if (result) {
             if (sensor->status_cur == SENSOR_OFF)
                 sensor_enable(sensor, SENSOR_ON);
         } else {
             if (sensor->status_cur == SENSOR_ON)
                 sensor_enable(sensor, SENSOR_OFF);
         }
         mutex_unlock(&sensor->operation_mutex);
         break;
 
     default:
         break;
     }
 
     return result;
 }
 
 static int temperature_dev_open(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 static int temperature_dev_release(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 /* ioctl - I/O control */
 static long temperature_dev_ioctl(struct file *file,
               unsigned int cmd, unsigned long arg)
 {
     struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_TEMPERATURE];
     void __user *argp = (void __user *)arg;
     int result = 0;
 
     switch (cmd) {
     case TEMPERATURE_IOCTL_GET_ENABLED:
         result = sensor->status_cur;
         if (copy_to_user(argp, &result, sizeof(result))) {
             dev_err(&sensor->client->dev, "%s:failed to copy temperature sensor status to user space.\n", __func__);
             return -EFAULT;
         }
         break;
     case TEMPERATURE_IOCTL_ENABLE:
         if (copy_from_user(&result, argp, sizeof(result))) {
             dev_err(&sensor->client->dev, "%s:failed to copy temperature sensor status from user space.\n", __func__);
             return -EFAULT;
         }
         mutex_lock(&sensor->operation_mutex);
         if (result) {
             if (sensor->status_cur == SENSOR_OFF)
                 sensor_enable(sensor, SENSOR_ON);
         } else {
             if (sensor->status_cur == SENSOR_ON)
                 sensor_enable(sensor, SENSOR_OFF);
         }
         mutex_unlock(&sensor->operation_mutex);
         break;
 
     default:
         break;
     }
 
     return result;
 }
 
 
 static int pressure_dev_open(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 
 static int pressure_dev_release(struct inode *inode, struct file *file)
 {
     return 0;
 }
 
 
 /* ioctl - I/O control */
 static long pressure_dev_ioctl(struct file *file,
               unsigned int cmd, unsigned long arg)
 {
     struct sensor_private_data *sensor = g_sensor[SENSOR_TYPE_PRESSURE];
     void __user *argp = (void __user *)arg;
     int result = 0;
 
     switch (cmd) {
     case PRESSURE_IOCTL_GET_ENABLED:
         result = sensor->status_cur;
         if (copy_to_user(argp, &result, sizeof(result))) {
             dev_err(&sensor->client->dev, "%s:failed to copy pressure sensor status to user space.\n", __func__);
             return -EFAULT;
         }
         break;
     case PRESSURE_IOCTL_ENABLE:
         if (copy_from_user(&result, argp, sizeof(result))) {
             dev_err(&sensor->client->dev, "%s:failed to copy pressure sensor status from user space.\n", __func__);
             return -EFAULT;
         }
         mutex_lock(&sensor->operation_mutex);
         if (result) {
             if (sensor->status_cur == SENSOR_OFF)
                 sensor_enable(sensor, SENSOR_ON);
         } else {
             if (sensor->status_cur == SENSOR_ON)
                 sensor_enable(sensor, SENSOR_OFF);
         }
         mutex_unlock(&sensor->operation_mutex);
         break;
 
     default:
         break;
     }
 
     return result;
 }
 
static int sensor_misc_device_register(struct sensor_private_data *sensor, int type)
{
    int result = 0;

    switch (type) {
    case SENSOR_TYPE_ACCEL:
        if (!sensor->ops->misc_dev) {
            sensor->fops.owner = THIS_MODULE;
            sensor->fops.unlocked_ioctl = gsensor_dev_ioctl;
            sensor->fops.open = gsensor_dev_open;
            sensor->fops.release = gsensor_dev_release;

            sensor->miscdev.minor = MISC_DYNAMIC_MINOR;
            sensor->miscdev.name = "sensor_accel";
            sensor->miscdev.fops = &sensor->fops;
        } else {
            memcpy(&sensor->miscdev, sensor->ops->misc_dev, sizeof(*sensor->ops->misc_dev));
        }
        break;

    case SENSOR_TYPE_COMPASS:
        if (!sensor->ops->misc_dev) {
            sensor->fops.owner = THIS_MODULE;
            sensor->fops.unlocked_ioctl = compass_dev_ioctl;
            sensor->fops.open = compass_dev_open;
            sensor->fops.release = compass_dev_release;

            sensor->miscdev.minor = MISC_DYNAMIC_MINOR;
            sensor->miscdev.name = "compass";
            sensor->miscdev.fops = &sensor->fops;
        } else {
            memcpy(&sensor->miscdev, sensor->ops->misc_dev, sizeof(*sensor->ops->misc_dev));
        }
        break;

    case SENSOR_TYPE_GYROSCOPE:
        if (!sensor->ops->misc_dev) {
            sensor->fops.owner = THIS_MODULE;
            sensor->fops.unlocked_ioctl = gyro_dev_ioctl;
            sensor->fops.open = gyro_dev_open;
            sensor->fops.release = gyro_dev_release;

            sensor->miscdev.minor = MISC_DYNAMIC_MINOR;
            sensor->miscdev.name = "sensor_gyro";
            sensor->miscdev.fops = &sensor->fops;
        } else {
            memcpy(&sensor->miscdev, sensor->ops->misc_dev, sizeof(*sensor->ops->misc_dev));
        }
        break;

    case SENSOR_TYPE_LIGHT:
        if (!sensor->ops->misc_dev) {
            sensor->fops.owner = THIS_MODULE;
            sensor->fops.unlocked_ioctl = light_dev_ioctl;
            sensor->fops.open = light_dev_open;
            sensor->fops.release = light_dev_release;

            sensor->miscdev.minor = MISC_DYNAMIC_MINOR;
            sensor->miscdev.name = "lightsensor";
            sensor->miscdev.fops = &sensor->fops;
        } else {
            memcpy(&sensor->miscdev, sensor->ops->misc_dev, sizeof(*sensor->ops->misc_dev));
        }
        break;

    case SENSOR_TYPE_PROXIMITY:
        if (!sensor->ops->misc_dev) {
            sensor->fops.owner = THIS_MODULE;
            sensor->fops.unlocked_ioctl = proximity_dev_ioctl;
            sensor->fops.open = proximity_dev_open;
            sensor->fops.release = proximity_dev_release;

            sensor->miscdev.minor = MISC_DYNAMIC_MINOR;
            sensor->miscdev.name = "psensor";
            sensor->miscdev.fops = &sensor->fops;
        } else {
            memcpy(&sensor->miscdev, sensor->ops->misc_dev, sizeof(*sensor->ops->misc_dev));
        }
        break;

    case SENSOR_TYPE_TEMPERATURE:
        if (!sensor->ops->misc_dev) {
            sensor->fops.owner = THIS_MODULE;
            sensor->fops.unlocked_ioctl = temperature_dev_ioctl;
            sensor->fops.open = temperature_dev_open;
            sensor->fops.release = temperature_dev_release;

            sensor->miscdev.minor = MISC_DYNAMIC_MINOR;
            sensor->miscdev.name = "temperature";
            sensor->miscdev.fops = &sensor->fops;
        } else {
            memcpy(&sensor->miscdev, sensor->ops->misc_dev, sizeof(*sensor->ops->misc_dev));
        }
        break;

    case SENSOR_TYPE_PRESSURE:
        if (!sensor->ops->misc_dev) {
            sensor->fops.owner = THIS_MODULE;
            sensor->fops.unlocked_ioctl = pressure_dev_ioctl;
            sensor->fops.open = pressure_dev_open;
            sensor->fops.release = pressure_dev_release;

            sensor->miscdev.minor = MISC_DYNAMIC_MINOR;
            sensor->miscdev.name = "pressure";
            sensor->miscdev.fops = &sensor->fops;
        } else {
            memcpy(&sensor->miscdev, sensor->ops->misc_dev, sizeof(*sensor->ops->misc_dev));
        }
        break;

    default:
        dev_err(&sensor->client->dev, "%s:unknow sensor type=%d\n", __func__, type);
        result = -1;
        goto error;
    }

    sensor->miscdev.parent = &sensor->client->dev;
    result = misc_register(&sensor->miscdev);
    if (result < 0) {
        dev_err(&sensor->client->dev,
            "fail to register misc device %s\n", sensor->miscdev.name);
        goto error;
    }
    dev_info(&sensor->client->dev, "%s:miscdevice: %s\n", __func__, sensor->miscdev.name);

error:
    return result;
}
 
static int sensor_probe(struct i2c_client *client, const struct i2c_device_id *devid)
{
    struct sensor_private_data *sensor;
    struct sensor_platform_data *pdata;
    struct device_node *np = client->dev.of_node;
    enum of_gpio_flags rst_flags, pwr_flags;
    unsigned long irq_flags;
    int result = 0;
    int type = 0;
    int reprobe_en = 0;

    dev_info(&client->adapter->dev, "%s: %s,%p\n", __func__, devid->name, client);

    /* 检查 I2C adapter 是否支持标准 i2c 功能 */
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
        result = -ENODEV;
        goto out_no_free;
    }
    if (!np) {
        dev_err(&client->dev, "no device tree\n");
        return -EINVAL;
    }
    pdata = devm_kzalloc(&client->dev, sizeof(*pdata), GFP_KERNEL);
    if (!pdata) {
        result = -ENOMEM;
        goto out_no_free;
    }
    sensor = devm_kzalloc(&client->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor) {
        result = -ENOMEM;
        goto out_no_free;
    }

    /* 获取传感器的类型 */
    of_property_read_u32(np, "type", &(pdata->type));

    pdata->irq_pin = of_get_named_gpio_flags(np, "irq-gpio", 0, (enum of_gpio_flags *)&irq_flags);
    pdata->reset_pin = of_get_named_gpio_flags(np, "reset-gpio", 0, &rst_flags);
    pdata->power_pin = of_get_named_gpio_flags(np, "power-gpio", 0, &pwr_flags);
    pdata->wake_enable = of_property_read_bool(np, "wakeup-source");
    of_property_read_u32(np, "irq_enable", &(pdata->irq_enable));
    of_property_read_u32(np, "poll_delay_ms", &(pdata->poll_delay_ms));

    of_property_read_u32(np, "x_min", &(pdata->x_min));
    of_property_read_u32(np, "y_min", &(pdata->y_min));
    of_property_read_u32(np, "z_min", &(pdata->z_min));
    of_property_read_u32(np, "factory", &(pdata->factory));
    of_property_read_u32(np, "layout", &(pdata->layout));
    of_property_read_u32(np, "reprobe_en", &reprobe_en);

    of_property_read_u8(np, "address", &(pdata->address));
    of_get_property(np, "project_name", pdata->project_name);

    of_property_read_u32(np, "power-off-in-suspend",
                &pdata->power_off_in_suspend);

    switch (pdata->layout) {
    case 1:
        pdata->orientation[0] = 1;
        pdata->orientation[1] = 0;
        pdata->orientation[2] = 0;

        pdata->orientation[3] = 0;
        pdata->orientation[4] = 1;
        pdata->orientation[5] = 0;

        pdata->orientation[6] = 0;
        pdata->orientation[7] = 0;
        pdata->orientation[8] = 1;
        break;

    case 2:
        pdata->orientation[0] = 0;
        pdata->orientation[1] = -1;
        pdata->orientation[2] = 0;

        pdata->orientation[3] = 1;
        pdata->orientation[4] = 0;
        pdata->orientation[5] = 0;

        pdata->orientation[6] = 0;
        pdata->orientation[7] = 0;
        pdata->orientation[8] = 1;
        break;

    case 3:
        pdata->orientation[0] = -1;
        pdata->orientation[1] = 0;
        pdata->orientation[2] = 0;

        pdata->orientation[3] = 0;
        pdata->orientation[4] = -1;
        pdata->orientation[5] = 0;

        pdata->orientation[6] = 0;
        pdata->orientation[7] = 0;
        pdata->orientation[8] = 1;
        break;

    case 4:
        pdata->orientation[0] = 0;
        pdata->orientation[1] = 1;
        pdata->orientation[2] = 0;

        pdata->orientation[3] = -1;
        pdata->orientation[4] = 0;
        pdata->orientation[5] = 0;

        pdata->orientation[6] = 0;
        pdata->orientation[7] = 0;
        pdata->orientation[8] = 1;
        break;

    case 5:
        pdata->orientation[0] = 1;
        pdata->orientation[1] = 0;
        pdata->orientation[2] = 0;

        pdata->orientation[3] = 0;
        pdata->orientation[4] = -1;
        pdata->orientation[5] = 0;

        pdata->orientation[6] = 0;
        pdata->orientation[7] = 0;
        pdata->orientation[8] = -1;
        break;

    case 6:
        pdata->orientation[0] = 0;
        pdata->orientation[1] = -1;
        pdata->orientation[2] = 0;

        pdata->orientation[3] = -1;
        pdata->orientation[4] = 0;
        pdata->orientation[5] = 0;

        pdata->orientation[6] = 0;
        pdata->orientation[7] = 0;
        pdata->orientation[8] = -1;
        break;

    case 7:
        pdata->orientation[0] = -1;
        pdata->orientation[1] = 0;
        pdata->orientation[2] = 0;

        pdata->orientation[3] = 0;
        pdata->orientation[4] = 1;
        pdata->orientation[5] = 0;

        pdata->orientation[6] = 0;
        pdata->orientation[7] = 0;
        pdata->orientation[8] = -1;
        break;

    case 8:
        pdata->orientation[0] = 0;
        pdata->orientation[1] = 1;
        pdata->orientation[2] = 0;

        pdata->orientation[3] = 1;
        pdata->orientation[4] = 0;
        pdata->orientation[5] = 0;

        pdata->orientation[6] = 0;
        pdata->orientation[7] = 0;
        pdata->orientation[8] = -1;
        break;

    default:
        pdata->orientation[0] = 1;
        pdata->orientation[1] = 0;
        pdata->orientation[2] = 0;

        pdata->orientation[3] = 0;
        pdata->orientation[4] = 1;
        pdata->orientation[5] = 0;

        pdata->orientation[6] = 0;
        pdata->orientation[7] = 0;
        pdata->orientation[8] = 1;
        break;
    }

    client->irq = pdata->irq_pin;
    type = pdata->type;
    pdata->irq_flags = irq_flags;

    if ((type >= SENSOR_NUM_TYPES) || (type <= SENSOR_TYPE_NULL)) {
        dev_err(&client->adapter->dev, "sensor type is error %d\n", type);
        result = -EFAULT;
        goto out_no_free;
    }
    if (((int)devid->driver_data >= SENSOR_NUM_ID_HIGH) || ((int)devid->driver_data <= SENSOR_NUM_ID_LOW)) {
        dev_err(&client->adapter->dev, "sensor id is error %d\n", (int)devid->driver_data);
        result = -EFAULT;
        goto out_no_free;
    }
    i2c_set_clientdata(client, sensor);
    sensor->client = client;
    sensor->pdata = pdata;
    sensor->type = type;
    sensor->i2c_id = (struct i2c_device_id *)devid;

    memset(&(sensor->axis), 0, sizeof(struct sensor_axis));
    mutex_init(&sensor->data_mutex);
    mutex_init(&sensor->operation_mutex);
    mutex_init(&sensor->sensor_mutex);
    mutex_init(&sensor->i2c_mutex);

    atomic_set(&sensor->is_factory, 0);
    init_waitqueue_head(&sensor->is_factory_ok);

    /* As default, report all information */
    atomic_set(&sensor->flags.m_flag, 1);
    atomic_set(&sensor->flags.a_flag, 1);
    atomic_set(&sensor->flags.mv_flag, 1);
    atomic_set(&sensor->flags.open_flag, 0);
    atomic_set(&sensor->flags.debug_flag, 1);
    init_waitqueue_head(&sensor->flags.open_wq);
    sensor->flags.delay = 100;

    sensor->status_cur = SENSOR_OFF;
    sensor->axis.x = 0;
    sensor->axis.y = 0;
    sensor->axis.z = 0;

    result = sensor_chip_init(sensor->client);
    if (result < 0) {
        if (reprobe_en && (result == -2)) {
            sensor_probe_times[sensor->ops->id_i2c]++;
            if (sensor_probe_times[sensor->ops->id_i2c] < 3)
                result = -EPROBE_DEFER;
        }
        goto out_free_memory;
    }

    // 分配并初始化输入设备
    sensor->input_dev = devm_input_allocate_device(&client->dev);
    if (!sensor->input_dev) {
        result = -ENOMEM;
        dev_err(&client->dev,
            "Failed to allocate input device\n");
        goto out_free_memory;
    }

    /* 根据器件类型，设置要上报的数据 */
    switch (type) {
    case SENSOR_TYPE_ANGLE:
        sensor->input_dev->name = "angle";
        set_bit(EV_ABS, sensor->input_dev->evbit);
        /* x-axis acceleration */
        input_set_abs_params(sensor->input_dev, ABS_X, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        /* y-axis acceleration */
        input_set_abs_params(sensor->input_dev, ABS_Y, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        /* z-axis acceleration */
        input_set_abs_params(sensor->input_dev, ABS_Z, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        break;
    case SENSOR_TYPE_ACCEL:
        sensor->input_dev->name = "gsensor";
        set_bit(EV_ABS, sensor->input_dev->evbit);
        /* x-axis acceleration */
        input_set_abs_params(sensor->input_dev, ABS_X, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        /* y-axis acceleration */
        input_set_abs_params(sensor->input_dev, ABS_Y, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        /* z-axis acceleration */
        input_set_abs_params(sensor->input_dev, ABS_Z, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        break;
    case SENSOR_TYPE_COMPASS:
        sensor->input_dev->name = "compass";
        /* Setup input device */
        set_bit(EV_ABS, sensor->input_dev->evbit);
        /* yaw (0, 360) */
        input_set_abs_params(sensor->input_dev, ABS_RX, 0, 23040, 0, 0);
        /* pitch (-180, 180) */
        input_set_abs_params(sensor->input_dev, ABS_RY, -11520, 11520, 0, 0);
        /* roll (-90, 90) */
        input_set_abs_params(sensor->input_dev, ABS_RZ, -5760, 5760, 0, 0);
        /* x-axis acceleration (720 x 8G) */
        input_set_abs_params(sensor->input_dev, ABS_X, -5760, 5760, 0, 0);
        /* y-axis acceleration (720 x 8G) */
        input_set_abs_params(sensor->input_dev, ABS_Y, -5760, 5760, 0, 0);
        /* z-axis acceleration (720 x 8G) */
        input_set_abs_params(sensor->input_dev, ABS_Z, -5760, 5760, 0, 0);
        /* status of magnetic sensor */
        input_set_abs_params(sensor->input_dev, ABS_RUDDER, -32768, 3, 0, 0);
        /* status of acceleration sensor */
        input_set_abs_params(sensor->input_dev, ABS_WHEEL, -32768, 3, 0, 0);
        /* x-axis of raw magnetic vector (-4096, 4095) */
        input_set_abs_params(sensor->input_dev, ABS_HAT0X, -20480, 20479, 0, 0);
        /* y-axis of raw magnetic vector (-4096, 4095) */
        input_set_abs_params(sensor->input_dev, ABS_HAT0Y, -20480, 20479, 0, 0);
        /* z-axis of raw magnetic vector (-4096, 4095) */
        input_set_abs_params(sensor->input_dev, ABS_BRAKE, -20480, 20479, 0, 0);
        break;
    case SENSOR_TYPE_GYROSCOPE:
        sensor->input_dev->name = "gyro";
        /* x-axis acceleration */
        // 相对运动事件，相对旋转或移动
        input_set_capability(sensor->input_dev, EV_REL, REL_RX);
        input_set_abs_params(sensor->input_dev, ABS_RX, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        /* y-axis acceleration */
        input_set_capability(sensor->input_dev, EV_REL, REL_RY);
        input_set_abs_params(sensor->input_dev, ABS_RY, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        /* z-axis acceleration */
        input_set_capability(sensor->input_dev, EV_REL, REL_RZ);
        input_set_abs_params(sensor->input_dev, ABS_RZ, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        break;
    case SENSOR_TYPE_LIGHT:
        sensor->input_dev->name = "lightsensor-level";
        set_bit(EV_ABS, sensor->input_dev->evbit);
        input_set_abs_params(sensor->input_dev, ABS_MISC, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        input_set_abs_params(sensor->input_dev, ABS_TOOL_WIDTH,  sensor->ops->brightness[0], sensor->ops->brightness[1], 0, 0);
        break;
    case SENSOR_TYPE_PROXIMITY:
        sensor->input_dev->name = "proximity";
        set_bit(EV_ABS, sensor->input_dev->evbit);
        input_set_abs_params(sensor->input_dev, ABS_DISTANCE, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        break;
    case SENSOR_TYPE_TEMPERATURE:
        sensor->input_dev->name = "temperature";
        set_bit(EV_ABS, sensor->input_dev->evbit);
        input_set_abs_params(sensor->input_dev, ABS_THROTTLE, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        break;
    case SENSOR_TYPE_PRESSURE:
        sensor->input_dev->name = "pressure";
        set_bit(EV_ABS, sensor->input_dev->evbit);
        input_set_abs_params(sensor->input_dev, ABS_PRESSURE, sensor->ops->range[0], sensor->ops->range[1], 0, 0);
        break;
    default:
        dev_err(&client->dev, "%s:unknow sensor type=%d\n", __func__, type);
        break;
    }
    sensor->input_dev->dev.parent = &client->dev;

    // 注册输入设备
    result = input_register_device(sensor->input_dev);
    if (result) {
        dev_err(&client->dev,
            "Unable to register input device %s\n", sensor->input_dev->name);
        goto out_input_register_device_failed;
    }

    /* 中断或延迟工作队列初始化 */
    result = sensor_irq_init(sensor->client);
    if (result) {
        dev_err(&client->dev,
            "fail to init sensor irq,ret=%d\n", result);
        goto out_input_register_device_failed;
    }

    sensor->miscdev.parent = &client->dev;
    result = sensor_misc_device_register(sensor, type);
    if (result) {
        dev_err(&client->dev,
            "fail to register misc device %s\n", sensor->miscdev.name);
        goto out_misc_device_register_device_failed;
    }

    g_sensor[type] = sensor;


    dev_info(&client->dev, "%s:initialized ok,sensor name:%s,type:%d,id=%d\n\n", __func__, sensor->ops->name, type, (int)sensor->i2c_id->driver_data);

    return result;

out_misc_device_register_device_failed:
out_input_register_device_failed:
out_free_memory:
out_no_free:
    dev_err(&client->adapter->dev, "%s failed %d\n\n", __func__, result);
    return result;
}
 
static int sensor_remove(struct i2c_client *client)
{
    struct sensor_private_data *sensor =
        (struct sensor_private_data *) i2c_get_clientdata(client);

    sensor->stop_work = 1;
    cancel_delayed_work_sync(&sensor->delaywork);
    misc_deregister(&sensor->miscdev);

    return 0;
}

/**
 * 进行一些条件检查，然后调用 sensor_probe 函数。
 */
int jason_sensor_register_device(struct i2c_client *client,
            struct sensor_platform_data *slave_pdata,
            const struct i2c_device_id *devid,
            struct sensor_operate *ops)
{
    int result = 0;

    if (!client || !ops) {
        dev_err(&client->dev, "%s: no device or ops.\n", __func__);
        return -ENODEV;
    }

    if ((ops->id_i2c >= SENSOR_NUM_ID_HIGH) || (ops->id_i2c <= SENSOR_NUM_ID_LOW) ||
        (((int)devid->driver_data) != ops->id_i2c)) {
        dev_err(&client->dev, "%s: %s id is error %d\n",
            __func__, ops->name, ops->id_i2c);
        return -EINVAL;
    }

    sensor_ops[ops->id_i2c] = ops;
    dev_info(&client->dev, "%s: %s, id = %d\n",
        __func__, sensor_ops[ops->id_i2c]->name, ops->id_i2c);

    sensor_probe(client, devid);

    return result;
}
EXPORT_SYMBOL(jason_sensor_register_device);
 
int jason_sensor_unregister_device(struct i2c_client *client,
        struct sensor_platform_data *slave_pdata,
        struct sensor_operate *ops)
{
    int result = 0;

    if (!client || !ops) {
        dev_err(&client->dev, "%s: no device or ops.\n", __func__);
        return -ENODEV;
    }

    if ((ops->id_i2c >= SENSOR_NUM_ID_HIGH) || (ops->id_i2c <= SENSOR_NUM_ID_LOW)) {
        dev_err(&client->dev, "%s: %s id is error %d\n",
            __func__, ops->name, ops->id_i2c);
        return -EINVAL;
    }

    sensor_remove(client);

    dev_info(&client->dev, "%s: %s, id = %d\n",
        __func__, sensor_ops[ops->id_i2c]->name, ops->id_i2c);
    sensor_ops[ops->id_i2c] = NULL;

    return result;
}
EXPORT_SYMBOL(jason_sensor_unregister_device);

/**********************************General**************************************/

static int sensor_class_init(void)
{
    jason_sensor_class = class_create(THIS_MODULE, "jason_sensor_class");

    return 0;
}

static int __init sensor_init(void)
{
    sensor_class_init();

    return 0;
}
 
static void __exit sensor_exit(void)
{
    class_destroy(jason_sensor_class);
}
 
module_init(sensor_init);
module_exit(sensor_exit);
 
MODULE_AUTHOR("Jason");
MODULE_DESCRIPTION("User space character device interface for sensors");
MODULE_LICENSE("GPL");
 