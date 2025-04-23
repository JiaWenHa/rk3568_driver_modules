#ifndef __JASON_SH30001_H__
#define __JASON_SH30001_H__

/******************************Custom Macro************************************/
#define JASON_SH3001_TRUE		(0)
#define JASON_SH3001_FALSE	    (-1)

/*****************************Register Descriptions****************************/

/* Accelerometer Data */
#define ACC_XDATA_L     (0x00)
#define ACC_XDATA_H     (0x01)
#define ACC_YDATA_L     (0x02)
#define ACC_YDATA_H     (0x03)
#define ACC_ZDATA_L     (0x04)
#define ACC_ZDATA_H     (0x05)

/* Gyroscope Data */
#define GYRO_XDATA_L    (0x06)
#define GYRO_XDATA_H    (0x07)
#define GYRO_YDATA_L    (0x08)
#define GYRO_YDATA_H    (0x09)
#define GYRO_ZDATA_L    (0x0A)
#define GYRO_ZDATA_H    (0x0B)

/* Temperature Data */
#define TEMP_DATA_L     (0x0C)
#define TEMP_DATA_H     (0x0D)

/* Chip ID */
#define CHIP_ID         (0x0F)

/* Interrupt Status */
#define INTERRUPT_STATUS_0  (0x10)
#define INTERRUPT_STATUS_1  (0x11)
#define INTERRUPT_STATUS_2  (0x12)
#define INTERRUPT_STATUS_3  (0x14)
#define INTERRUPT_STATUS_4  (0x15)

/* FIFO Status */
#define FIFO_STATUS_0       (0x16)
#define FIFO_STATUS_1       (0x17)

/* FIFO Data */
#define FIFO_DATA           (0x18)

/* Temperature Sensor Configuration */
#define TEMP_SENSOR_CONFIG_0    (0x20)
#define TEMP_SENSOR_CONFIG_1    (0x21)
#define TEMP_SENSOR_CONFIG_2    (0xD5)

/* Accelerometer Configuration */
#define ACC_CONFIG_0            (0x22)
#define ACC_CONFIG_1            (0x23)
#define ACC_CONFIG_2            (0x25)
#define ACC_CONFIG_3            (0x26)

/* Gyroscope Configuration */
#define GYRO_CONFIG_0           (0x28)
#define GYRO_CONFIG_1           (0x29)
#define GYRO_CONFIG_2           (0x2B)
#define GYRO_CONFIG_3           (0x8F)
#define GYRO_CONFIG_4           (0x9F)
#define GYRO_CONFIG_5           (0xAF)

/* SPI Configuration */
#define SPI_CONFIG              (0x32)

/* FIFO Configuration */
#define FIFO_CONFIG_0           (0x35)
#define FIFO_CONFIG_1           (0x36)
#define FIFO_CONFIG_2           (0x37)
#define FIFO_CONFIG_3           (0x38)
#define FIFO_CONFIG_4           (0x39)

/* Master i2c Configuration */
#define MI2C_CONFIG_0           (0x3A)
#define MI2C_CONFIG_1           (0x3B)

/* Master i2c Command */
#define MI2C_COMM_0             (0x3C)
#define MI2C_COMM_1             (0x3D)

/* Master i2c Write Data */
#define MI2C_WRT_DATA           (0x3E)

/* Master i2c Read Out Data */
#define MI2C_RAD_DATA           (0x3F)

/* Interrupt Enable */
#define INTERRUPT_EN_0          (0x40)
#define INTERRUPT_EN_1          (0x41)

/* Interrupt Configuration */
#define INTERRUPT CONFIG        (0x44)

/* Interrupt Count Limit */
#define INTERRUPT_CONT_LIM      (0x45)

/* Orientation Interrupt Configuration */
#define ORIENT_INT_CONFIG_0     (0x46)
#define ORIENT_INT_CONFIG_1     (0x47)
#define ORIENT_INT_CONFIG_2     (0x48)
#define ORIENT_INT_CONFIG_3     (0x49)
#define ORIENT_INT_CONFIG_4     (0x4A)
#define ORIENT_INT_CONFIG_5     (0x4B)
#define ORIENT_INT_CONFIG_6     (0x4C)
#define ORIENT_INT_CONFIG_7     (0x4D)

