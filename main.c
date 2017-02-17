// Example# 3.6b: Simple Char Driver with Static or Dynamic Major#
//		  as requested at install-time.
//                works only in 2.6 (.. a la Example# 3.2)

//      using dev_t, struct cdev (2.6)          (example 3.1b)
//      using myCDD structure                   (example 3.2b)
//      using file->private_data structure      (example 3.3b)
//      using container_of() macro              (example 3.4b)
//      using Static/Dynamic Major#          	(example 3.5b)
//      using Static/Dynamic Major# as a param  (example 3.6b)
//      using O_TRUNC and O_APPEND				(here .. example 3.7b)

// #define	EXPORT_NO_SYMBOLS
#define	EXPORT_SYMBOLS
#define	EXPORT_SYMTAB
#include <linux/version.h>

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/uaccess.h>
#include <linux/vmalloc.h>

#include "basic_ops.h"
#include "CDDdev.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/moduleparam.h>
#endif

MODULE_LICENSE("GPL");   /*  Kernel isn't tainted .. but doesn't
			     it doesn't matter for SUSE anyways :-( */

#define CDD		"CDD2"
#define CDDMAJOR  	35
#define CDDMINOR  	0	// 2.6
#define CDDNUMDEVS  	1	// 2.6

static unsigned int CDDmajor = CDDMAJOR;
static unsigned int CDDparm = CDDMAJOR;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	module_param(CDDparm,int,0);
#else
	MODULE_PARM(CDDparm,"i");
#endif

dev_t   firstdevno;

static struct CDDdev_struct myCDD;

static struct file_operations CDD_fops =
{
	// for LINUX_VERSION_CODE 2.4.0 and later
	owner:	THIS_MODULE, 	// struct module *owner
	open:	CDD_open, 	// open method
	read:   CDD_read,	// read method
	write:  CDD_write, 	// write method
	llseek:  CDD_llseek, // llseek method
	release:  CDD_release 	// release method
};

static int CDD_init(void)
{
	int i;
	struct CDDdev_struct *thisCDD=&myCDD;

	thisCDD->CDD_storage=vmalloc(4096);

	CDDmajor = CDDparm;

	if (CDDmajor) {
	//  Step 1a of 2:  create/populate device numbers
 		firstdevno = MKDEV(CDDmajor, CDDMINOR);

  //  Step 1b of 2:  request/reserve Major Number from Kernel
  	i = register_chrdev_region(firstdevno,1,CDD);
		if (i < 0) { printk(KERN_ALERT "Error (%d) adding CDD", i); return i;}
	}
	else {
	//  Step 1c of 2:  Request a Major Number Dynamically.
 		i = alloc_chrdev_region(&firstdevno, CDDMINOR, CDDNUMDEVS, CDD);
   	if (i < 0) { printk(KERN_ALERT "Error (%d) adding CDD", i); return i;}
		CDDmajor = MAJOR(firstdevno);
		printk(KERN_ALERT "kernel assigned major#: %d to CDD\n", CDDmajor);
	}

	//  Step 2a of 2:  initialize thisCDD->cdev struct
 	cdev_init(&thisCDD->cdev, &CDD_fops);

 	//  Step 2b of 2:  register device with kernel
 	thisCDD->cdev.owner = THIS_MODULE;
	thisCDD->cdev.ops = &CDD_fops;
 	i = cdev_add(&thisCDD->cdev, firstdevno, CDDNUMDEVS);
 	if (i) { printk(KERN_ALERT "Error (%d) adding CDD", i); return i; }

 	return 0;

}

static void CDD_exit(void)
{
 	struct CDDdev_struct *thisCDD=&myCDD;

	vfree(thisCDD->CDD_storage);

 	//  Step 1 of 2:  unregister device with kernel
 	cdev_del(&thisCDD->cdev);

 	//  Step 2b of 2:  Release request/reserve of Major Number from Kernel
 	unregister_chrdev_region(firstdevno, CDDNUMDEVS);

	if (CDDmajor != CDDMAJOR)
		printk(KERN_ALERT "kernel unassigned major#: %d from CDD\n", CDDmajor);
}

module_init(CDD_init);
module_exit(CDD_exit);
