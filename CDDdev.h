#ifndef CDDEV_H
#define CDDEV_H

#include <linux/cdev.h>		// 2.6
#include <linux/types.h>		// dev_t
//#include <asm/semaphore.h>
#include <linux/spinlock.h>
#include <linux/rwsem.h>
#include <linux/ioctl.h>

// multiple minor numbers scheme - start and count
// NOTE: currently allows from 1-5 minor numbers (edit CDDNUMDEVS)
#define CDDMAJOR  	(34)  // this must be specified for ioctl commands
#define CDDMINOR  	(0)
#define CDDNUMDEVS  	(5)
#define CDDLASTMINOR  	( CDDMINOR + CDDNUMDEVS )
#define CDDNAMELEN  (32)

#define CDDCMD_DEVSIZE (0)
#define CDDCMD_DEVUSED (1)
#define CDDCMD_DEVOPENS (2)

// Ioctl commands
// NOTE: Since the user provides the result buffer, we us _IOR
#define CDDIO_DEVSIZE  _IOR(CDDMAJOR, CDDCMD_DEVSIZE, char*)
#define CDDIO_DEVUSED  _IOR(CDDMAJOR, CDDCMD_DEVUSED, char*)
#define CDDIO_DEVOPENS  _IOR(CDDMAJOR, CDDCMD_DEVOPENS, char*)

struct CDDdev_struct {
        dev_t		        devno; // device number (major, minor)
        unsigned int    alloc_len; // total size of buffer
        unsigned int    counter; // current fill level of buffer (==0: empty, ==alloc_len: full)
        char            *CDD_storage; // buffer ptr
        struct cdev     cdev; // kernel device struct
        int							append; // append-in-progress flag

        struct rw_semaphore* CDD_sem; // for RW and RO access to this structure

        unsigned int    active_opens; // open() counter
        spinlock_t      CDDspinlock; // guard for open counter
};

extern struct CDDdev_struct* get_CDDdev(int minor_number);
extern int get_devname_number(int minor_number);
extern const char* get_devname(int minor_number);
extern const char* get_CDD_usage(int type, struct CDDdev_struct *thisCDD);

#endif
 /*
 struct CDDdev_struct{
		atomic_t	counter;
	char 		CDD_name[CDD_PROCLEN + 1];
	char 		CDD_storage[132];

	struct cdev 	cdev;
	dev_t		devno;

	struct semaphore CDDsem;
	spinlock_t      CDDspinlock;
	int	     CDDnumopen;
	atomic_t	CDDcuropen;
	atomic_t	CDDtotopen;
	atomic_t	CDDtotreads;
	atomic_t	CDDtotwrites;
};
*/
