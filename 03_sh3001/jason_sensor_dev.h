#ifndef __JASON_SENSOR_DEV_H__
#define __JASON_SENSOR_DEV_H__

#include <linux/miscdevice.h>
#include <dt-bindings/sensor-dev.h>
#include <linux/module.h>

#define SENSOR_ON		1
#define SENSOR_OFF		0
#define SENSOR_UNKNOW_DATA	-1
 
enum sensor_id {
    ID_INVALID = 0,

    ANGLE_ID_ALL,
    ANGLE_ID_KXTIK,
    ANGLE_ID_LIS3DH,

    ACCEL_ID_ALL,
    ACCEL_ID_LIS331,
    ACCEL_ID_LSM303DLX,
    ACCEL_ID_LIS3DH,
    ACCEL_ID_KXSD9,
    ACCEL_ID_KXTF9,
    ACCEL_ID_KXTIK,
    ACCEL_ID_KXTJ9,
    ACCEL_ID_BMA150,
    ACCEL_ID_BMA222,
    ACCEL_ID_BMA250,
    ACCEL_ID_ADXL34X,
    ACCEL_ID_MMA8450,
    ACCEL_ID_MMA845X,
    ACCEL_ID_MMA7660,
    ACCEL_ID_SC7660,
    ACCEL_ID_SC7A20,
    ACCEL_ID_SC7A30,
    ACCEL_ID_MPU6050,
    ACCEL_ID_MXC6225,
    ACCEL_ID_MXC6655XA,
    ACCEL_ID_DMARD10,
    ACCEL_ID_LSM303D,
    ACCEL_ID_MC3230,
    ACCEL_ID_MPU6880,
    ACCEL_ID_MPU6500,
    ACCEL_ID_LSM330,
    ACCEL_ID_BMA2XX,
    ACCEL_ID_STK8BAXX,
    ACCEL_ID_MIR3DA,
    ACCEL_ID_ICM2060X,
    ACCEL_ID_DA215S,
    ACCEL_ID_DA228E,
    ACCEL_ID_SH3001,
    COMPASS_ID_ALL,
    COMPASS_ID_AK8975,
    COMPASS_ID_AK8963,
    COMPASS_ID_AK09911,
    COMPASS_ID_AK8972,
    COMPASS_ID_AMI30X,
    COMPASS_ID_AMI306,
    COMPASS_ID_YAS529,
    COMPASS_ID_YAS530,
    COMPASS_ID_HMC5883,
    COMPASS_ID_LSM303DLH,
    COMPASS_ID_LSM303DLM,
    COMPASS_ID_MMC314X,
    COMPASS_ID_HSCDTD002B,
    COMPASS_ID_HSCDTD004A,
    COMPASS_ID_AK09918,

    GYRO_ID_ALL,
    GYRO_ID_L3G4200D,
    GYRO_ID_L3G20D,
    GYRO_ID_EWTSA,
    GYRO_ID_K3G,
    GYRO_ID_MPU6500,
    GYRO_ID_MPU6880,
    GYRO_ID_LSM330,
    GYRO_ID_ICM2060X,
    GYRO_ID_SH3001,
    LIGHT_ID_ALL,
    LIGHT_ID_CM3217,
    LIGHT_ID_CM3218,
    LIGHT_ID_CM3232,
    LIGHT_ID_AL3006,
    LIGHT_ID_STK3171,
    LIGHT_ID_ISL29023,
    LIGHT_ID_AP321XX,
    LIGHT_ID_PHOTORESISTOR,
    LIGHT_ID_US5152,
    LIGHT_ID_STK3410,
    LIGHT_ID_EM3071X,

    PROXIMITY_ID_ALL,
    PROXIMITY_ID_AL3006,
    PROXIMITY_ID_STK3171,
    PROXIMITY_ID_AP321XX,
    PROXIMITY_ID_STK3410,
    PROXIMITY_ID_EM3071X,

    TEMPERATURE_ID_ALL,
    TEMPERATURE_ID_MS5607,

    PRESSURE_ID_ALL,
    PRESSURE_ID_BMA085,
    PRESSURE_ID_MS5607,

    HALL_ID_ALL,
    HALL_ID_OCH165T,

    SENSOR_NUM_ID,
};

struct sensor_axis {
    int x;
    int y;
    int z;
};

struct sensor_flag {
    atomic_t a_flag;
    atomic_t m_flag;
    atomic_t mv_flag;
    atomic_t open_flag;
    atomic_t debug_flag;
    long long delay;
    wait_queue_head_t open_wq;
};

