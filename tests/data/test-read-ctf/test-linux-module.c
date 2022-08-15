#include <linux/kernel.h>
#include <linux/module.h>

int global_sym = 0;
EXPORT_SYMBOL(global_sym);
static spinlock_t my_lock;

int testexport(void)
{
  printk("in testexport\n");
  return 0;
}

EXPORT_SYMBOL(testexport);

int testexport2(spinlock_t *t)
{
  printk("in testexport\n");
  return 0;
}
EXPORT_SYMBOL(testexport2);

int hello_init(void)
{
  printk(KERN_INFO "Hello World!\n");
  return 0;
}

void hello_exit(void)
{
  printk(KERN_INFO "Bye World!\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
