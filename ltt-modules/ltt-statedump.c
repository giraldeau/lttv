/* ltt-state-dump.c
 *
 * Lunix Trace Toolkit Kernel State Dump
 *
 * Copyright 2005 -
 * Jean-Hugues Deschenes <jean-hugues.deschenes@polymtl.ca>
 *
 * Changes:
 *     Eric Clement:            add listing of network IP interface
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ltt-core.h>
#include <linux/netlink.h>
#include <linux/inet.h>
#include <linux/ip.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cpu.h>
#include <linux/ltt/ltt-facility-statedump.h>

#define NB_PROC_CHUNK 20

#include <linux/netdevice.h>
#include <linux/inetdevice.h>

/* in modules.c */
extern int ltt_enumerate_modules(void);

static inline int ltt_enumerate_network_ip_interface(void)
{
  struct net_device *list;
  struct in_device *in_dev = NULL;
  struct in_ifaddr *ifa = NULL;
	
  read_lock(&dev_base_lock);
  for(list=dev_base; list != NULL; list=list->next) {

    if(list->flags & IFF_UP) {  
      in_dev = in_dev_get(list);

      if(in_dev) {
        for (ifa = in_dev->ifa_list; ifa != NULL; ifa = ifa->ifa_next) {
          trace_statedump_enumerate_network_ip_interface(list->name, 
																											  	ifa->ifa_address,
																													LTTNG_UP);
				}
				in_dev_put(in_dev);    
      }
    } else {
      trace_statedump_enumerate_network_ip_interface(list->name,
																													0,
																													LTTNG_DOWN);
		}
  }
  read_unlock(&dev_base_lock);
  
  return 0;
}

static inline int ltt_enumerate_file_descriptors(void)
{
  struct task_struct * t = &init_task;
	unsigned int	i;
	struct file * 		filp;
	char				*tmp = (char*)__get_free_page(GFP_KERNEL), *path;
  struct fdtable *fdt;

	/* Enumerate active file descriptors */

	do {
	  read_lock(&tasklist_lock);
    if(t != &init_task) atomic_dec(&t->usage);
	  t = next_task(t);
    atomic_inc(&t->usage);
    read_unlock(&tasklist_lock);
   
		task_lock(t);
		if (t->files) {
			spin_lock(&t->files->file_lock);
			fdt = files_fdtable(t->files);
			for (i=0; i < fdt->max_fds; i++) {
				filp = fcheck_files(t->files, i);
				if (!filp)
				 continue;
				path = d_path(filp->f_dentry, filp->f_vfsmnt, tmp, PAGE_SIZE);

				/* Make sure we give at least some info */
				if(IS_ERR(path))
					trace_statedump_enumerate_file_descriptors(filp->f_dentry->d_name.name, t->pid, i);
				else
				 trace_statedump_enumerate_file_descriptors(path, t->pid, i);
		 }
		 spin_unlock(&t->files->file_lock);
		}
		task_unlock(t);

  } while( t != &init_task );

	free_page((unsigned long)tmp);
	
	return 0;
}

static inline int ltt_enumerate_vm_maps(void)
{
	struct mm_struct *mm;
  struct task_struct *  t = &init_task;
	struct vm_area_struct *	map;
	unsigned long			ino = 0;
	
	do {
    read_lock(&tasklist_lock);
    if(t != &init_task) atomic_dec(&t->usage);
	  t = next_task(t);
    atomic_inc(&t->usage);
    read_unlock(&tasklist_lock);
    
		/* get_task_mm does a task_lock... */

		mm = get_task_mm(t);

		if (mm)
		{
			map = mm->mmap;
	
			if(map)
			{
				down_read(&mm->mmap_sem);
		
				while (map) {
					if (map->vm_file) {
						ino = map->vm_file->f_dentry->d_inode->i_ino;
					} else {
						ino = 0;
					}
		
					trace_statedump_enumerate_vm_maps(t->pid, (void *)map->vm_start, (void *)map->vm_end, map->vm_flags, map->vm_pgoff << PAGE_SHIFT, ino);
					map = map->vm_next;
				}
		
				up_read(&mm->mmap_sem);
			}
	
			mmput(mm);
		}  

 	} while( t != &init_task );

	return 0;
}

#if defined( CONFIG_ARM )
/* defined in arch/arm/kernel/irq.c because of dependency on statically-defined lock & irq_desc */
int ltt_enumerate_interrupts(void);
#else
static inline int ltt_enumerate_interrupts(void)
{
  unsigned int i;
  unsigned long flags = 0;

  /* needs irq_desc */
  
  for(i = 0; i < NR_IRQS; i++)
  {
    struct irqaction * action;

    spin_lock_irqsave(&irq_desc[i].lock, flags);

    
    for (action=irq_desc[i].action; action; action = action->next)
      trace_statedump_enumerate_interrupts(
        irq_desc[i].handler->typename,
        action->name,
        i );

    spin_unlock_irqrestore(&irq_desc[i].lock, flags);
  }
  
  return 0;
}
#endif

static inline int ltt_enumerate_process_states(void)
{
  struct task_struct *          t = &init_task;
  struct task_struct *          p = t;
  enum lttng_process_status     status;
  enum lttng_thread_type        type;
  enum lttng_execution_mode     mode;
  enum lttng_execution_submode  submode;
  
  do {
    mode = LTTNG_MODE_UNKNOWN;
    submode = LTTNG_UNKNOWN;

    read_lock(&tasklist_lock);
    if(t != &init_task) {
      atomic_dec(&t->usage);
      t = next_thread(t);
    }
    if(t == p) {
      t = p = next_task(t);
    }
    atomic_inc(&t->usage);
    read_unlock(&tasklist_lock);
    
    task_lock(t);
    
    if(t->exit_state == EXIT_ZOMBIE)
      status = LTTNG_ZOMBIE;
    else if(t->exit_state == EXIT_DEAD)
      status = LTTNG_DEAD;
    else if(t->state == TASK_RUNNING)
    {
      /* Is this a forked child that has not run yet? */
      if( list_empty(&t->run_list) )
      {
        status = LTTNG_WAIT_FORK;
      }
      else
      {
        /* All tasks are considered as wait_cpu; the viewer will sort out if the task was relly running at this time. */
        status = LTTNG_WAIT_CPU;
      }
    }
    else if(t->state & (TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE))
    {
      /* Task is waiting for something to complete */
      status = LTTNG_WAIT;
    }
    else
      status = LTTNG_UNNAMED;

    submode = LTTNG_NONE;


    /* Verification of t->mm is to filter out kernel threads;
       Viewer will further filter out if a user-space thread was in syscall mode or not */
    if(t->mm)
      type = LTTNG_USER_THREAD;
    else
      type = LTTNG_KERNEL_THREAD;
      
    trace_statedump_enumerate_process_state(t->pid, t->parent->pid, t->comm,
                                            type, mode, submode, status);
    
    task_unlock(t);

  } while( t != &init_task );

  return 0;
}

void ltt_statedump_work_func(void *sem)
{
  /* Our job is just to release the semaphore so
     that we are sure that each CPU has been in syscall
     mode before the end of ltt_statedump_thread */
  up((struct semaphore *)sem);
}

static struct work_struct cpu_work[NR_CPUS];

int ltt_statedump_thread(void *data)
{
  struct semaphore work_sema4;
  int cpu;

  printk("ltt_statedump_thread\n");

 	ltt_enumerate_process_states();
  
  ltt_enumerate_file_descriptors();

	ltt_enumerate_modules();
  
	ltt_enumerate_vm_maps();
  
	ltt_enumerate_interrupts();
	
	ltt_enumerate_network_ip_interface();
  
  /* Fire off a work queue on each CPU. Their sole purpose in life
   * is to guarantee that each CPU has been in a state where is was in syscall
   * mode (i.e. not in a trap, an IRQ or a soft IRQ) */
  sema_init(&work_sema4, 1 - num_online_cpus());
  
  lock_cpu_hotplug();
  for_each_online_cpu(cpu)
  {
    INIT_WORK(&cpu_work[cpu], ltt_statedump_work_func, &work_sema4);

    schedule_delayed_work_on(cpu,&cpu_work[cpu],0);
  }
  unlock_cpu_hotplug();
  
  /* Wait for all work queues to have completed */
  down(&work_sema4);
  
  /* Our work is done */
  printk("trace_statedump_statedump_end\n");
  trace_statedump_statedump_end();
	
	do_exit(0);

	return 0;
}

int ltt_statedump_start(struct ltt_trace_struct *trace)
{
	printk("ltt_statedump_start\n");

	kthread_run(	ltt_statedump_thread,
								NULL,
								"ltt_statedump");
	
	return 0;
}


/* Dynamic facility. */

static int __init statedump_init(void)
{
	int ret;
	printk(KERN_INFO "LTT : ltt-facility-statedump init\n");

	ret = ltt_module_register(LTT_FUNCTION_STATEDUMP,
														ltt_statedump_start,THIS_MODULE);
	
	return ret;
}

static void __exit statedump_exit(void)
{
	ltt_module_unregister(LTT_FUNCTION_STATEDUMP);
}

module_init(statedump_init)
module_exit(statedump_exit)


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jean-Hugues Deschenes");
MODULE_DESCRIPTION("Linux Trace Toolkit Statedump");

