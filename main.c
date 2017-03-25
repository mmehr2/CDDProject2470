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

#include "proc_ops.h" // for /proc/CDD reporting entries

#include "proc_seq_ops.h" // for the /proc/myps feature

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
#include <linux/moduleparam.h>
#endif

MODULE_AUTHOR("Michael Mehr");
MODULE_LICENSE("GPL");   /*  Kernel isn't tainted .. but doesn't
			     it doesn't matter for SUSE anyways :-( */

#define CDDMAJOR  	35
// #define CDDMINOR  	0	// moved to CDDev.h
// #define CDDNUMDEVS  	5
//#define STORAGE_LEN 4096
static const size_t storage_lengths[] = { 4096, 16, 64, 128, 256 };
int get_storage_length(int minor) { minor -= CDDMINOR; return storage_lengths[minor]; }

// return the numeric portion of the device name (CDD2, CDD16, CDD64, etc)
int get_devname_number(int minor) { minor -= CDDMINOR; return ((minor==0) ? 2 : storage_lengths[minor]); }
const char* get_devname(int minor) {
	static char buffer[CDDNAMELEN];
	sprintf(buffer,"CDD%d", get_devname_number(minor));
	return buffer;
}

//#define CDD		"CDD2"
#define CDD		(get_devname(0))

static unsigned int CDDmajor = CDDMAJOR;
static unsigned int CDDparm = CDDMAJOR;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
	module_param(CDDparm,int,0);
#else
	MODULE_PARM(CDDparm,"i");
#endif

dev_t   firstdevno;

static struct CDDdev_struct CDD_dev[CDDNUMDEVS];
// access the private CDD block (for /proc debugging)
struct CDDdev_struct* get_CDDdev(int minor) { minor -= CDDMINOR; return &CDD_dev[minor]; }

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
	int i, err;
	int some_devices_ok = 0; // for adding the /proc facility
	struct CDDdev_struct *thisCDD;

	CDDproc_seq_init(); // unrelated /proc/myps feature

	CDDmajor = CDDparm;

	if (CDDmajor) {
	//  Step 1a of 2:  create/populate device numbers
 		firstdevno = MKDEV(CDDmajor, CDDMINOR);

  //  Step 1b of 2:  request/reserve Major Number from Kernel
  	err = register_chrdev_region(firstdevno,1,CDD);
		if (err < 0) { printk(KERN_ALERT "Error (%d) registerng CDD major #%d\n", err, CDDmajor); goto Init_error_exit;}
	}
	else {
	//  Step 1c of 2:  Request a Major Number Dynamically.
 		err = alloc_chrdev_region(&firstdevno, CDDMINOR, CDDNUMDEVS, CDD);
   	if (err < 0) { printk(KERN_ALERT "Error (%d) allocating CDD major #%d\n", err, CDDmajor); goto Init_error_exit;}
		CDDmajor = MAJOR(firstdevno);
		printk(KERN_ALERT "kernel assigned major#: %d to CDD\n", CDDmajor);
	}

	// NOTE: it is required that all the CDDdev_struct memory is zeroed out at this point, esp.the devno field
	for (i=CDDMINOR; i<CDDLASTMINOR; ++i) {
		thisCDD = get_CDDdev(i);
		thisCDD->devno = 0; // in case of failure of one of the set of minor drivers
	}

	for (i=CDDMINOR; i<CDDLASTMINOR; ++i) {
		int storage_length;
		thisCDD = get_CDDdev(i);

		//thisCDD->devno = 0; // in case of failure...
		thisCDD->counter = 0;
		thisCDD->append = 0;
		thisCDD->active_opens = 0;

		// use read/write semaphore to separate read and write access to this
		thisCDD->CDD_sem=(struct rw_semaphore *)
	     kmalloc(sizeof(struct rw_semaphore),GFP_KERNEL);
	 	if (thisCDD->CDD_sem == NULL) { err = -ENOMEM; goto Minor_dev_error; }
		init_rwsem(thisCDD->CDD_sem);

		// acquire write access for allocation and cdev setup
		storage_length = get_storage_length(i);
		thisCDD->CDD_storage=vmalloc(storage_length);
		if (thisCDD->CDD_storage == NULL) { err = -ENOMEM; goto Minor_dev_error; }
		thisCDD->alloc_len=storage_length;

		// initialize CDD_dev spinlock
		spin_lock_init(&(thisCDD->CDDspinlock));

		//  Step 2a of 2:  initialize thisCDD->cdev struct
	 	cdev_init(&thisCDD->cdev, &CDD_fops);

	 	//  Step 2b of 2:  register device with kernel
	 	thisCDD->cdev.owner = THIS_MODULE;
		thisCDD->cdev.ops = &CDD_fops;
		thisCDD->devno = MKDEV(CDDmajor,i);
	 	err = cdev_add(&thisCDD->cdev, thisCDD->devno, 1);
		if (!err) {
			printk(KERN_ALERT "kernel Successfully added CDD%d(#%d)\n", get_devname_number(i), i);
			some_devices_ok = 1;
			CDDproc_init(i); // OK to do this after the device goes active w the kernel
		}
Minor_dev_error:
	 	if (err) {
			thisCDD->devno = 0; // mark the failure..
			printk(KERN_ALERT "Error (%d) adding CDD%d(#%d)\n", err, get_devname_number(i), i); goto Partial_decommission_exit;
		}
	}
 	return 0; // err must be 0 here
