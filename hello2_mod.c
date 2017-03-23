// #set tabstop=2 number nohlsearch
// Example# 2.1 .. Simple "Hello World!" module example
//
// This version creates one of two modules that calls the other module to do the printouts.
// Parameters are stored in this module.

#include <linux/init.h>
#include <linux/module.h>

#include "hello_sub.h"

MODULE_AUTHOR("Michael L. Mehr");
MODULE_LICENSE("GPL"); 	/* kernel isn't tainted .. SUSE noop */

// Parameters: how many times we say hello, and some Id to whom.
static char *whom = "Underground";
// static char *whom;
static int howmany=1;

module_param(whom, charp, 0);
module_param(howmany, int, 0);

static int hmx_hello_greeter(int hm, char *wh) {
	return hello_greeter(hm, wh);
}

EXPORT_SYMBOL(hmx_hello_greeter);

static void hmx_goodbye_greeter(char *wh) {
	goodbye_greeter(wh);
}

EXPORT_SYMBOL(hmx_goodbye_greeter);

static int hello_init(void){

	printk(KERN_ALERT "Loading hello2 greeter module.\n");

	return 0;
}

static void hello_exit(void){

	printk(KERN_ALERT "Unloading hello2 greeter module.\n");

}

module_init(hello_init);
module_exit(hello_exit);