/* Flat Interrupt Configuration */
#define FLAT_INT_CONFIG         (0x4E)

/* Activity & Inactivity Interrupt Configuration */
#define ACT_INACT_CONFIG_0      (0x4F)
#define TAP_ACT_INACT_CONFIG    (0x50)
#define ACT_INT_THR             (0x55)
#define ACT_INT_TIME            (0x56)
#define INACT_INT_THR_L         (0x57)
#define INACT_INT_TIME          (0x58)
#define INACT_INT_THR_M         (0x7B)
#define INACT_INT_THR_H         (0x7C)
#define INACT_INT_1G_L          (0x7D)
#define INACT_INT_1G_H          (0x7E)

/* Tap Interrupt Configuration */
#define TAP_ACT_INACT_CONFIG    (0x50)
#define TAP_INT_THR             (0x51)
#define TAP_INT_DUR             (0x52)
#define TAP_INT_LAT             (0x53)
#define DOUBLETAP_INT_WIN       (0x54)

/* High-G & Low-G Interrupt Configuration */
#define H_L_G_INT_CONFIG_0      (0x59)
#define HIGH_G_INT_THR          (0x5A)
#define HIGH_G_INT_TIME         (0x5B)
#define LOW_G_INT_THR           (0x5C)
#define LOW_G_INT_TIME          (0x5D)

/* Free Fall Interrupt Configuration */
#define FREE_FALL_INT_THR       (0x5E)
#define FREE_FALL_INT_TIME      (0x5F)

/* Interrupt Pin Mapping */
#define INT_PINMP_0             (0x79)
#define INT_PINMP_1             (0x7A)

/* SPI Register Access */
#define SPI_CONFIG_1            (0x7F)

/* Auxiliary I2C Configuration */
#define AUX_I2C_CONFIG          (0xFD)

/*****************************Register Configuration****************************/

/************************** Accelerometer Configuration *********************/
/* ACC_CONFIG_0 位字段选项 */
typedef enum {
    ACC_WORK_MODE_NORMAL = 0, // 正常模式
    ACC_WORK_MODE_LOW_POWER = 1 // 低功耗模式
} AccWorkMode;

typedef enum {
    ACC_DITHER_DISABLE = 0, // 禁用
    ACC_DITHER_ENABLE = 1 // 启用
} AccDither;

typedef enum {
    ACC_DIGITAL_FILTER_DISABLE = 0, // 禁用
    ACC_DIGITAL_FILTER_ENABLE = 1  // 启用
} AccDigitalFilter;

/* ACC_CONFIG_1 位字段选项 */
typedef enum {
    ACC_ODR_1000HZ = 0x0000,
    ACC_ODR_500HZ  = 0x0001,
    ACC_ODR_250HZ  = 0x0010,
    ACC_ODR_125HZ  = 0x0100,
    ACC_ODR_63HZ   = 0x0101,
    ACC_ODR_31HZ   = 0x0110,
    ACC_ODR_16HZ   = 0x1000,
    ACC_ODR_2000HZ = 0x1100,
    ACC_ODR_4000HZ = 0x1101,
    ACC_ODR_8000HZ = 0x1110
} AccODR;

/* ACC_CONFIG_2 位字段选项 */
typedef enum {
    ACC_RANGE_16G  = 0x010,
    ACC_RANGE_8G   = 0x011,
    ACC_RANGE_4G   = 0x100,
    ACC_RANGE_2G   = 0x101
} AccRange;

/* ACC_CONFIG_3 位字段选项 */
typedef enum {
    ACC_LPF_CUTOFF_0_40  = 0x000, // ODR × 0.40
    ACC_LPF_CUTOFF_0_25  = 0x001, // ODR × 0.25
    ACC_LPF_CUTOFF_0_11  = 0x010, // ODR × 0.11
    ACC_LPF_CUTOFF_0_04  = 0x011, // ODR × 0.04
    ACC_LPF_CUTOFF_0_02  = 0x100  // ODR × 0.02
} AccLPFCutoff;

