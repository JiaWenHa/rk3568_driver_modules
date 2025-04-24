#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include "jason_sh3001.h"


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Jia");
MODULE_DESCRIPTION("A driver for sh3001 gyro.");
MODULE_SOFTDEP("pre: jason_sh3001_acc");

/*****************************Function definition********************************/
static int jason_sh3001_read_regs(struct i2c_client *client, uint8_t addr, int len, uint8_t *buf);

/**********************************Specific**************************************/

static int jason_sh3001_read_regs(struct i2c_client *client, uint8_t addr, int len, uint8_t *buf)
{
    struct i2c_msg msgs[2];
    int ret;

    // 第1步：写操作，指定要读取的寄存器地址
    msgs[0].flags = !I2C_M_RD; // 写操作标志
    msgs[0].addr = client->addr; // 器件地址，即I2C设备的地址
    msgs[0].len = 1; // 要发送的数据长度，这里只发送1个字节的寄存器地址
    msgs[0].buf = &addr; // 要发送的数据，即要读取的寄存器地址

    // 第2步：读操作，从指定寄存器读取数据
    msgs[1].flags = I2C_M_RD; // 读操作标志
    msgs[1].addr = client->addr; // 器件地址，与写操作使用相同的设备地址
    msgs[1].len = len; // 要读取的数据长度，这里只读取1个字节
    msgs[1].buf = buf; // 存储读取数据的缓冲区

    // 执行 I2C 传输操作，发送两个消息（写和读）
    ret = i2c_transfer(client->adapter, msgs, 2);

    if (ret != 2) {
        dev_err(&client->dev, "I2C transfer regs failed: ret=%d\n", ret);
        return JASON_SH3001_FALSE;
    }

    return JASON_SH3001_TRUE;
}

static int jason_sh3001_sensor_init(struct i2c_client *client)
{

    return JASON_SH3001_TRUE;
}

/**********************************General**************************************/

static int sensor_init(struct i2c_client *client)
{
    int ret = -1;

    /* Initialize sh3001 sensor */
    ret = jason_sh3001_sensor_init(client);
    if(ret < 0)
        return ret;
    dev_info(&client->dev, "Sensor initialization succeeded!\n");

    return ret;
}

static int sensor_active(struct i2c_client *client, int enable, int rate)
{
    dev_info(&client->dev, "Enter sensor_active.\n");
    return 0;
}

static int sensor_report_value(struct i2c_client *client)
{
    struct sensor_private_data *sensor = 
        (struct sensor_private_data *)i2c_get_clientdata(client);
    struct sensor_platform_data *pdata = sensor->pdata;
    struct sensor_axis axis;
    uint8_t buf[6] = {0};
    uint16_t x, y, z;
    int ret = -1;

    memset(buf, 0, sizeof(buf));
    do {
        ret = jason_sh3001_read_regs(client, sensor->ops->read_reg,
            sensor->ops->read_len, buf);
        if (ret < 0) {
            dev_err(&client->dev, "%s:%d jason_sh3001_read_regs error!\n", __func__, __LINE__);
            return ret;
        }
    } while (0);

	x = ((buf[1] << 8) & 0xFF00) + (buf[0] & 0xFF);
	y = ((buf[3] << 8) & 0xFF00) + (buf[2] & 0xFF);
	z = ((buf[5] << 8) & 0xFF00) + (buf[4] & 0xFF);
	axis.x = (pdata->orientation[0]) * x + (pdata->orientation[1]) * y + (pdata->orientation[2]) * z;
	axis.y = (pdata->orientation[3]) * x + (pdata->orientation[4]) * y + (pdata->orientation[5]) * z;
	axis.z = (pdata->orientation[6]) * x + (pdata->orientation[7]) * y + (pdata->orientation[8]) * z;

    if (sensor->status_cur == SENSOR_ON) {
		/* Report acceleration sensor information */
		input_report_abs(sensor->input_dev, ABS_RX, axis.x);
		input_report_abs(sensor->input_dev, ABS_RY, axis.y);
		input_report_abs(sensor->input_dev, ABS_RZ, axis.z);
		input_sync(sensor->input_dev);
	}

	mutex_lock(&(sensor->data_mutex));
	sensor->axis = axis;
	mutex_unlock(&(sensor->data_mutex));
    // dev_info(&client->dev, "sensor_report_value ended.\n");

    return ret;
}

static struct sensor_operate jason_sh3001_ops = {
    .name = "jason_sh3001_gyro",
    .type = SENSOR_TYPE_GYROSCOPE,
    .id_i2c = GYRO_ID_SH3001,
    .read_reg = GYRO_XDATA_L,
    .read_len = 6,
    .id_reg = CHIP_ID,
    .id_data = 0x61,
    .precision = 16,
    .ctrl_reg = -1,
	.ctrl_data = -1,
	.int_ctrl_reg = -1,
	.int_status_reg = -1,
    .range = {-32768, 32768},
    .trig = IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
    .init = sensor_init,
    .active = sensor_active,
	.report	= sensor_report_value, 
    .suspend = NULL,
	.resume	= NULL,
};

static int sh3001_gyro_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    pr_info("sh3001_gyro driver module loaded.\n");
    return jason_sensor_register_device(client, NULL, dev_id, &jason_sh3001_ops);
}

static int sh3001_gyro_remove(struct i2c_client *client)
{
    pr_info("sh3001_gyro driver module unloaded.\n");
    return jason_sensor_unregister_device(client, NULL, &jason_sh3001_ops);
}

/* 这里的 GYRO_ID_SH3001 一定要有，用于与 linux/sensor-dev.h 配合使用 */
static const struct i2c_device_id sh3001_gyro_id_table[] = {
    {"jason_sh3001_gyro", GYRO_ID_SH3001},
    {},
};

static struct i2c_driver sh3001_gyro_driver = {
	/* Standard driver model interfaces */
	.probe = sh3001_gyro_probe, 
	.remove = sh3001_gyro_remove, 
    .id_table = sh3001_gyro_id_table,
    .driver = {
        .name = "jason_sh3001_gyro",
        .owner = THIS_MODULE,
    },
};

/* This will create the init and exit function automatically */
module_i2c_driver(sh3001_gyro_driver);