struct sensor_operate {
    char *name;
    int type;
    int id_i2c;
    int range[2];
    int brightness[2];
    int read_reg;
    int read_len;
    int id_reg;
    int id_data;
    int precision;
    int ctrl_reg;
    int ctrl_data;
    int int_ctrl_reg;
    int int_status_reg;
    int trig;
    int (*active)(struct i2c_client *client, int enable, int rate);
    int (*init)(struct i2c_client *client);
    int (*report)(struct i2c_client *client);
    int (*suspend)(struct i2c_client *client);
    int (*resume)(struct i2c_client *client);
    struct miscdevice *misc_dev;
};

/* Platform data for the sensor */
struct sensor_private_data {
    int type;
    struct i2c_client *client;
    struct input_dev *input_dev;
    int stop_work;
    struct delayed_work delaywork;
    struct sensor_axis axis;
    char sensor_data[40];
    atomic_t is_factory;
    wait_queue_head_t is_factory_ok;
    struct mutex data_mutex;
    struct mutex operation_mutex;
    struct mutex sensor_mutex;
    struct mutex i2c_mutex;
    int status_cur;
    int start_count;
    int devid;
    struct sensor_flag flags;
    struct i2c_device_id *i2c_id;
    struct sensor_platform_data *pdata;
    struct sensor_operate *ops;
    struct file_operations fops;
    struct miscdevice miscdev;
};

struct sensor_platform_data {
    int type;
    int irq;
    int irq_pin;
    int power_pin;
    int reset_pin;
    int standby_pin;
    int irq_enable;
    int poll_delay_ms;
    int x_min;
    int y_min;
    int z_min;
    int factory;
    int layout;
    unsigned char address;
    unsigned long irq_flags;
    signed char orientation[9];
    short m_layout[4][3][3];
    int *project_name;
    int power_off_in_suspend;
    int wake_enable;
};

struct gsensor_platform_data {
    u16 model;
    u16 swap_xy;
    u16 swap_xyz;
    signed char orientation[9];
    int (*get_pendown_state)(void);
    int (*init_platform_hw)(void);
    int (*gsensor_platform_sleep)(void);
    int (*gsensor_platform_wakeup)(void);
    void (*exit_platform_hw)(void);
};

struct akm8975_platform_data {
    short m_layout[4][3][3];
    char project_name[64];
    int gpio_DRDY;
};

struct akm_platform_data {
    short m_layout[4][3][3];
    char project_name[64];
    char layout;
    char outbit;
    int gpio_DRDY;
    int gpio_RST;
};

#define DBG(x...)

#define GSENSOR_IOCTL_MAGIC			'a'
#define GBUFF_SIZE				12	/* Rx buffer size */

/* IOCTLs for MMA8452 library */
#define GSENSOR_IOCTL_INIT						_IO(GSENSOR_IOCTL_MAGIC, 0x01)
#define GSENSOR_IOCTL_RESET					_IO(GSENSOR_IOCTL_MAGIC, 0x04)
#define GSENSOR_IOCTL_CLOSE					_IO(GSENSOR_IOCTL_MAGIC, 0x02)
#define GSENSOR_IOCTL_START					_IO(GSENSOR_IOCTL_MAGIC, 0x03)
#define GSENSOR_IOCTL_GETDATA					_IOR(GSENSOR_IOCTL_MAGIC, 0x08, char[GBUFF_SIZE+1])
#define GSENSOR_IOCTL_APP_SET_RATE			_IOW(GSENSOR_IOCTL_MAGIC, 0x10, short)

#define COMPASS_IOCTL_MAGIC					'c'
/* IOCTLs for APPs */
#define ECS_IOCTL_APP_SET_MODE				_IOW(COMPASS_IOCTL_MAGIC, 0x10, short)
#define ECS_IOCTL_APP_SET_MFLAG				_IOW(COMPASS_IOCTL_MAGIC, 0x11, short)
#define ECS_IOCTL_APP_GET_MFLAG				_IOW(COMPASS_IOCTL_MAGIC, 0x12, short)
#define ECS_IOCTL_APP_SET_AFLAG				_IOW(COMPASS_IOCTL_MAGIC, 0x13, short)
#define ECS_IOCTL_APP_GET_AFLAG				_IOR(COMPASS_IOCTL_MAGIC, 0x14, short)
#define ECS_IOCTL_APP_SET_TFLAG				_IOR(COMPASS_IOCTL_MAGIC, 0x15, short)/* NOT use */
#define ECS_IOCTL_APP_GET_TFLAG				_IOR(COMPASS_IOCTL_MAGIC, 0x16, short)/* NOT use */
#define ECS_IOCTL_APP_RESET_PEDOMETER		_IOW(COMPASS_IOCTL_MAGIC, 0x17)	/* NOT use */
#define ECS_IOCTL_APP_SET_DELAY				_IOW(COMPASS_IOCTL_MAGIC, 0x18, short)
#define ECS_IOCTL_APP_SET_MVFLAG				_IOW(COMPASS_IOCTL_MAGIC, 0x19, short)
#define ECS_IOCTL_APP_GET_MVFLAG				_IOR(COMPASS_IOCTL_MAGIC, 0x1A, short)
#define ECS_IOCTL_APP_GET_DELAY				_IOR(COMPASS_IOCTL_MAGIC, 0x1B, short)

