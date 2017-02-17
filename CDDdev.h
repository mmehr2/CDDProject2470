#ifndef CDDEV_H
#define CDDEV_H

#include <linux/cdev.h>		// 2.6

struct CDDdev_struct {
        unsigned int    counter;
        char            *CDD_storage;
        struct cdev     cdev;
        int							append;
};

#endif
