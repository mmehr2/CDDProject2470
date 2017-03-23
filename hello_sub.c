// #set tabstop=2 number nohlsearch
// Example# 2.2 .. Simple "Hello World!" module example

// Change Synopsis 	(top down)
//	macro LINUX_VERSION_CODE
//	macro KERNEL_VERSION(x,y,z)
//	macro	UTS_RELEASE
//
//#include <linux/init.h>
//#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <generated/utsrelease.h>

#include "hello_sub.h"
//MODULE_AUTHOR("Michael L. Mehr");
//MODULE_LICENSE("GPL"); 	/* kernel isn't tainted .. SUSE noop */

int hello_greeter(int num_times, char *target){
	int ver = LINUX_VERSION_CODE;
	char *ver2 = UTS_RELEASE;

	int i;
	for (i = 0; i < num_times; i++)
		printk(KERN_ALERT "(%d) Hello, %s\n", i, target);

	printk(KERN_ALERT "   Kernel Version UTS_RELEASE=\"%s\", internally .. LINUX_VERSION_CODE=\"%d\" KERNEL_VERSION=%d\n",ver2, ver,KERNEL_VERSION(2,5,0));
	return 0;
}

void goodbye_greeter(char *target){
	printk(KERN_ALERT "Goodbye, cruel %s\n", target);
}
