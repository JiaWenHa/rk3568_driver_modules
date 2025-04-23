#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/input.h>

#define GSENSOR_IOCTL_MAGIC 'a'
#define GSENSOR_IOCTL_START _IO(GSENSOR_IOCTL_MAGIC, 0x03)
#define GSENSOR_IOCTL_GETDATA _IOR(GSENSOR_IOCTL_MAGIC, 0x08, char[12+1])
#define GSENSOR_IOCTL_CLOSE _IO(GSENSOR_IOCTL_MAGIC, 0x02)

struct sensor_axis {
    int x;
    int y;
    int z;
};

int main() {
    int fd;
    struct sensor_axis axis;
    int ret;

    // 打开设备文件
    fd = open("/dev/mma8452_daemon", O_RDWR);
    if (fd < 0) {
        perror("Failed to open /dev/mma8452_daemon");
        return -1;
    }

    // 发送 GSENSOR_IOCTL_START 命令
    ret = ioctl(fd, GSENSOR_IOCTL_START, NULL);
    if (ret < 0) {
        perror("Failed to send GSENSOR_IOCTL_START");
        close(fd);
        return -1;
    }
    printf("Sensor started\n");

    // 循环读取数据
    for (int i = 0; i < 100; i++) {
        ret = ioctl(fd, GSENSOR_IOCTL_GETDATA, &axis);
        if (ret < 0) {
            perror("Failed to get data");
            break;
        }
        printf("Acceleration: x=%d, y=%d, z=%d\n", axis.x, axis.y, axis.z);
        sleep(1);  // 每秒读取一次
    }

    // 发送 GSENSOR_IOCTL_CLOSE 命令
    ret = ioctl(fd, GSENSOR_IOCTL_CLOSE, NULL);
    if (ret < 0) {
        perror("Failed to send GSENSOR_IOCTL_CLOSE");
    }
    printf("Sensor stopped\n");

    close(fd);
    return 0;
}