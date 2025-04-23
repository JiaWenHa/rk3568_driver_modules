obj-m += jason_sh3001_acc.o
obj-m += jason_sh3001_gyro.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	gcc -o jason_sh3001_test jason_sh3001_test.c

app:
	gcc -o jason_sh3001_test jason_sh3001_test.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf jason_sh3001_test