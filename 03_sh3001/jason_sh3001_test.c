#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#define GSENSOR_IOCTL_MAGIC			'a'
#define GBUFF_SIZE				12	/* Rx buffer size */

/* IOCTLs for MMA8452 library */
#define GSENSOR_IOCTL_INIT						_IO(GSENSOR_IOCTL_MAGIC, 0x01)
#define GSENSOR_IOCTL_RESET					_IO(GSENSOR_IOCTL_MAGIC, 0x04)
#define GSENSOR_IOCTL_CLOSE					_IO(GSENSOR_IOCTL_MAGIC, 0x02)
#define GSENSOR_IOCTL_START					_IO(GSENSOR_IOCTL_MAGIC, 0x03)
#define GSENSOR_IOCTL_GETDATA					_IOR(GSENSOR_IOCTL_MAGIC, 0x08, char[GBUFF_SIZE+1])
#define GSENSOR_IOCTL_APP_SET_RATE			_IOW(GSENSOR_IOCTL_MAGIC, 0x10, short)
#define GSENSOR_IOCTL_GET_CALIBRATION		_IOR(GSENSOR_IOCTL_MAGIC, 0x11, int[3])

#define L3G4200D_IOCTL_BASE 77

#define L3G4200D_IOCTL_SET_DELAY _IOW(L3G4200D_IOCTL_BASE, 0, int)
#define L3G4200D_IOCTL_GET_DELAY _IOR(L3G4200D_IOCTL_BASE, 1, int)
#define L3G4200D_IOCTL_SET_ENABLE _IOW(L3G4200D_IOCTL_BASE, 2, int)
#define L3G4200D_IOCTL_GET_ENABLE _IOR(L3G4200D_IOCTL_BASE, 3, int)
#define L3G4200D_IOCTL_GET_CALIBRATION _IOR(L3G4200D_IOCTL_BASE, 4, int[3])

#define ACCEL_DEVICE "/dev/mma8452_daemon"
#define GYRO_DEVICE "/dev/gyrosensor"
#define TEST_SAMPLES 10
#define DEFAULT_RATE 100 // ms

struct sensor_axis {
    int x;
    int y;
    int z;
};

void print_axis_data(const char *sensor_name, struct sensor_axis *axis) {
    printf("%s Data: X=%d, Y=%d, Z=%d\n", sensor_name, axis->x, axis->y, axis->z);
}

int test_sensor(int fd, const char *sensor_name, int is_gyro) {
    int ret;
    struct sensor_axis axis;
    int calibration[3];
    short rate = DEFAULT_RATE;
    int enable = 1;
    int i;

    // Set sampling rate
    printf("Setting %s rate to %dms\n", sensor_name, rate);
    ret = ioctl(fd, is_gyro ? L3G4200D_IOCTL_SET_DELAY : GSENSOR_IOCTL_APP_SET_RATE, &rate);
    if (ret < 0) {
        perror("Failed to set rate");
        return -1;
    }

    // Enable sensor
    printf("Enabling %s\n", sensor_name);
    ret = ioctl(fd, is_gyro ? L3G4200D_IOCTL_SET_ENABLE : GSENSOR_IOCTL_START, &enable);
    if (ret < 0) {
        perror("Failed to enable sensor");
        return -1;
    }

    // Read sensor data
    printf("Reading %s data (%d samples)...\n", sensor_name, TEST_SAMPLES);
    for (i = 0; i < TEST_SAMPLES; i++) {
        ret = ioctl(fd, GSENSOR_IOCTL_GETDATA, &axis);
        if (ret < 0) {
            perror("Failed to read sensor data");
            return -1;
        }
        print_axis_data(sensor_name, &axis);
        usleep(rate * 1000);
    }

    // Read calibration data
    printf("Reading %s calibration data\n", sensor_name);
    ret = ioctl(fd, is_gyro ? L3G4200D_IOCTL_GET_CALIBRATION : GSENSOR_IOCTL_GET_CALIBRATION, calibration);
    if (ret < 0) {
        perror("Failed to read calibration data");
    } else {
        printf("%s Calibration: X=%d, Y=%d, Z=%d\n", sensor_name, 
               calibration[0], calibration[1], calibration[2]);
    }

    // Disable sensor
    enable = 0;
    printf("Disabling %s\n", sensor_name);
    ret = ioctl(fd, is_gyro ? L3G4200D_IOCTL_SET_ENABLE : GSENSOR_IOCTL_CLOSE, &enable);
    if (ret < 0) {
        perror("Failed to disable sensor");
        return -1;
    }

    return 0;
}

int trigger_calibration(const char *sensor_type) {
    char buf[16];
    int fd;
    ssize_t ret;

    snprintf(buf, sizeof(buf), "/sys/class/sensor_class/%s_calibration", sensor_type);
    fd = open(buf, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open calibration file");
        return -1;
    }

    ret = write(fd, "1", 1);
    if (ret < 0) {
        perror("Failed to trigger calibration");
        close(fd);
        return -1;
    }

    close(fd);
    printf("%s calibration triggered\n", sensor_type);
    return 0;
}

int main(int argc, char *argv[]) {
    int accel_fd, gyro_fd;
    int calibrate = 0;

    // Check for calibration flag
    if (argc > 1 && strcmp(argv[1], "--calibrate") == 0) {
        calibrate = 1;
    }

    // Open accelerometer device
    accel_fd = open(ACCEL_DEVICE, O_RDWR);
    if (accel_fd < 0) {
        perror("Failed to open accelerometer device");
        return -1;
    }

    // Open gyroscope device
    gyro_fd = open(GYRO_DEVICE, O_RDWR);
    if (gyro_fd < 0) {
        perror("Failed to open gyroscope device");
        close(accel_fd);
        return -1;
    }

    // Trigger calibration if requested
    if (calibrate) {
        if (trigger_calibration("accel") < 0 || trigger_calibration("gyro") < 0) {
            close(accel_fd);
            close(gyro_fd);
            return -1;
        }
        // Wait for calibration to complete
        sleep(2);
    }

    // Test accelerometer
    printf("\n=== Testing Accelerometer ===\n");
    if (test_sensor(accel_fd, "Accelerometer", 0) < 0) {
        printf("Accelerometer test failed\n");
    } else {
        printf("Accelerometer test completed successfully\n");
    }

    // Test gyroscope
    printf("\n=== Testing Gyroscope ===\n");
    if (test_sensor(gyro_fd, "Gyroscope", 1) < 0) {
        printf("Gyroscope test failed\n");
    } else {
        printf("Gyroscope test completed successfully\n");
    }

    // Clean up
    close(accel_fd);
    close(gyro_fd);
    return 0;
}