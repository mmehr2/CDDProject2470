#define	EXPORT_SYMBOLS
#define	EXPORT_SYMTAB

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h> // for copy_*_user
#include <linux/poll.h>                	// for poll() functions and macros

#include "basic_ops.h"
#include "CDDdev.h"

// functions .. exported to kernel

int CDD_open (struct inode *inode, struct file *file)
{
        struct CDDdev_struct *thisCDD=
                container_of(inode->i_cdev, struct CDDdev_struct, cdev);

      // acquire write lock on data structure
      if (0 == down_write_trylock(thisCDD->CDD_sem))
        return -ERESTARTSYS;

      // count the open called
      ++thisCDD->active_opens;

      thisCDD->append = 0;

	    if ( file->f_flags & O_TRUNC )  {

        	printk(KERN_ALERT "file '%s' opened O_TRUNC\n",
            	file->f_path.dentry->d_name.name);

        	thisCDD->CDD_storage[0]=0;
        	thisCDD->counter=0;
    	}

    	if ( file->f_flags & O_APPEND )  {
        	printk(KERN_ALERT "file '%s' opened O_APPEND\n",
	            file->f_path.dentry->d_name.name);
          thisCDD->append = 1;
    	}

        file->private_data=thisCDD;

        printk(KERN_ALERT "file '%s' opened with devno=%d\n",
            file->f_path.dentry->d_name.name, thisCDD->devno);

        up_write(thisCDD->CDD_sem); // release write lock

	return 0;
}

int CDD_release (struct inode *inode, struct file *file)
{
// 	struct CDDdev_struct *thisCDD=file->private_data;

	//if( thisCDD->counter <= 0 ) return 0; // -> for compiler warning msg.
	// MOD_DEC_USE_COUNT;
	return 0;
}

ssize_t CDD_read (struct file *file, char *buf,
  size_t count, loff_t *ppos)
{
  int retval=0;
 	struct CDDdev_struct *thisCDD=file->private_data;

  // acquire read lock
    down_read(thisCDD->CDD_sem);

    printk(KERN_DEBUG "CDD_read try: count=%db @ ofs=%d fpos=%d\n", (int)count, (int)*ppos, (int)thisCDD->counter);

    if( *ppos >= thisCDD->counter) {retval= 0; goto Done;}
    else if( *ppos + count >= thisCDD->counter)
      count = thisCDD->counter - *ppos;

    if( count <= 0 ) {retval= 0; goto Done;}
  	printk(KERN_ALERT "CDD_read: count=%d\n", (int)count);

  	// bzero(buf,64);  // a bogus 64byte initialization
  	//memset(buf,0,64);  // a bogus 64byte initialization
  	retval = copy_to_user(buf,&(thisCDD->CDD_storage[*ppos]),count);
  	if (retval != 0) {retval= -EFAULT; goto Done;}

  	buf[count]=0;
  	*ppos += count;
  	retval = count;
Done:
    up_read(thisCDD->CDD_sem);
    return retval;
}

ssize_t CDD_write (struct file *file, const char *buf,
  size_t count, loff_t *ppos)
{
	int retval=0, pos=0, end=0;
  struct CDDdev_struct *thisCDD=file->private_data;

// critical section for write
  if (0 == down_write_trylock(thisCDD->CDD_sem)) {
    return -ERESTARTSYS; // no semaphore to release
  }

  pos=(thisCDD->append)?thisCDD->counter:*ppos;
  end = pos + count;

  printk(KERN_DEBUG "CDD_write(App=%d) try: count=%db @ ofs=%d bufpos=%d, tot=%d: [start=%d, end=%d]\n"
    , thisCDD->append, (int)count, (int)*ppos, (int)thisCDD->counter, thisCDD->alloc_len, pos, end);

  // with seeking, *ppos can be negative or past EOF or buffer size
  // make sure we never overflow the buffer
  if (end > thisCDD->alloc_len) {retval= -EFAULT; goto Done;}
  // make sure we never underflow the buffer
  if (pos < 0) {retval= -EFAULT; goto Done;}

	retval = copy_from_user(&(thisCDD->CDD_storage[pos]),buf,count);
	if (retval != 0) {retval= -EFAULT; goto Done;}

  // only update end of buffer if we have written past previous one
  if (end > thisCDD->counter)
	   thisCDD->counter = end;
  *ppos += count;
	retval = count;
  // end critical section
Done:
  up_write(thisCDD->CDD_sem);
  return retval;
}

loff_t CDD_llseek (struct file *file, loff_t newpos, int whence)
{
  int pos, from = 0;
	struct CDDdev_struct *thisCDD=file->private_data;

	switch(whence) {
		case SEEK_SET:        // CDDoffset can be 0 or +ve
			from = 0;
			break;
		case SEEK_CUR:        // CDDoffset can be 0 or +ve
      from = file->f_pos;
			break;
		case SEEK_END:        // CDDoffset can be 0 or +ve
      from = thisCDD->counter;
			break;
		default:
			return -EINVAL;
	}

  pos = from + newpos;
  printk(KERN_DEBUG "CDD_seek(%d) try: set to (%d)+(%d)=(%d) from buffer:%d, file:%d\n"
    , whence, (int)newpos, from, pos, (int)thisCDD->counter, (int)file->f_pos);

	if ((pos < 0)||(pos>thisCDD->counter))
		return -EINVAL;

	file->f_pos = pos;
	return pos;
}

const char* get_CDD_usage(int type, struct CDDdev_struct *thisCDD) {
  static char retbuf[128];
  const char* fmt = "";
  int value = 0;
  switch(type) {
    case CDDCMD_DEVSIZE:
      fmt = "Buffer Length - Allocated: %d";
      value = thisCDD->alloc_len;
      break;
    case CDDCMD_DEVUSED:
      fmt = "Buffer Length - Used: %d";
      value = thisCDD->counter;
      break;
    case CDDCMD_DEVOPENS:
      fmt = "# Opens: %d";
      value = thisCDD->active_opens;
      break;
  }
  sprintf(retbuf, fmt, value);
  return retbuf;
}

long CDD_ioctl(	/* see include/linux/fs.h */
		 struct file *file,	/* ditto */
		 unsigned int ioctl_num,	/* number and param for ioctl */
		 unsigned long ioctl_param)
{
	int i, retval;
	char *temp = (char *)ioctl_param;
  const char *str;
	struct CDDdev_struct *thisCDD=file->private_data;

/* 
 * Switch according to the ioctl called 
 */
	switch (ioctl_num) {
    case CDDIO_DEVSIZE:
      i = CDDCMD_DEVSIZE;
  		break;
    case CDDIO_DEVUSED:
      i = CDDCMD_DEVUSED;
  		break;
    case CDDIO_DEVOPENS:
      i = CDDCMD_DEVOPENS;
  		break;
    default:
      return -ENOTTY; // invalid ioctl
	}

  // acquire read lock
  down_read(thisCDD->CDD_sem);
  // get the requested usage string
  str = get_CDD_usage(i, thisCDD);
  // return to user
  retval = copy_to_user(temp,str,strlen(str) + 1); // include the final null char
  if (retval != 0) {retval= -EFAULT; goto Done;}

	//retval = 0;
  
Done:
// free the lock when done
  up_read(thisCDD->CDD_sem);
  return retval;
}


unsigned int CDD_poll (struct file *file, struct poll_table_struct *polltbl) 
{
	unsigned int mask=0;

	mask |= POLLIN | POLLRDNORM;
	mask |= POLLOUT | POLLWRNORM;

	return mask;
}
