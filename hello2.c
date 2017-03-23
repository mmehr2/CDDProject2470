// #set tabstop=2 number nohlsearch
// Example# 2.1 .. Simple "Hello World!" module example
//
// This version creates one of two modules that calls the other module to do the printouts.
// Parameters are stored in this module.

#include <linux/init.h>
#include <linux/module.h>

#define HELLO_DOUBLE_MODULE
#include "hello_sub.h"

MODULE_AUTHOR("Michael L. Mehr");
MODULE_LICENSE("GPL"); 	/* kernel isn't tainted .. SUSE noop */

// Parameters: how many times we say hello, and some Id to whom.
static char *whom = "World";
// static char *whom;
static int howmany=1;

module_param(whom, charp, 0);
module_param(howmany, int, 0);

static int hello_init(void){

	hmx_hello_greeter(howmany, whom);

	return 0;
}

static void hello_exit(void){

	hmx_goodbye_greeter(whom);

}

module_init(hello_init);
module_exit(hello_exit);
