obj-m += jason_sensor_dev.o
obj-m += jason_sh3001_acc.o
obj-m += jason_sh3001_gyro.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o jason_sh3001_test jason_sh3001_test.c

app:
	gcc -o jason_sh3001_test jason_sh3001_test.c

copy:
	rm -rf /lib/modules/4.19.232/*.ko
	cp ./*.ko /lib/modules/4.19.232/
	depmod -a


clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf jason_sh3001_test