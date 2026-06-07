# 文件作用：使用内核 kbuild 编译 oslab_monitor.ko。
# 设计原因：内核模块必须交给当前内核构建系统处理，避免手写 gcc 命令导致 ABI 不匹配。

obj-m += oslab_monitor.o

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

.PHONY: all clean

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
