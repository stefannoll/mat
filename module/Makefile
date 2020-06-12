SRCS := module.c utilities.c flags.c rangecounter.c corebuffer.c
### name of module
module_name := memory_address_tracer
obj-m += $(module_name).o
$(module_name)-objs := $(SRCS:.c=.o)
### class and device name of the module
class_name := memory_address_tracer
device_name := memory_address_tracer
EXTRA_CFLAGS += -DCLASS_NAME=\"$(class_name)\" -DDEVICE_NAME=\"$(device_name)\"

.PHONY: clean info check load unload log

all:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build/ M=$(PWD) clean

info:
	modinfo $(module_name).ko

check:
	lsmod | grep $(module_name) | cat
	cat /proc/devices | grep $(device_name) | cat
	ls -l /sys/module/$(module_name)
	ls -l /sys/devices/virtual/$(class_name)
	ls -l /sys/devices/virtual/$(class_name)/$(device_name)

load:
	### remove module if loaded (ignore error)
	-sudo rmmod $(module_name)
	sudo insmod $(module_name).ko

unload:
	### load module into running kernel
	sudo rmmod $(module_name).ko

log:
	### display the kernel log
	-sudo dmesg -H
