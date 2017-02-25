#ifndef CDDEV_H
#define CDDEV_H

#include <linux/cdev.h>		// 2.6
#include <linux/rwsem.h>

struct CDDdev_struct {
        unsigned int    alloc_len;
        unsigned int    counter;
        char            *CDD_storage;
        struct cdev     cdev;
        int							append;

        struct rw_semaphore* CDD_sem;
};

extern struct CDDdev_struct* get_CDDdev(void);

#endif
