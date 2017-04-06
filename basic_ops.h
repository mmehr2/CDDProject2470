#ifndef BASIC_OPS_H
#define BASIC_OPS_H

#include <linux/fs.h>

// extern declaration of sub_doprintk() for use in EXPORT_SYMBOL
extern int CDD_open (struct inode *inode, struct file *file);
extern int CDD_release (struct inode *inode, struct file *file);
extern ssize_t CDD_read (struct file *file, char *buf,
  size_t count, loff_t *ppos);
extern ssize_t CDD_write (struct file *file, const char *buf,
  size_t count, loff_t *ppos);
extern loff_t CDD_llseek (struct file *file, loff_t newpos, int whence);
extern long CDD_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

#endif
