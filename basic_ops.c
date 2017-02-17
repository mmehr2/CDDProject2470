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
          thisCDD->append = 0;
    	}

        file->private_data=thisCDD;

	return 0;
}

int CDD_release (struct inode *inode, struct file *file)
{
 	struct CDDdev_struct *thisCDD=file->private_data;

	if( thisCDD->counter <= 0 ) return 0; // -> for compiler warning msg.
	// MOD_DEC_USE_COUNT;
	return 0;
}

ssize_t CDD_read (struct file *file, char *buf,
  size_t count, loff_t *ppos)
{
  int err;
 	struct CDDdev_struct *thisCDD=file->private_data;

    if( *ppos >= thisCDD->counter) return 0;
    else if( *ppos + count >= thisCDD->counter)
      count = thisCDD->counter - *ppos;

    if( count <= 0 ) return 0;
  	printk(KERN_ALERT "CDD_read: count=%d\n", (int)count);

  	// bzero(buf,64);  // a bogus 64byte initialization
  	//memset(buf,0,64);  // a bogus 64byte initialization
  	err = copy_to_user(buf,&(thisCDD->CDD_storage[*ppos]),count);
  	if (err != 0) return -EFAULT;

  	buf[count]=0;
  	*ppos += count;
  	return count;
}

ssize_t CDD_write (struct file *file, const char *buf,
  size_t count, loff_t *ppos)
{
	int err, pos=0;
  struct CDDdev_struct *thisCDD=file->private_data;

  pos=(thisCDD->append)?thisCDD->counter:*ppos;

  // make sure we never overflow the buffer
  if ((pos + count) > thisCDD->alloc_len) return -EFAULT;

	err = copy_from_user(&(thisCDD->CDD_storage[pos]),buf,count);
	if (err != 0) return -EFAULT;

	thisCDD->counter += count;
  *ppos += count;
	return count;
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
