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
#include <linux/slab.h> // for kmalloc/kfree

#include "basic_ops.h"
#include "CDDdev.h"
#include "proc_ops.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/moduleparam.h>
#endif

MODULE_AUTHOR("Michael Mehr");
MODULE_LICENSE("GPL");   /*  Kernel isn't tainted .. but doesn't
			     it doesn't matter for SUSE anyways :-( */

#define CDD		"CDD2"
#define CDDMAJOR  	35
#define CDDMINOR  	0	// 2.6
#define CDDNUMDEVS  	1	// 2.6
#define STORAGE_LEN 4096

static unsigned int CDDmajor = CDDMAJOR;
static unsigned int CDDparm = CDDMAJOR;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	module_param(CDDparm,int,0);
#else
	MODULE_PARM(CDDparm,"i");
#endif

dev_t   firstdevno;

static struct CDDdev_struct myCDD;
// access the private CDD block (for /proc debugging)
struct CDDdev_struct* get_CDDdev(void) { return &myCDD; }

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

	// use read/write semaphore to separate read and write access to this
	thisCDD->CDD_sem=(struct rw_semaphore *)
     kmalloc(sizeof(struct rw_semaphore),GFP_KERNEL);
	init_rwsem(thisCDD->CDD_sem);
	thisCDD->active_opens = 0; // init open counter

	// acquire write access for allocation and cdev setup

	if (thisCDD->CDD_storage == NULL) {
		thisCDD->CDD_storage=vmalloc(STORAGE_LEN);
		thisCDD->alloc_len=STORAGE_LEN;
	}

	CDDmajor = CDDparm;

	if (CDDmajor) {
	//  Step 1a of 2:  create/populate device numbers
 		firstdevno = MKDEV(CDDmajor, CDDMINOR);

  //  Step 1b of 2:  request/reserve Major Number from Kernel
  	i = register_chrdev_region(firstdevno,1,CDD);
		if (i < 0) { printk(KERN_ALERT "Error (%d) adding CDD\n", i); goto Error;}
	}
	else {
	//  Step 1c of 2:  Request a Major Number Dynamically.
 		i = alloc_chrdev_region(&firstdevno, CDDMINOR, CDDNUMDEVS, CDD);
   	if (i < 0) { printk(KERN_ALERT "Error (%d) adding CDD\n", i); goto Error;}
		CDDmajor = MAJOR(firstdevno);
		printk(KERN_ALERT "kernel assigned major#: %d to CDD\n", CDDmajor);
	}

	//  Step 2a of 2:  initialize thisCDD->cdev struct
 	cdev_init(&thisCDD->cdev, &CDD_fops);
	CDDproc_init(); // must be done before the device goes active w the kernel

 	//  Step 2b of 2:  register device with kernel
 	thisCDD->cdev.owner = THIS_MODULE;
	thisCDD->cdev.ops = &CDD_fops;
 	i = cdev_add(&thisCDD->cdev, firstdevno, CDDNUMDEVS);
 	if (i) { printk(KERN_ALERT "Error (%d) adding CDD\n", i); goto Error; }

 	return 0;
Error:
	// all errors will come here to release common resources
	// free any allocated memory
	if (thisCDD->CDD_storage) {
		vfree(thisCDD->CDD_storage);
		thisCDD->CDD_storage = NULL;
	}

	// free the lock last
	if (thisCDD->CDD_sem) {
		kfree(thisCDD->CDD_sem);
		thisCDD->CDD_sem = NULL;
	}

	// i will be set with any error code
	return i;
}

static void CDD_exit(void)
{
 	struct CDDdev_struct *thisCDD=&myCDD;
 	//  Step 1 of 2:  unregister device with kernel
 	cdev_del(&thisCDD->cdev);

 	//  Step 2b of 2:  Release request/reserve of Major Number from Kernel
 	unregister_chrdev_region(firstdevno, CDDNUMDEVS);

		// ACQUIRE THE WRITE LOCK? OR JUST KILL IT ALL?

		vfree(thisCDD->CDD_storage);

		CDDproc_exit();


		// free the lock last
		if (thisCDD->CDD_sem) {
			kfree(thisCDD->CDD_sem);
			thisCDD->CDD_sem = NULL;
		}

	if (CDDmajor != CDDMAJOR)
		printk(KERN_ALERT "kernel unassigned major#: %d from CDD\n", CDDmajor);
}

module_init(CDD_init);
module_exit(CDD_exit);
