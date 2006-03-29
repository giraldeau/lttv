ifneq ($(KERNELRELEASE),)
ifneq ($(CONFIG_LTT),)
	obj-m := test-time-probe.o test-l1-hit-probe.o test-int-response.o test-instrument-size-small.o test-instrument-size-med.o test-instrument-size-big.o test-time-precision.o
endif

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
