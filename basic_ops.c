#define	EXPORT_SYMBOLS
#define	EXPORT_SYMTAB

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h> // for copy_*_user
#include <linux/poll.h>                	// for poll() functions and macros
#include <linux/vmalloc.h>

#include "basic_ops.h"
#include "CDDdev.h"

// functions .. exported to module, but not kernel

// test for readiness of reading
//NOTE: caller must hold CDD_sem
int content_is_unavailable(struct CDDdev_struct *thisCDD, struct file *file) {
  int empty_init = (thisCDD->counter == 0)? 1: 0; // E flag
  unsigned int accmode = file->f_flags & O_ACCMODE; // R/W bits
  int open_writeable = (accmode != O_RDONLY); // W flag
  int open_truncreq = (file->f_flags & O_TRUNC); // T flag

  //int would_block = 0; // flag to trigger blocking-open mutex acquire (Block flag)
  int would_sleep =  // flag to trigger sleeping/wait call (wAit flag)
    (!open_writeable && (empty_init || open_truncreq));
  return would_sleep;
}

// unprotected form of above (no requirement to hold the CDD_sem)
// this will acqiore CDD_sem for reading, returning <0 if unable
int content_is_unavailable_unprot(struct CDDdev_struct *thisCDD, struct file *file) {
  int result;
    // acquire read lock (to read the counter)
    // NOTE - opposite convention: 0 means success here
    if (0 != down_read_trylock(thisCDD->CDD_sem))
      return -ERESTARTSYS;
    result = content_is_unavailable(thisCDD, file);
    up_read(thisCDD->CDD_sem); // release read lock
    return result;
}


// // mechanism to restrict usage to one user at a time (for open blocking)
// static int acquire_user(struct CDDdev_struct *thisCDD) {
//   // grab exclusive ownership section
//   spin_lock(&thisCDD->CDD_spinlock);
//   if (thisCDD->CDD_oblk_count && 
//     (!uid_eq(thisCDD->CDD_owner, current_uid())) && // allow known user
//     (!uid_eq(thisCDD->CDD_owner, current_euid())) && // allow who did su
//     !capable(CAP_DAC_OVERRIDE) /// still allow root
//   ) {
//     spin_unlock(&thisCDD->CDD_spinlock);
//     return -EBUSY;
//   }

//   if (thisCDD->CDD_oblk_count == 0)
//     thisCDD->CDD_owner = current_uid();

//   thisCDD->CDD_oblk_count++;
//   spin_unlock(&thisCDD->CDD_spinlock);
//   return 0;
// }

// // release count on current user (called from release method)
// static void release_user(struct CDDdev_struct *thisCDD) {
//   spin_lock(&thisCDD->CDD_spinlock);
//   thisCDD->CDD_oblk_count--;
//   spin_unlock(&thisCDD->CDD_spinlock);
// }

// release the block-on-open-empty mechanism
//NOTE: caller must hold CDD_sem
int release_oblk(struct CDDdev_struct *thisCDD) {
  int blocked = 0;
  // wake up ALL waiting non-exclusive threads (and one exclusive one)
  // for final release, it makes sense to do this before exiting, so don't use exclusive;
  // if we use exclusive waiting, we need a loop to free all of them before exit
  //   and a separate call to free only the one - pass a flag maybe
  wake_up_interruptible(&thisCDD->CDD_inq);
  return blocked;
}

// put caller to sleep (to be called in open method)
//NOTE: caller must hold CDD_sem; will be released on error
static int wait_for_content(struct CDDdev_struct *thisCDD, struct file *file) {
  int ret;
  while (content_is_unavailable(thisCDD, file)) {
    // empty: wait a bit
    DEFINE_WAIT(wait);

    up_write(thisCDD->CDD_sem); // release write lock

    // support non-blocking opens specifically
    if (file->f_flags & O_NONBLOCK) {
      return -EAGAIN;
    }

    printk(KERN_ALERT "%s: open-for-read blocked, ", current->comm);
    prepare_to_wait(&thisCDD->CDD_inq, &wait, TASK_INTERRUPTIBLE);
    ret = content_is_unavailable(thisCDD, file);
    if (ret < 0) {
      printk(KERN_ALERT "returning with error %d\n", ret); //strerror(ret));
      return ret;
    }
    else if (ret > 0) {
      printk(KERN_ALERT "sleeping.\n");
      schedule();
      printk(KERN_ALERT "%s returning from sleep.\n", current->comm);
    }

    finish_wait(&thisCDD->CDD_inq, &wait);
    if (signal_pending(current))
      return -ERESTARTSYS;
    // acquire write lock again
    if (0 == down_write_trylock(thisCDD->CDD_sem))
      return -ERESTARTSYS;
  }
  return 0;
}