Partial_decommission_exit:
	// all errors will come here to release common resources
	// go through entire list of possible allocated devices, free memory for each
	for (i=CDDMINOR; i<CDDLASTMINOR; ++i) {
		thisCDD = get_CDDdev(i);
		// if device was added to kernel, let it keep running
		// we detect this with zero devno contents
		if (thisCDD->devno != 0) {
			continue;
		}

		// otherwise, we had a failed call to add the driver
		printk(KERN_ALERT "Decommissioned CDD%d(#%d)\n", get_devname_number(i), i);
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
	}
Init_error_exit:
	// err will be set with any error code
	return err;
}

static void CDD_exit(void)
{
	int i;
 	struct CDDdev_struct *thisCDD;

	CDDproc_seq_exit(); // remove unrelated /proc/myps feature (???)

	for (i=CDDMINOR; i<CDDLASTMINOR; ++i) {
		thisCDD = get_CDDdev(i);
		// if device was not added to kernel, don't try to exit it
		// we detect this with zero devno contents
		if (thisCDD->devno == 0)
			continue;

		// ACQUIRE THE WRITE LOCK? OR JUST KILL IT?
		// NOTE: Need to wait until pending uses of the device are done
		// Does the wait happen here? (or see below)

		CDDproc_exit(i); // remove /proc entry that could query thisCDD too
		// NOTE: It probably also should wait on pending queries

	 	//  Step 1 of 2:  unregister device with kernel
	 	cdev_del(&thisCDD->cdev);

		// NOTE: maybe the wait happens here, if the kernel is oK with it
		// this way, you can't even create new ones while you're waiting
		// BUT if something is in progress, does the cdev_del() cause a panic?

		// free allocated memory
		vfree(thisCDD->CDD_storage);
		thisCDD->CDD_storage = NULL;

		// free the lock last
		kfree(thisCDD->CDD_sem);
		thisCDD->CDD_sem = NULL;
	}

 	//  Step 2b of 2:  Release request/reserve of Major Number from Kernel
 	unregister_chrdev_region(firstdevno, CDDNUMDEVS);

	if (CDDmajor != CDDMAJOR)
		printk(KERN_ALERT "kernel unassigned major#: %d from CDD\n", CDDmajor);
}

module_init(CDD_init);
module_exit(CDD_exit);
