#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#define SENSOR_ACCEL_IOCTL_MAGIC			'a'
#define GBUFF_SIZE				12	/* Rx buffer size */

/* IOCTLs for sensir accell library */
#define SENSOR_ACCEL_IOCTL_CLOSE					_IO(SENSOR_ACCEL_IOCTL_MAGIC, 0x02)
#define SENSOR_ACCEL_IOCTL_START					_IO(SENSOR_ACCEL_IOCTL_MAGIC, 0x03)
#define SENSOR_ACCEL_IOCTL_GETDATA					_IOR(SENSOR_ACCEL_IOCTL_MAGIC, 0x08, char[GBUFF_SIZE+1])
#define SENSOR_ACCEL_IOCTL_SET_RATE			_IOW(SENSOR_ACCEL_IOCTL_MAGIC, 0x10, short)


#define SENSOR_GYRO_IOCTL_MAGIC			'g'
#define GBUFF_SIZE				12	/* Rx buffer size */

/* IOCTLs for sensor accel library */
#define SENSOR_GYRO_IOCTL_CLOSE					_IO(SENSOR_GYRO_IOCTL_MAGIC, 0x02)
#define SENSOR_GYRO_IOCTL_START					_IO(SENSOR_GYRO_IOCTL_MAGIC, 0x03)
#define SENSOR_GYRO_IOCTL_GETDATA					_IOR(SENSOR_GYRO_IOCTL_MAGIC, 0x08, char[GBUFF_SIZE+1])
#define SENSOR_GYRO_IOCTL_SET_RATE			        _IOW(SENSOR_GYRO_IOCTL_MAGIC, 0x10, short)

#define ACCEL_DEVICE "/dev/sensor_accel"
#define GYRO_DEVICE "/dev/sensor_gyro"
#define TEST_SAMPLES 10
#define DEFAULT_RATE 30 // ms

struct sensor_axis {
    int x;
    int y;
    int z;
};

int read_sensor(int fd, const char *sensor_name, int is_gyro);
int start_sensor(int fd, const char *sensor_name, int is_gyro);
int close_sensor(int fd, const char *sensor_name, int is_gyro);

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
    ret = ioctl(fd, is_gyro ? SENSOR_GYRO_IOCTL_SET_RATE : SENSOR_ACCEL_IOCTL_SET_RATE, &rate);
    if (ret < 0) {
        perror("Failed to set rate");
        return -1;
    }

    // Enable sensor
    printf("Enabling %s\n", sensor_name);
    ret = ioctl(fd, is_gyro ? SENSOR_GYRO_IOCTL_START : SENSOR_ACCEL_IOCTL_START, &enable);
    if (ret < 0) {
        perror("Failed to enable sensor");
        return -1;
    }

    // Read sensor data
    printf("Reading %s data (%d samples)...\n", sensor_name, TEST_SAMPLES);
    for (i = 0; i < TEST_SAMPLES; i++) {
        ret = ioctl(fd, is_gyro ? SENSOR_GYRO_IOCTL_GETDATA : SENSOR_ACCEL_IOCTL_GETDATA, &axis);
        if (ret < 0) {
            perror("Failed to read sensor data");
            return -1;
        }
        print_axis_data(sensor_name, &axis);
        usleep(rate * 1000);
    }

    // Disable sensor
    enable = 0;
    printf("Disabling %s\n", sensor_name);
    ret = ioctl(fd, is_gyro ? SENSOR_GYRO_IOCTL_CLOSE : SENSOR_ACCEL_IOCTL_CLOSE, &enable);
    if (ret < 0) {
        perror("Failed to disable sensor");
        return -1;
    }

    return 0;
}

int start_sensor(int fd, const char *sensor_name, int is_gyro) {
    int ret;
    int enable = 1;

    // Enable sensor
    printf("Enabling %s\n", sensor_name);
    ret = ioctl(fd, is_gyro ? SENSOR_GYRO_IOCTL_START : SENSOR_ACCEL_IOCTL_START, &enable);
    if (ret < 0) {
        perror("Failed to enable sensor");
        return -1;
    }

    return 0;
}

int close_sensor(int fd, const char *sensor_name, int is_gyro) {
    int ret;
    int enable = 1;

    // Disable sensor
    enable = 0;
    printf("Disabling %s\n", sensor_name);
    ret = ioctl(fd, is_gyro ? SENSOR_GYRO_IOCTL_CLOSE : SENSOR_ACCEL_IOCTL_CLOSE, &enable);
    if (ret < 0) {
        perror("Failed to disable sensor");
        return -1;
    }

    return 0;
}

int read_sensor(int fd, const char *sensor_name, int is_gyro) {
    int ret;
    struct sensor_axis axis;

    // Read sensor data
    ret = ioctl(fd, is_gyro ? SENSOR_GYRO_IOCTL_GETDATA : SENSOR_ACCEL_IOCTL_GETDATA, &axis);
    if (ret < 0) {
        perror("Failed to read sensor data");
        return -1;
    }
    print_axis_data(sensor_name, &axis);

    return 0;
}

int main(int argc, char *argv[]) {
    int accel_fd, gyro_fd;

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

    start_sensor(accel_fd, "Accelerometer", 0);
    start_sensor(gyro_fd, "Gyroscope", 1);
    
    while(1){
        read_sensor(accel_fd, "Accelerometer", 0);
        read_sensor(gyro_fd, "Gyroscope", 1);
        usleep(1000 * 100); // 睡眠 100 毫秒
    }


    close_sensor(accel_fd, "Accelerometer", 0);
    close_sensor(gyro_fd, "Gyroscope", 1);

    // Clean up
    close(accel_fd);
    close(gyro_fd);
    return 0;
}