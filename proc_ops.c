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

#include "proc_log_marker.h" // for the /proc/CDD/marker utility

#define CDD		"myCDD2" // proc entry (for pre hwk.6 single minor larger-buffer device)
#define myCDD  		"CDD" // proc outer directory

#define CDD_PROCLEN     32


static int topdir_refcount = 0;
static struct proc_dir_entry *proc_topdir;
//static struct proc_dir_entry *proc_entry;

/*
 * PROC CDD FILE SERVICES
 *
 * These functions allow other parts of the driver to allocate entries in the /proc/CDD subtree
*/

// create_CDD_procdir_entry() - to allow others to attach to the /proc/CDD subtree
// will return NULL and do nothing if proc_topdir has not been initialized yet
// NOTE: TBD - determine if we need to lock the recount or use atomic_t or both
struct proc_dir_entry *
create_CDD_procdir_entry(const char* name, int permissions, const struct file_operations* fops)
{
  struct proc_dir_entry* proc_entry;
  int prevct=topdir_refcount;

  if (topdir_refcount++ == 0) {
    proc_topdir = proc_mkdir(myCDD,0);
    printk(KERN_ALERT "Created procfs entry /proc/%s (refct=%d->%d)\n", myCDD, prevct++, topdir_refcount);
  }

  proc_entry = proc_create(name, permissions, proc_topdir, fops);
  printk(KERN_ALERT "Created procfs entry /proc/%s/%s (refct=%d->%d)\n", myCDD, name, prevct, topdir_refcount);

  return proc_entry;
}

// This will allow removal of the immediate subdirectory by name
void remove_CDD_procdir_entry(const char* name)
{
  int prevct=topdir_refcount;
  --topdir_refcount;

  remove_proc_entry (name, proc_topdir);
  printk(KERN_ALERT "Removed procfs entry /proc/%s/%s (refct=%d->%d)\n", myCDD, name, prevct--, topdir_refcount);

  if (!topdir_refcount) {
    remove_proc_entry(myCDD,0);
    printk(KERN_ALERT "Removed procfs entry /proc/%s (refct=%d->%d)\n", myCDD, prevct, topdir_refcount);
  }
}

// because of autocreate of CDD/marker, this isn't obviously when refcount==1
int topdir_entry_count(void) { return !topdir_refcount ? 0 : topdir_refcount - 1; }

/*
 * BASIC PROC/CDD DATA
*/
struct CDDproc_struct {
    char CDD_procname[CDD_PROCLEN + 1];
    char *CDD_procvalue;
    char CDD_procflag;
    int minor;
    struct proc_dir_entry *proc_entry;
};

static struct CDDproc_struct CDDprocs[CDDNUMDEVS];

struct CDDproc_struct* get_CDDdproc(int minor) { minor -= CDDMINOR; return &CDDprocs[minor]; }

struct CDDproc_struct* lookup_CDDdproc(const char* devname) {
  int minor;
  for(minor = CDDMINOR; minor<CDDLASTMINOR; ++minor) {
    struct CDDproc_struct* ptr = get_CDDdproc(minor);
    if (strcmp(devname, ptr->CDD_procname) == 0) {
      return ptr;
    }
  }
  return NULL; // not found
}

/*
 * BASIC PROC/CDD OPERATIONS
*/
static int eof[1];

static ssize_t readproc_CDD2(struct file *file, char *buf,
	size_t len, loff_t *ppos)
  {
    ssize_t retval = 0;

  // 1: get the filename of the proc entry, such as 'CDDx' for /proc/CDD/CDDx
  // 2: convert it to a struct CDDproc_struct
  // 3: get the minor number
  // 4: get its CDDdev_struct
  const char* devfname = file->f_path.dentry->d_name.name;
  struct CDDproc_struct *usrsp = lookup_CDDdproc(devfname);
  int minor = usrsp? usrsp->minor: 0;
  struct CDDdev_struct *thisCDD=get_CDDdev(minor);

  if (!usrsp) {
    printk(KERN_ALERT "/proc/CDDx READ: Invalid device name %s", devfname);
    return -EINVAL;
  }

  // lock the CDD info for reading
  if (0 == down_read_trylock(thisCDD->CDD_sem))
    return -ERESTARTSYS;

  if (*eof!=0) { *eof=0; retval= 0; goto Done; }
  else {
    static char result1[64], result2[64], result3[64];

    strcpy(result1, get_CDD_usage(CDDCMD_DEVSIZE, thisCDD));
    strcpy(result2, get_CDD_usage(CDDCMD_DEVUSED, thisCDD));
    strcpy(result3, get_CDD_usage(CDDCMD_DEVOPENS, thisCDD));

    snprintf(buf,len,
       "Mode: %s\n"
       "%s(%d) Group Number: %d\n"
       "Team Members: %s, %s\n"
       "%s\n"
       "%s\n"
       "%s\n"
      , ((usrsp->CDD_procflag!=0)? "Written": "Readable")
      , devfname
      , minor
      , 42
      , "Mike Mehr"
      , usrsp->CDD_procvalue
      , result1
      , result2
      , result3
    );
    usrsp->CDD_procflag=0;
	}

  *eof = 1;
	retval= (strlen(buf));

Done:
  up_read(thisCDD->CDD_sem);
  return retval;
}


static ssize_t writeproc_CDD2 (struct file *file,const char * buf,
    size_t count, loff_t *ppos)
{
	int length=count;
  const char* devfname = file->f_path.dentry->d_name.name;
  struct CDDproc_struct *usrsp = lookup_CDDdproc(devfname);

  if (!usrsp) {
    printk(KERN_ALERT "/proc/CDDx READ: Invalid device name %s", devfname);
    return -EINVAL;
  }

	length = (length<CDD_PROCLEN)? length:CDD_PROCLEN;

	if (copy_from_user(usrsp->CDD_procvalue, buf, length))
		return -EFAULT;

	usrsp->CDD_procvalue[length-1]=0;
	usrsp->CDD_procflag=1;

	return(length);
}

static const struct file_operations proc_fops = {
 // .owner = THIS_MODULE,
 .read  = readproc_CDD2,
 .write = writeproc_CDD2,
};

int CDDproc_init(int minor_number)
{
  struct CDDproc_struct *usrsp=get_CDDdproc(minor_number);
  strcpy(usrsp->CDD_procname, get_devname(minor_number));
	usrsp->CDD_procvalue=vmalloc(4096);
  strcpy(usrsp->CDD_procvalue, "Team Member #2");
  usrsp->minor = minor_number;

  // Create the necessary proc entries
  if (topdir_entry_count() == 0){
    // when needed, create topdir and also marker feature
    // add the logfile marker utility /proc/CDD/marker
    // NOTE: this will also create the /proc/CDD folder if needed, since it calls create_CDD_procdir_entry
    CDDproc_log_marker_init();
  }
  usrsp->proc_entry = create_CDD_procdir_entry(usrsp->CDD_procname, 0777, &proc_fops);

	return 0;
}

void CDDproc_exit(int minor_number)
{
  struct CDDproc_struct *usrsp=get_CDDdproc(minor_number);

	vfree(usrsp->CDD_procvalue);
  usrsp->CDD_procvalue = NULL;

  if (topdir_entry_count() == 1) {
    // we will be removing the last entry except for /proc/CDD/marker
    // remove the logfile marker utility
    CDDproc_log_marker_exit();
  }

  // remove subdirectories and subtrees
	if (usrsp->proc_entry) {
    remove_CDD_procdir_entry(usrsp->CDD_procname);
    usrsp->proc_entry = NULL;
  }

}
