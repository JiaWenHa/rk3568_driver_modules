obj-m += jason_input.o


all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) -o jason_input_app jason_input_app.c

app:
	$(CC) -o jason_input_app jason_input_app.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f jason_input_app