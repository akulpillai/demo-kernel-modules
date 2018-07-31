#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/timer.h>

MODULE_LICENSE("GPL");

static struct timer_list my_timer;

void my_timer_callback(struct timer_list *t)
{	
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
	printk(KERN_INFO "YAY");
}

int init_module(void)
{
	/* setup your timer to call my_timer_callback */
	timer_setup(&my_timer, my_timer_callback, 0);
	/* setup timer interval to 200 msecs */
	mod_timer(&my_timer, jiffies + msecs_to_jiffies(2000));
	return 0;
}

void cleanup_module(void)
{
	/* remove kernel timer when unloading module */
	del_timer(&my_timer);
	return;
}
