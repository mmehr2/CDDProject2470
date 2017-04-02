#define	EXPORT_SYMBOLS
#define	EXPORT_SYMTAB

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h> // for copy_*_user

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
	int retval=0, pos=0;
  struct CDDdev_struct *thisCDD=file->private_data;

// critical section for write
  if (down_write_trylock(thisCDD->CDD_sem)) {

    pos=(thisCDD->append)?thisCDD->counter:*ppos;

    // make sure we never overflow the buffer
    if ((pos + count) > thisCDD->alloc_len) {retval= -EFAULT; goto Done;}

  	retval = copy_from_user(&(thisCDD->CDD_storage[pos]),buf,count);
  	if (retval != 0) {retval= -EFAULT; goto Done;}

  	thisCDD->counter += count;
    *ppos += count;
  	retval = count;
    // end critical section
  } else {
    return -ERESTARTSYS; // no semaphore to release
  }
  Done:
  up_write(thisCDD->CDD_sem);
  return retval;
}

loff_t CDD_llseek (struct file *file, loff_t newpos, int whence)
{
  int pos;
	struct CDDdev_struct *thisCDD=file->private_data;

	switch(whence) {
		case SEEK_SET:        // CDDoffset can be 0 or +ve
			pos=newpos;
			break;
		case SEEK_CUR:        // CDDoffset can be 0 or +ve
			pos=(file->f_pos + newpos);
			break;
		case SEEK_END:        // CDDoffset can be 0 or +ve
			pos=(thisCDD->counter + newpos);
			break;
		default:
			return -EINVAL;
	}
	if ((pos < 0)||(pos>thisCDD->counter))
		return -EINVAL;

	file->f_pos = pos;
	return pos;
}
