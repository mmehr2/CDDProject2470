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

#define CDD		"myCDD2" // proc entry
#define myCDD  		"CDD" // proc outer directory

#define CDD_PROCLEN     32


static struct proc_dir_entry *proc_topdir;
static struct proc_dir_entry *proc_entry;

struct CDDproc_struct {
        char CDD_procname[CDD_PROCLEN + 1];
        char *CDD_procvalue;
        char CDD_procflag;
};

static struct CDDproc_struct CDDproc;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static int eof[1];

static ssize_t read_hello(struct file *file, char *buf,
	size_t len, loff_t *ppos)
#else
static int readproc_CDD2(char *buf, char **start,
  off_t offset, int len, int *eof, void *unused)
#endif
{
  struct CDDproc_struct *usrsp=&CDDproc;

  if (*eof!=0) { *eof=0; return 0; }
  else if (usrsp->CDD_procflag) {
  	usrsp->CDD_procflag=0;

    snprintf(buf,len,"Hello..I got \"%s\"\n",usrsp->CDD_procvalue);
	}
  else
   	snprintf(buf,len,"Hello from process %d\n", (int)current->pid);

  *eof = 1;
	return (strlen(buf));
}


#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static ssize_t write_hello (struct file *file,const char * buf,
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
 .read  = read_hello,
 .write = write_hello,
};
#endif

int CDDproc_init(void)
{
	CDDproc.CDD_procvalue=vmalloc(4096);

  // Create the necessary proc entries
  proc_topdir = proc_mkdir(myCDD,0);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
  proc_entry = proc_create(CDD, 0777, proc_topdir, &proc_fops);
#else
  proc_entry = create_proc_entry(CDD,0,proc_topdir);
  proc_entry->read_proc = readproc_CDD2;
  proc_entry->write_proc = writeproc_CDD2;
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,29)
  proc_entry->owner = THIS_MODULE;
#endif

	return 0;
}

void CDDproc_exit(void)
{
	vfree(CDDproc.CDD_procvalue);

	if (proc_entry) remove_proc_entry (CDD, proc_topdir);
 	if (proc_topdir) remove_proc_entry (myCDD, 0);

}
