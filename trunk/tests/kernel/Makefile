ifneq ($(KERNELRELEASE),)
ifneq ($(CONFIG_LTT),)
	#obj-m += ltt-facility-loader-tests.o
#	#obj-m += test-time-probe.o 
	#obj-m += test-time-probe2.o 
	#obj-m += test-compact.o 
#	obj-m += ltt-facility-loader-compact.o
#	obj-m += test-instrument-size-small.o
#	obj-m += test-instrument-size-med.o
#	obj-m += test-instrument-size-big.o
#	obj-m += test-printk-effect.o
	#obj-m += ltt-probe-tests.o
	#obj-m += test-time-probe3.o
endif
	#obj-m += test-sys_call.o
#	obj-m += test-bug.o
	obj-m += test-tsc-sync2.o
	obj-m += test-nop-speed.o
	#obj-m += test-hpet.o
#	obj-m += test-prefix-speed-32.o
	#obj-m += test-prefix-speed.o
	#obj-m += test-psrwlock.o
#	obj-m += test-cmpxchg-nolock2.o
#	obj-m += test-trace-speed.o
#	obj-m += test-rcu-speed.o
#	obj-m += test-fct-speed.o
#	obj-m += test-int3-speed.o
#	obj-m += test-kprobes2.o
	#obj-m += test-mark-speed.o
	#obj-m += test-mark-speed-edit.o
	#obj-m += test-mark-speed-opt.o
#	obj-m += toggle-tester.o
	#obj-m += test-slub.o
	#obj-m += test-slub2.o
	#obj-m += test-showval.o
	#obj-m += cond_call.o
	#obj-m += cond_call2.o
#	obj-m += test-irq-latency.o
#	obj-m += test-kprobes2.o
#	obj-m += test-imval.o
	#obj-m += test-imval-bug.o
#	obj-m += test-imvalw.o
	#obj-m += list-mark.o
	#obj-m += test-rodata.o
	#obj-m += test-tlb.o
	#obj-m += test-init.o
	#obj-m += probe-example.o
	#obj-m += marker-example.o
#	obj-m += test-async-tsc.o
	#obj-m += test-tsc-sync.o
	#obj-m += test-tsc.o
#	obj-m += test-hpet.o
	#obj-m += test-debugfs.o
#	obj-m += test-mod.o
#	obj-m += atomic-pIII-cmx.o
	#obj-m += test-hotplug.o
	#obj-m += rdtsc-smp.o
	#obj-m += test-local.o
	#obj-m += test-ser.o
#	obj-m += test-cmpxchg.o
#	obj-m += test-cmpxchg-nolock.o
	#obj-m += test-cmpxchg-nolock2.o
	#obj-m += test-cmpxchg8b.o
#	obj-m += test-spinlock.o
#	obj-m += test-inc.o
#	obj-m += test-inc-nolock.o
#	obj-m += test-kmalloc.o
#	obj-m += test-irqdis.o
#	obj-m += test-int3.o
	#obj-m += test-kprobes.o
	#obj-m += test-printk.o
	#obj-m += test-debugfs.o
	#obj-m += test-mark.o
	#obj-m += test-markxmc.o
#	obj-m += probe.o
#	obj-m += test-irqloop.o
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
