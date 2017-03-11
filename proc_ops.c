#define	EXPORT_SYMBOLS
#define	EXPORT_SYMTAB
#include <linux/version.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h> // for copy_*_user
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>

#include <linux/sched.h>                // for current->pid

#include "proc_ops.h"
#include "CDDdev.h"

#include "proc_seq_ops.h" // for the /proc/myps sequencer
#include "proc_log_marker.h" // for the /proc/CDD/marker utility

#define CDD		"myCDD2" // proc entry
#define myCDD  		"CDD" // proc outer directory

#define CDD_PROCLEN     32


static struct proc_dir_entry *proc_topdir;
static struct proc_dir_entry *proc_entry;

/*
 * PROC CDD FILE SERVICES
 *
 * These functions allow other parts of the driver to allocate entries in the /proc/CDD subtree
*/

// create_CDD_procdir_entry() - to allow others to attach to the /proc/CDD subtree
// will return NULL and do nothing if proc_topdir has not been initialized yet
struct proc_dir_entry *
create_CDD_procdir_entry(const char* name, int permissions, const struct file_operations* fops)
{
  struct proc_dir_entry* proc_entry = NULL;
  if (proc_topdir == NULL)
    return NULL;

  #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
    proc_entry = proc_create(name, permissions, proc_topdir, fops);
  #else
    proc_entry = create_proc_entry(name,0,proc_topdir);
    proc_entry->read_proc = fops->read_proc;
    proc_entry->write_proc = fops->writeproc;
  #endif

  #if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,29)
    proc_entry->owner = THIS_MODULE;
  #endif
    return proc_entry;
}

// This will allow removal of the immediate subdirectory by name
void remove_CDD_procdir_entry(const char* name)
{
  return remove_proc_entry (name, proc_topdir);
}

/*
 * BASIC PROC/CDD DATA
*/
struct CDDproc_struct {
        char CDD_procname[CDD_PROCLEN + 1];
        char *CDD_procvalue;
        char CDD_procflag;
};

static struct CDDproc_struct CDDproc;

/*
 * BASIC PROC/CDD OPERATIONS
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int eof[1];

static ssize_t readproc_CDD2(struct file *file, char *buf,
	size_t len, loff_t *ppos)
  {
    ssize_t retval = 0;
#else
static int readproc_CDD2(char *buf, char **start,
  off_t offset, int len, int *eof, void *unused)
  {
    int retval = 0;
#endif

  struct CDDproc_struct *usrsp=&CDDproc;
  struct CDDdev_struct *thisCDD=get_CDDdev();

  // lock the CDD info for reading
  if (0 == down_read_trylock(thisCDD->CDD_sem))
    return -ERESTARTSYS;

  if (*eof!=0) { *eof=0; retval= 0; goto Done; }
  else {

    snprintf(buf,len,
       "Mode: %s\n"
       "Group Number: %d\n"
       "Team Members: %s, %s\n"
       "Buffer Length - Allocated: %u\n"
       "Buffer Length - Used: %u\n"
       "# Opens: %d\n"
      , ((usrsp->CDD_procflag!=0)? "Written": "Readable")
      , 42
      , "Mike Mehr", usrsp->CDD_procvalue
      , thisCDD->alloc_len
      , thisCDD->counter
      , thisCDD->active_opens
    );
    usrsp->CDD_procflag=0;
	}

  *eof = 1;
	retval= (strlen(buf));

Done:
  up_read(thisCDD->CDD_sem);
  return retval;
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static ssize_t writeproc_CDD2 (struct file *file,const char * buf,
    size_t count, loff_t *ppos)
#else
static int writeproc_CDD2(struct file *file,const char *buf,
                unsigned long count, void *data)
#endif
{
	int length=count;
	struct CDDproc_struct *usrsp=&CDDproc;

	length = (length<CDD_PROCLEN)? length:CDD_PROCLEN;

	if (copy_from_user(usrsp->CDD_procvalue, buf, length))
		return -EFAULT;

	usrsp->CDD_procvalue[length-1]=0;
	usrsp->CDD_procflag=1;

	return(length);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static const struct file_operations proc_fops = {
 // .owner = THIS_MODULE,
 .read  = readproc_CDD2,
 .write = writeproc_CDD2,
};
#endif

int CDDproc_init(void)
{
	CDDproc.CDD_procvalue=vmalloc(4096);
  strcpy(CDDproc.CDD_procvalue, "Team Member #2");

  // Create the necessary proc entries
  proc_topdir = proc_mkdir(myCDD,0);
  proc_entry = create_CDD_procdir_entry(CDD, 0777, &proc_fops);

  // add the /proc/myps sequencer
  CDDproc_seq_init();
  // add the logfile marker utility
  CDDproc_log_marker_init();

	return 0;
}

void CDDproc_exit(void)
{
  // add the logfile marker utility
  CDDproc_log_marker_exit();
  // remove the /proc/myps sequencer
  CDDproc_seq_exit();

	vfree(CDDproc.CDD_procvalue);

  // remove subdirectories and subtrees
	if (proc_entry) remove_CDD_procdir_entry(CDD);
  // remove master directory entry
 	if (proc_topdir) remove_proc_entry (myCDD, 0);

}
