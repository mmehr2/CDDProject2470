#define	EXPORT_SYMBOLS
#define	EXPORT_SYMTAB
#include <linux/version.h>

#include <linux/module.h>
#include <linux/kernel.h>
#include <asm/uaccess.h> // for copy_*_user
#include <linux/proc_fs.h>
#include <linux/vmalloc.h>

#define CDD		"marker" // proc entry
#define myCDD  		"CDD" // proc outer directory

#define CDD_PROCLEN     256

#include "proc_ops.h" // for get_CDD_dir_entry()
//static struct proc_dir_entry *proc_topdir;
static struct proc_dir_entry *proc_entry;

struct CDDproc_struct {
        char CDD_procname[CDD_PROCLEN + 1];
        char *CDD_procvalue;
        char CDD_procflag;
};

static struct CDDproc_struct CDDproc;

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

  if (*eof!=0) { *eof=0; retval= 0; goto Done; }
  else {

    snprintf(buf,len,
       "%s"
      , usrsp->CDD_procvalue
    );
    usrsp->CDD_procflag=0;
	}

  *eof = 1;
	retval= (strlen(buf));

Done:
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

  // perform marker functions
  printk(KERN_ALERT "%s\n", usrsp->CDD_procvalue);

	return(length);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static const struct file_operations proc_fops = {
 // .owner = THIS_MODULE,
 .read  = readproc_CDD2,
 .write = writeproc_CDD2,
};
#endif

int CDDproc_log_marker_init(void)
{
	CDDproc.CDD_procvalue=vmalloc(CDD_PROCLEN);
  strcpy(CDDproc.CDD_procvalue, "~~~ UNIQUE KERNEL MARKER ~~~");

  // Create the necessary proc entries
  // NOTE: It is assumed that /proc/CDD is already created by another part of this driver
  //proc_topdir = get_CDD_dir_entry();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
  proc_entry = proc_create(CDD, 0777, get_CDD_dir_entry(), &proc_fops);
#else
  proc_entry = create_proc_entry(CDD,0,get_CDD_dir_entry());
  proc_entry->read_proc = readproc_CDD2;
  proc_entry->write_proc = writeproc_CDD2;
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,29)
  proc_entry->owner = THIS_MODULE;
#endif

	return 0;
}

void CDDproc_log_marker_exit(void)
{

	vfree(CDDproc.CDD_procvalue);

	if (proc_entry) remove_proc_entry (CDD, get_CDD_dir_entry());

}
