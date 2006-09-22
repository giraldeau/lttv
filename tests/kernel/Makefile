ifneq ($(KERNELRELEASE),)
ifneq ($(CONFIG_LTT),)
#	obj-m += ltt-facility-loader-tests.o
#	obj-m += test-time-probe.o 
#	obj-m += test-time-probe2.o 
#	obj-m += test-instrument-size-small.o
#	obj-m += test-instrument-size-med.o
#	obj-m += test-instrument-size-big.o
#	obj-m += test-printk-effect.o
endif
#	obj-m += test-cmpxchg.o
#	obj-m += test-cmpxchg-nolock.o
#	obj-m += test-spinlock.o
#	obj-m += test-inc.o
#	obj-m += test-inc-nolock.o
#	obj-m += test-kmalloc.o
#	obj-m += test-irqdis.o
#	obj-m += test-int3.o
	#obj-m += test-kprobes.o
	#obj-m += test-printk.o
	#obj-m += test-debugfs.o
	obj-m += test-mark.o
	obj-m += test-mark-instrumented.o
	obj-m += probe.o
	#obj-m += marker-loader.o
	#obj-m += test-linuxtrace.o

	CFLAGS_test-mark-instrumented.o += -DFORCE_MARK_JUMP_INLINE
	CFLAGS_test-mark.o += -funroll-all-loops
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)
	KERNELRELEASE = $(shell cat $(KERNELDIR)/$(KBUILD_OUTPUT)/include/linux/version.h | sed -n 's/.*UTS_RELEASE.*\"\(.*\)\".*/\1/p')
ifneq ($(INSTALL_MOD_PATH),)
	DEPMOD_OPT := -b $(INSTALL_MOD_PATH)
endif

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install
	if [ -f $(KERNELDIR)/$(KBUILD_OUTPUT)/System.map ] ; then /sbin/depmod -ae -F $(KERNELDIR)/$(KBUILD_OUTPUT)/System.map $(DEPMOD_OPT) $(KERNELRELEASE) ; fi


clean:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif
