#include <linux/kernel.h>
#include <linux/module.h>

int init_module()
{
  unsigned tmp;
    
  /* Disable cache */
    
  asm volatile ("movl  %%cr0, %0\n\t"
                "orl   $0x40000000, %0\n\t"
                "wbinvd\n\t"
                "movl  %0, %%cr0\n\t"
                "wbinvd\n\t"
                : "=r" (tmp) : : "memory");

  return 0;
}

void cleanup_module()
{
  unsigned tmp;

  asm volatile ("movl  %%cr0, %0\n\t"
                "andl   $0xbfffffff, %0\n\t"
                "wbinvd\n\t"
                "movl  %0, %%cr0\n\t"
                "wbinvd\n\t"
                : "=r" (tmp) : : "memory");
}
