// #set tabstop=2 number nohlsearch
// Example# 2.2 .. Simple "Hello World!" module example

// Change Synopsis 	(top down)
//	macro LINUX_VERSION_CODE
//	macro KERNEL_VERSION(x,y,z)
//	macro	UTS_RELEASE
//
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>

#include <generated/utsrelease.h>


MODULE_AUTHOR("Michael L. Mehr");
MODULE_LICENSE("GPL"); 	/* kernel isn't tainted .. SUSE noop */

// Parameters: how many times we say hello, and some Id to whom.
static char *whom = "World";
// static char *whom;
static int howmany=1;

module_param(whom, charp, 0);
module_param(howmany, int, 0);

static int hello_greeter(int num_times, char *target){
	int i;
	for (i = 0; i < num_times; i++)
		printk(KERN_ALERT "(%d) Hello, %s\n", i, target);
	return 0;
}

static void goodbye_greeter(char *target){
	printk(KERN_ALERT "Goodbye, cruel %s\n", target);
}

static int hello_init(void){
	int ver = LINUX_VERSION_CODE;
	char *ver2 = UTS_RELEASE;

	hello_greeter(howmany, whom);

	printk(KERN_ALERT "   Kernel Version UTS_RELEASE=\"%s\", internally .. LINUX_VERSION_CODE=\"%d\" KERNEL_VERSION=%d\n",ver2, ver,KERNEL_VERSION(2,5,0));

	return 0;
}

static void hello_exit(void){
	goodbye_greeter(whom);
}

module_init(hello_init);
module_exit(hello_exit);
