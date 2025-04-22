obj-m += jason_sh3001_acc.o
obj-m += jason_sh3001_gyro.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean