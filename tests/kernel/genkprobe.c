#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>

MODULE_LICENSE("GPL");

static int kph(struct kprobe *kp, struct pt_regs *regs)
{
	return 0;
}
static int kpfh(struct kprobe *kp, struct pt_regs *regs, int nr)
{
  printk("fault occurred on kprobes at %p(@%lx:%d)\n", kp->addr, regs->ip, nr);
	return 0;
}
static struct kprobe kp[] = {
[0]={.pre_handler=kph, .fault_handler=kpfh, .symbol_name="sys_accept"},
[1]={.pre_handler=kph, .fault_handler=kpfh, .symbol_name="sys_access"},
[2]={.pre_handler=kph, .fault_handler=kpfh, .symbol_name="sys_acct"},
[3]={.pre_handler=kph, .fault_handler=kpfh, .symbol_name="sys_add_key"},
[4]={.pre_handler=kph, .fault_handler=kpfh, .symbol_name="sys_adjtimex"},
[5]={.pre_handler=kph, .fault_handler=kpfh, .symbol_name="sys_alarm"},
[6]={.pre_handler=kph, .fault_handler=kpfh, .symbol_name="sys_bdflush"},
};
#define NRPB 7

static struct kprobe *kps[NRPB];

int __gen_init(void)
{
	int ret, i;
	for (i=0;i<NRPB;i++)
		kps[i]=&kp[i];
	printk("registering...");
	ret = register_kprobes(kps, NRPB);
	if (ret) {
		printk("failed to register kprobes\n");
		return ret;
	}
	printk("registered\n");
	return 0;
}

void __gen_exit(void)
{
	printk("unregistering...");
	unregister_kprobes(kps, NRPB);
	printk("unregistered\n");
}

module_init(__gen_init);
module_exit(__gen_exit);
