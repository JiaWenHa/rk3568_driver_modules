#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/sensor-dev.h>
#include <linux/types.h>
#include "jason_sh3001.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jason Jia");
MODULE_DESCRIPTION("A driver for sh3001 acc.");

/******************************************************************************/
static int jason_sh3001_read_reg(struct i2c_client *client, u_int8_t addr, u_int8_t *buf);

/**********************************Specific**************************************/

static int configureGyroscope(struct i2c_client *client, const GyroConfig *config)
{
    uint8_t reg0 = 0, reg1 = 0, reg2 = 0, reg3 = 0, reg4 = 0, reg5 = 0;

    // 配置 GYRO_CONFIG_0
    reg0 |= (config->shutDown << 4);
    reg0 |= (config->digitalFilter << 0);

    // 配置 GYRO_CONFIG_1
    reg1 |= (config->odr & 0x0F);

    // 配置 GYRO_CONFIG_2
    reg2 |= (config->lpfBypass << 4);
    reg2 |= (config->lpfCutoff << 2);

    // 配置 GYRO_CONFIG_3, GYRO_CONFIG_4, GYRO_CONFIG_5
    reg3 |= (config->fsrX & 0x07);
    reg4 |= (config->fsrY & 0x07);
    reg5 |= (config->fsrZ & 0x07);

    if(jason_sh3001_read_reg(client, GYRO_CONFIG_0, &reg0) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;

    if(jason_sh3001_read_reg(client, GYRO_CONFIG_1, &reg1) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;
    
    if(jason_sh3001_read_reg(client, GYRO_CONFIG_2, &reg2) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;

    if(jason_sh3001_read_reg(client, GYRO_CONFIG_3, &reg3) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;

    if(jason_sh3001_read_reg(client, GYRO_CONFIG_2, &reg4) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;

    if(jason_sh3001_read_reg(client, GYRO_CONFIG_3, &reg5) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;

    return JASON_SH3001_TRUE;
}

static int configureAccelerometer(struct i2c_client *client, const AccConfig* config)
{
    uint8_t reg0 = 0, reg1 = 0, reg2 = 0, reg3 = 0;

    // 配置 ACC_CONFIG_0
    reg0 |= (config->workMode << 7);
    reg0 |= (config->dither << 6);
    reg0 |= (config->digitalFilter << 0);

    // 配置 ACC_CONFIG_1
    reg1 |= (config->odr & 0x0F);

    // 配置 ACC_CONFIG_2
    reg2 |= (config->range & 0x07);

    // 配置 ACC_CONFIG_3
    reg3 |= (config->lpfCutoff << 5);
    reg3 |= (config->bypassLPF << 3);

    if(jason_sh3001_read_reg(client, ACC_CONFIG_0, &reg0) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;

    if(jason_sh3001_read_reg(client, ACC_CONFIG_1, &reg1) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;
    
    if(jason_sh3001_read_reg(client, ACC_CONFIG_2, &reg2) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;

    if(jason_sh3001_read_reg(client, ACC_CONFIG_3, &reg3) == JASON_SH3001_FALSE)
        return JASON_SH3001_FALSE;

    return JASON_SH3001_TRUE;
}

static int jason_sh3001_read_reg(struct i2c_client *client, u_int8_t addr, u_int8_t *buf)
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
    msgs[1].len = 1; // 要读取的数据长度，这里只读取1个字节
    msgs[1].buf = buf; // 存储读取数据的缓冲区

    // 执行 I2C 传输操作，发送两个消息（写和读）
    ret = i2c_transfer(client->adapter, msgs, 2);

    if (ret != 2) {
        dev_err(&client->dev, "I2C transfer failed: ret=%d\n", ret);
        return JASON_SH3001_FALSE;
    }

    return JASON_SH3001_TRUE;
}

static int jason_sh3001_sensor_init(struct i2c_client *client)
{
    u_int8_t regData = 0;
    int8_t i = 0;
    /* C90 变量声明必须在函数开头 */
    AccConfig acc_config = {
        .workMode = ACC_WORK_MODE_LOW_POWER,
        .dither = ACC_DITHER_ENABLE,
        .digitalFilter = ACC_DIGITAL_FILTER_ENABLE,
        .odr = ACC_ODR_500HZ,
        .range = ACC_RANGE_2G,
        .lpfCutoff = ACC_LPF_CUTOFF_0_25,
        .bypassLPF = ACC_BYPASS_LPF_NO
    };

    // 初始化陀螺仪配置
    GyroConfig gyro_config;

    gyro_config.shutDown = GYRO_SHUT_DOWN_NO;
    gyro_config.digitalFilter = GYRO_DIGITAL_FILTER_ENABLE;
    gyro_config.odr = GYRO_ODR_500HZ;
    gyro_config.lpfBypass = GYRO_LPF_BYPASS_DISABLE;
    gyro_config.lpfCutoff = GYRO_LPF_CUTOFF_00;
    gyro_config.fsrX = GYRO_FSR_2000DPS;
    gyro_config.fsrY = GYRO_FSR_2000DPS;
    gyro_config.fsrZ = GYRO_FSR_2000DPS;

    /* The default Chip ID of this device is 0x61 */
    while((regData != 0x61) && (i++ < 3)) {
        if(jason_sh3001_read_reg(client, CHIP_ID, &regData) == JASON_SH3001_TRUE){
            break;
        }
    }
    if (regData != 0x61) {
        dev_err(&client->dev, "check id error, read data:0x%x, ops->id_data:0x%x\n", regData, 0x61);
        return JASON_SH3001_FALSE;
    } else {
        dev_info(&client->dev, "check id ok, read data:0x%x, ops->id_data:0x%x\n", regData, 0x61);
    }

    if(configureAccelerometer(client, &acc_config) == JASON_SH3001_FALSE){
        dev_err(&client->dev, "Configure accelerometer error!\n");
    }
    dev_err(&client->dev, "Configure accelerometer succeeded!\n");

    if(configureGyroscope(client, &gyro_config) == JASON_SH3001_FALSE){
        dev_err(&client->dev, "Configure gyroscope error!\n");
    }
    dev_err(&client->dev, "Configure gyroscope succeeded!\n");

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
    dev_info(&client->dev, "Enter sensor_report_value.\n");
    return 0;
}

static struct sensor_operate jason_sh3001_ops = {
    .name = "jason_sh3001_acc",
    .type = SENSOR_TYPE_ACCEL,
    .id_i2c = ACCEL_ID_SH3001,
    .read_reg = ACC_XDATA_L,
    .read_len = 6,
    .id_reg = CHIP_ID,
    .id_data = 0x61,
    .precision = 16,
    .ctrl_reg = -1,
	.ctrl_data = -1,
	.int_ctrl_reg = -1,
	.int_status_reg = -1,
    .range = {-32768, 32768},
    .init = sensor_init,
    .active = sensor_active,
	.report	= sensor_report_value, 
    .suspend = NULL,
	.resume	= NULL,
};

static int sh3001_acc_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    pr_info("sh3001_acc driver module loaded.\n");
    return sensor_register_device(client, NULL, dev_id, &jason_sh3001_ops);
}

static int sh3001_acc_remove(struct i2c_client *client)
{
    pr_info("sh3001_acc driver module unloaded.\n");
    return sensor_unregister_device(client, NULL, &jason_sh3001_ops);
}

/* 这里的 ACCEL_ID_SH3001 一定要有，用于与 linux/sensor-dev.h 配合使用 */
static const struct i2c_device_id sh3001_acc_id_table[] = {
    {"jason_sh3001_acc", ACCEL_ID_SH3001},
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
