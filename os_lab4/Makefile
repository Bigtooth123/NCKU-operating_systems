KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

obj-m += osfs.o

osfs-objs := super.o inode.o file.o dir.o osfs_init.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

load:
	@sudo insmod osfs.ko
	
unload:
	@sudo umount -f mnt
	@sudo rm -rf mnt
	@sudo rmmod osfs.ko
test:
	mkdir mnt || true
	$(MAKE) all
	$(MAKE) load
	sudo mount -t osfs none mnt/