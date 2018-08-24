obj-m += simple.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	rm -rf *.mod *.o *.mod.c *.ko *.symvers *.order tmp_versions *.cmd