#define LIGHTSENSOR_IOCTL_MAGIC					'l'
#define LIGHTSENSOR_IOCTL_GET_ENABLED			_IOR(LIGHTSENSOR_IOCTL_MAGIC, 1, int *)
#define LIGHTSENSOR_IOCTL_ENABLE					_IOW(LIGHTSENSOR_IOCTL_MAGIC, 2, int *)
#define LIGHTSENSOR_IOCTL_SET_RATE				_IOW(LIGHTSENSOR_IOCTL_MAGIC, 3, short)

#define PSENSOR_IOCTL_MAGIC				'p'
#define PSENSOR_IOCTL_GET_ENABLED		_IOR(PSENSOR_IOCTL_MAGIC, 1, int *)
#define PSENSOR_IOCTL_ENABLE				_IOW(PSENSOR_IOCTL_MAGIC, 2, int *)
#define PSENSOR_IOCTL_DISABLE				_IOW(PSENSOR_IOCTL_MAGIC, 3, int *)

#define PRESSURE_IOCTL_MAGIC 				'r'
#define PRESSURE_IOCTL_GET_ENABLED		_IOR(PRESSURE_IOCTL_MAGIC, 1, int *)
#define PRESSURE_IOCTL_ENABLE				_IOW(PRESSURE_IOCTL_MAGIC, 2, int *)
#define PRESSURE_IOCTL_DISABLE			_IOW(PRESSURE_IOCTL_MAGIC, 3, int *)
#define PRESSURE_IOCTL_SET_DELAY			_IOW(PRESSURE_IOCTL_MAGIC, 4, int *)


#define TEMPERATURE_IOCTL_MAGIC			't'
#define TEMPERATURE_IOCTL_GET_ENABLED	_IOR(TEMPERATURE_IOCTL_MAGIC, 1, int *)
#define TEMPERATURE_IOCTL_ENABLE			_IOW(TEMPERATURE_IOCTL_MAGIC, 2, int *)
#define TEMPERATURE_IOCTL_DISABLE		_IOW(TEMPERATURE_IOCTL_MAGIC, 3, int *)
#define TEMPERATURE_IOCTL_SET_DELAY		_IOW(TEMPERATURE_IOCTL_MAGIC, 4, int *)


#define L3G4200D_IOCTL_BASE 77

#define L3G4200D_IOCTL_SET_DELAY _IOW(L3G4200D_IOCTL_BASE, 0, int)
#define L3G4200D_IOCTL_GET_DELAY _IOR(L3G4200D_IOCTL_BASE, 1, int)
#define L3G4200D_IOCTL_SET_ENABLE _IOW(L3G4200D_IOCTL_BASE, 2, int)
#define L3G4200D_IOCTL_GET_ENABLE _IOR(L3G4200D_IOCTL_BASE, 3, int)

extern int sensor_rx_data(struct i2c_client *client, char *rxData, int length);
extern int sensor_tx_data(struct i2c_client *client, char *txData, int length);
extern int sensor_write_reg(struct i2c_client *client, int addr, int value);
extern int sensor_read_reg(struct i2c_client *client, int addr);
extern int sensor_tx_data_normal(struct i2c_client *client, char *buf, int num);
extern int sensor_rx_data_normal(struct i2c_client *client, char *buf, int num);
extern int sensor_write_reg_normal(struct i2c_client *client, char value);
extern int sensor_read_reg_normal(struct i2c_client *client);

extern int jason_sensor_register_device(struct i2c_client *client,
    struct sensor_platform_data *slave_pdata,
    const struct i2c_device_id *devid,
    struct sensor_operate *ops);
extern int jason_sensor_unregister_device(struct i2c_client *client,
        struct sensor_platform_data *slave_pdata,
        struct sensor_operate *ops);
extern void jason_sensor_shutdown(struct i2c_client *client);
 
#endif