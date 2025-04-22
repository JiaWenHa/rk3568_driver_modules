#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>

int main(void)
{
    int fd, ret;
    struct input_event event;

    /* 打开设备节点 */
    fd = open("/dev/input/event7", O_RDWR);
    if(fd < 0){
        printf("open /dev/input/event7 error.\n");
        return -1;
    }

    while (1)
    {
        /* 获取上报的数据 */
        ret = read(fd, &event, sizeof(struct input_event));
        if(ret < 0) {
            printf("read file error.\n");
            return -2;
        }

        /* 解析数据 */
        if(event.type == EV_KEY){
            if(event.code == KEY_1){
                if(event.value == 1){
                    printf("event value is 1\n");
                } else {
                    printf("event value is 0\n");
                }
            }
        }
    }
    
    return 0;
}