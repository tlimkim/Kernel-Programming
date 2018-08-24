obj-m += simple-5.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
test:
	sudo dmesg -C
	sudo insmod simple-5.ko
	sudo rmmod simple-5.ko
	dmesg
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -rf *.mod *.o *.mod.c *.ko *.symvers *.order tmp_versions *.cmd
