#ifndef __JASON_SH30001_H__
#define __JASON_SH30001_H__

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

#endif