typedef enum {
    ACC_BYPASS_LPF_NO  = 0,  // 不旁路
    ACC_BYPASS_LPF_YES = 1   // 旁路
} AccBypassLPF;

/* 加速度计寄存器配置结构体 */
typedef struct {
    AccWorkMode workMode;         // ACC_CONFIG_0 [7]
    AccDither dither;             // ACC_CONFIG_0 [6]
    AccDigitalFilter digitalFilter; // ACC_CONFIG_0 [0]
    AccODR odr;                   // ACC_CONFIG_1 [3:0]
    AccRange range;               // ACC_CONFIG_2 [2:0]
    AccLPFCutoff lpfCutoff;       // ACC_CONFIG_3 [7:5]
    AccBypassLPF bypassLPF;       // ACC_CONFIG_3 [3]
} AccConfig;


/************************* Gyroscope Configuration *************************/

/* GYRO_CONFIG_0 位字段选项 */
typedef enum {
    GYRO_SHUT_DOWN_NO = 0,   // 当检测到无活动中断时，不关闭
    GYRO_SHUT_DOWN_YES = 1   // 当检测到无活动中断时，关闭
} GyroShutDown;

typedef enum {
    GYRO_DIGITAL_FILTER_DISABLE = 0,  // 禁用
    GYRO_DIGITAL_FILTER_ENABLE = 1    // 启用
} GyroDigitalFilter;

/* GYRO_CONFIG_1 位字段选项 */
typedef enum {
    GYRO_ODR_1000HZ  = 0x0000,
    GYRO_ODR_500HZ   = 0x0001,
    GYRO_ODR_250HZ   = 0x0010,
    GYRO_ODR_125HZ   = 0x0011,
    GYRO_ODR_63HZ    = 0x0100,
    GYRO_ODR_31HZ    = 0x0101,
    GYRO_ODR_2KHZ    = 0x1000,
    GYRO_ODR_4KHZ    = 0x1001,
    GYRO_ODR_8KHZ    = 0x1010,
    GYRO_ODR_16KHZ   = 0x1011,
    GYRO_ODR_32KHZ   = 0x1100
} GyroODR;

/* GYRO_CONFIG_2 位字段选项 */
typedef enum {
    GYRO_LPF_BYPASS_DISABLE = 0,  // 不旁路
    GYRO_LPF_BYPASS_ENABLE = 1    // 旁路
} GyroLPFBypass;

typedef enum {
    GYRO_LPF_CUTOFF_00 = 0x00,  // 对应表中值 00
    GYRO_LPF_CUTOFF_01 = 0x01,  // 对应表中值 01
    GYRO_LPF_CUTOFF_10 = 0x02,  // 对应表中值 10
    GYRO_LPF_CUTOFF_11 = 0x03   // 对应表中值 11
} GyroLPFCutoff;

/* GYRO_CONFIG_3, GYRO_CONFIG_4, GYRO_CONFIG_5 位字段选项 */
typedef enum {
    GYRO_FSR_125DPS  = 0x010,  // 125dps
    GYRO_FSR_250DPS  = 0x011,  // 250dps
    GYRO_FSR_500DPS  = 0x100,  // 500dps
    GYRO_FSR_1000DPS = 0x101,  // 1000dps
    GYRO_FSR_2000DPS = 0x110   // 2000dps
} GyroFSR;

/* 陀螺仪配置结构体 */
typedef struct {
    GyroShutDown shutDown;         // GYRO_CONFIG_0 [4]
    GyroDigitalFilter digitalFilter; // GYRO_CONFIG_0 [0]
    GyroODR odr;                   // GYRO_CONFIG_1 [3:0]
    GyroLPFBypass lpfBypass;       // GYRO_CONFIG_2 [4]
    GyroLPFCutoff lpfCutoff;       // GYRO_CONFIG_2 [3:2]
    GyroFSR fsrX;                  // GYRO_CONFIG_3 [2:0]
    GyroFSR fsrY;                  // GYRO_CONFIG_4 [2:0]
    GyroFSR fsrZ;                  // GYRO_CONFIG_5 [2:0]
} GyroConfig;

#endif