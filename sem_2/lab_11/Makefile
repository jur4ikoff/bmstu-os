CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)

TARGET = fortune

obj-m := src/$(TARGET).o
ccflags-y += -I$(M)/inc

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean: 
	$(MAKE) -C $(KDIR) M=$(PWD) clean