// release the mechanism and awaken reader(s)
//NOTE: caller must hold CDD_sem, call in write or exit methods only

int CDD_open (struct inode *inode, struct file *file)
{
        struct CDDdev_struct *thisCDD=
                container_of(inode->i_cdev, struct CDDdev_struct, cdev);

      int result = 0, storage_length, minor;

      // acquire write lock on rest of data structure
      if (0 == down_write_trylock(thisCDD->CDD_sem))
        return -ERESTARTSYS;

      // Ch.8: assure that memory is allocated on first usage
      // BENEFIT: if device never used, memory is never allocated
      if (thisCDD->CDD_storage == NULL) {

        minor =  MINOR(thisCDD->devno);
        storage_length = get_storage_length(minor);
        thisCDD->CDD_storage = vmalloc(storage_length);

        if (thisCDD->CDD_storage == NULL) {
          printk(KERN_ALERT "OPEN(%s): Unable to vmalloc(%d) bytes for device buffer.\n", 
            get_devname(minor), storage_length); 
          result = -ENOMEM; goto Dev_error; 
        } else {
          printk(KERN_ALERT "OPEN(%s): Vmallocated(%d) bytes for device buffer.\n", 
            get_devname(minor), storage_length); 
        }

        thisCDD->alloc_len = storage_length;
      }

      // INSERT SLEEP CODE FROM TEXT
      result = wait_for_content(thisCDD, file);
      if (result) {
        // wait routine releases CDD_sem on error return
        return result;
      }

      // insert exclusive-caller code from text (other users get EBUSY)
      // result = acquire_user(thisCDD);
      // if (result) {
      //   return result;
      // }

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

      printk(KERN_ALERT "file '%s' opened with devno=%d by %s(%d)\n",
          file->f_path.dentry->d_name.name, thisCDD->devno, current->comm, (int)current->pid);

Dev_error:
  up_write(thisCDD->CDD_sem); // release write lock

	return result;
}

int CDD_release (struct inode *inode, struct file *file)
{
 	//struct CDDdev_struct *thisCDD=file->private_data;

  // release count of active user
  //release_user(thisCDD);
	return 0;
}

ssize_t CDD_read (struct file *file, char *buf,
  size_t count, loff_t *ppos)
{
  int retval=0;
 	struct CDDdev_struct *thisCDD=file->private_data;

  // acquire read lock
    down_read(thisCDD->CDD_sem);

    //printk(KERN_DEBUG "CDD_read try: count=%db @ ofs=%d fpos=%d\n", (int)count, (int)*ppos, (int)thisCDD->counter);

    if( *ppos >= thisCDD->counter) {retval= 0; goto Done;}
    else if( *ppos + count >= thisCDD->counter)
      count = thisCDD->counter - *ppos;

    if( count <= 0 ) {retval= 0; goto Done;}
  	printk(KERN_ALERT "CDD_read: count=%d by %s(%d)\n", (int)count, current->comm, (int)current->pid);

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

  //printk(KERN_DEBUG "CDD_write(App=%d) try: count=%db @ ofs=%d bufpos=%d, tot=%d: [start=%d, end=%d]\n"
  //  , thisCDD->append, (int)count, (int)*ppos, (int)thisCDD->counter, thisCDD->alloc_len, pos, end);

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
 	printk(KERN_ALERT "CDD_write: count=%d by %s(%d)\n", (int)count, current->comm, (int)current->pid);
  // end critical section
Done:
  up_write(thisCDD->CDD_sem);
  if (retval >= 0) {
    // something was written, release anyone blocked on empty open
    release_oblk(thisCDD);
  }
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
