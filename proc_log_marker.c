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

// only need one global string buffer, and maybe even just static const
static const char* CDDmarker = "UNIQUE-KERNEL-MARKER";

static int eof[1];

static ssize_t readproc_CDD2(struct file *file, char *buf,
	size_t len, loff_t *ppos)
  {
    ssize_t retval = 0;

  if (*eof!=0) { *eof=0; retval= 0; goto Done; }
  else {
    // just copy the static marker string into user space
    snprintf(buf,len,
       "%s", CDDmarker
    );
	}

  *eof = 1;
	retval= (strlen(buf));

Done:
  return retval;
}


static ssize_t writeproc_CDD2 (struct file *file,const char * buf,
    size_t count, loff_t *ppos)
{
  // DECISION: No need for custom markers
  // perform marker functions
  printk(KERN_ALERT "%s\n", CDDmarker);

	return (count);
}

static const struct file_operations proc_fops = {
 // .owner = THIS_MODULE,
 .read  = readproc_CDD2,
 .write = writeproc_CDD2,
};

int CDDproc_log_marker_init(void)
{
  // Create the necessary proc entries
  // NOTE: It is assumed that /proc/CDD is already created by another part of this driver
  //proc_topdir = get_CDD_dir_entry();

  proc_entry = create_CDD_procdir_entry(CDD, 0777, &proc_fops);

	return 0;
}

void CDDproc_log_marker_exit(void)
{

	if (proc_entry) remove_CDD_procdir_entry (CDD);

}
