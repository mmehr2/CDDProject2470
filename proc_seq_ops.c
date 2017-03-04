#define	EXPORT_SYMBOLS
#define	EXPORT_SYMTAB
#include <linux/version.h>
#include <linux/module.h>
#include <linux/kernel.h>
// #include <asm/uaccess.h> // for copy_*_user
#include <linux/proc_fs.h>
// #include <linux/vmalloc.h>
#include <linux/sched.h>                // for struct_task
#include <linux/list.h>  // for list macros

#include <linux/seq_file.h>
#include <linux/slab.h> // for kmalloc/kfree
#include <asm/uaccess.h> // for user space transfers

// DISCUSSION: To iterate the children list, we want the following
// From article here: http://www.informit.com/articles/article.aspx?p=368650:
//
// // Similarly, it is possible to iterate over a process's children with
// //
// // struct task_struct *task;
// // struct list_head *list;
// //
// // list_for_each(list, &current->children) {
// //     task = list_entry(list, struct task_struct, sibling);
// //     /* task now points to one of current's children */
// // }
//
// So we need to break the for_each macro down into individual steps.
//406 /**
//407  * list_for_each        -       iterate over a list
//408  * @pos:        the &struct list_head to use as a loop cursor.
//409  * @head:       the head for your list.
//410  */
//411 #define list_for_each(pos, head)
//412         for (pos = (head)->next; pos != (head); pos = pos->next)
//413
// // THUS:
// // struct task_struct *task;
// // struct list_head *list; // same as pos
// // struct list_head *head;
// //
// // ITERATOR INIT:
// // head = &(current->children);
// // list = (head)->next;
// //
// // ITERATOR TEST:
// // while (list != head) {
// //     ITERATOR DE-REFERENCE:
// //     task = list_entry(list, struct task_struct, sibling);
// //     /* task now points to one of current's children */
// //     ITERATOR ADVANCE:
// //     list = list->next;
// // }
//

/*
DOCUMENTING CURRENT OUTPUT
This does NOT work like either article talks about, OR something else is wrong.
Here is the current output of this code, which causes a crash (Seg Fault):

Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231345] MyPS SEQ: START => IT0: Seen 1 starts, 0 shows, 0 nexts, 0 stops. LEN=0-EMPTY
Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231347] Myps SEQ: child iterated: task cat (pid 18636).
Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231349] MyPS SEQ: SHOW => IT1: Seen 1 starts, 1 shows, 0 nexts, 0 stops. LEN=0-EMPTY
Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231351] MyPS SEQ: NEXT => IT1: Seen 1 starts, 1 shows, 1 nexts, 0 stops. LEN=0-EMPTY
Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231352] MyPS SEQ: STOP => IT2: Seen 1 starts, 1 shows, 1 nexts, 1 stops. LEN=0-EMPTY
Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231404] Myps SEQ: child whoops: IT0 (pid 1769366884).
Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231406] MyPS SEQ: SHOW => IT0: Seen 1 starts, 2 shows, 1 nexts, 1 stops. LEN=0-EMPTY
Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231407] MyPS SEQ: NEXT => IT1: Seen 1 starts, 2 shows, 2 nexts, 1 stops. LEN=0-EMPTY
Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231408] Myps SEQ: child iterated: task  (pid 1).
Feb 22 12:26:44 mike-VirtualBox kernel: [  548.231423] general protection fault: 0000 [#1] SMP

It should have one valid child, since we are using the DEBUG_PARENT code.
It calls SHOW for the valid child.
It calls NEXT, which results NULL at state 2.
It STOP without freeing RAM, since state is 2.
Then it calls SHOW with an invalid PID, and by then the state is 0 again (huh?)
It calls NEXT at state 1, which should not happen since NEXT already returned NULL. What does it return here?
Then it calls SHOW again, the PID seems to be 1 (random data?) which is valie, and CRASHOLA!

No time to figure all this out. I'll ask Raghav on Friday. For now, let it go.

*/

#include "proc_seq_ops.h"

#define PROC_ENTRY		"myps" // proc entry name

typedef struct PSTaskSequenceIterator {
  //struct task_struct * task; // actually local to show() method
  struct list_head * pos;
  struct list_head * head;
  loff_t pos_max;
} seqiter_t;

/////////////
// Incorporate seq_file code from standard reference (used in ch.4 ex.4)
/////////////
// MODULE_AUTHOR("Jonathan Corbet");
// MODULE_LICENSE("Dual BSD/GPL");


/*
 * The sequence iterator functions.  We simply use the count of the
 * next line as our internal position.
 */
 // MODS: On first entry (offset==0), we will initialize our iterator to point to the first child task
 // subsequent calls to start() might happen if list is long enough (>4096b page)
 // See comments on article here:https://lwn.net/Articles/22355/
 //#define DEBUG_PARENT // uncomment this line to test using current->parent instead of current
 #ifdef DEBUG_PARENT
 //#define BIG_ENTRY // uncomment this to try code to simulate seq_printf() of >4096 bytes (causes extra stops and starts on page bounds)
 #define DB2 // uncomment this line to test using carefully crafted parent that has >1 child
 // NOTE: this uses pstree -p | grep bash to find the tree of tasks
 /*
 pstree -p | grep bash
            |              |               |               |-gnome-terminal-(2074)-+-bash(2080)---sudo(2173)---su(2195)---bash(2196)+
            |              |               |               |                       |-bash(2135)---sudo(2148)---su(2152)---bash(2153)+

 */
#endif

static void *ct_seq_start(struct seq_file *s, loff_t *pos)
{
	seqiter_t *spos = kmalloc(sizeof(seqiter_t), GFP_KERNEL);
	if (! spos)
		return NULL;
  // convert pos offset to actual iterator struct
  // for us, only initialize if *pos==0, else just continue
  if (*pos==0) {
    struct list_head *p = &current->children;
#ifdef DEBUG_PARENT // special testing just to make sure code works - use parent who always has a child (us)
    struct task_struct *q = current->parent;
  #ifdef DB2
    if (q) {p = &q->children; q=q->parent;}
    if (q) {p = &q->children; q=q->parent;}
    if (q) {p = &q->children; q=q->parent;}
    if (q) {p = &q->children; q=q->parent;}
  #endif
    if (q) p = &q->children;
#endif
    if (list_empty(p)) {
      printk(KERN_ALERT "Myps SEQ: no children to iterate in task %s (pid %d).\n", current->comm, (int) current->pid);
      return NULL;
    }
    spos->pos_max = *pos;
    { // declaration block
    struct list_head *list;
    list_for_each(list, p) {
      ++spos->pos_max; // count children in advance
    }
  } // end declaration block
    spos->head = p;
    spos->pos = p->next;
    // spos->task = NULL; // don't dereference this until we know there is a child
  } else {
    // this is the START called after the list ends; must return NULL here to terminate sequencing
    // see article here for a simple diagram: http://www.tldp.org/LDP/lkmpg/2.6/html/x861.html
    // TEST: see if this is still valid for kernel 4.9+
    // ALSO, obviously this won't handle multipage STOP/START sequences...but first things first
    // We would have to be able to tell JUST FROM *pos whether we are at the end of the list or not.
    // This would only work if we knew the number of items in the list, which is not possible in the general case.
    // For a task list, the number of children can be found I would think.
    seqiter_t *sspos = s->private;
    if (sspos && *pos != sspos->pos_max) {
      // this is the intermediate case for restart in middle of big file output
      // we want to return the old spos, which is still kept in the private field of struct seq_file
      // free the new spos first
      kfree(spos);
      spos = sspos;
      printk(KERN_ALERT "Myps SEQ: middle re-START/free call in task %s (pid %d).\n", current->comm, (int) current->pid);
    } else {
      printk(KERN_ALERT "Myps SEQ: final START call in task %s (pid %d).\n", current->comm, (int) current->pid);
      return NULL;
    }
  }
	//*spos = *pos;
  printk(KERN_ALERT "Myps SEQ: first START call in task %s (pid %d).\n", current->comm, (int) current->pid);
	return spos;
}

static void *ct_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	seqiter_t *spos = (seqiter_t *) v;
  // advance the iterator
  // once we have initialized the list, we can get this from the next field directly
  //412         for (pos = (head)->next; pos != (head); pos = pos->next)
  spos->pos = spos->pos->next;
  if (spos->pos == spos->head) {
    printk(KERN_ALERT "Myps SEQ: final NEXT call in task %s (pid %d).\n", current->comm, (int) current->pid);
    return NULL;
  }
  printk(KERN_ALERT "Myps SEQ: NEXT call in task %s (pid %d).\n", current->comm, (int) current->pid);
	return spos;
}

static void ct_seq_stop(struct seq_file *s, void *v)
{
  kfree (v);
  printk(KERN_ALERT "MyPS SEQ: STOP freed up iterator memory (pid=%d).\n", (int)current->pid);
}

/*
 * The show function.
 */
static int ct_seq_show(struct seq_file *s, void *v)
{
	seqiter_t *spos = (seqiter_t *) v;
	// seq_printf(s, "%Ld\n", *spos);
  struct task_struct* task = list_entry(spos->pos, struct task_struct, sibling);

  printk(KERN_ALERT "Myps SEQ: child shown: task %s (pid %d).\n", task->comm, (int) task->pid);
  seq_printf(s, "Task %s (pid %d), child of %s (pid %d).\n", task->comm, (int) task->pid, task->parent->comm, (int) task->parent->pid);

#ifdef BIG_ENTRY
{ // declaration block
  int i, len;
  // create paging sequence
  len = 4000; // how big an entry we want to simulate
  for (i=0; i<len; ++i) {
    seq_putc(s, ('0' + (i % 10)));
    if (0 == ((i+1) % 40)) seq_putc(s, '\n');
    if (0 == ((i+1) % 400)) seq_putc(s, '\n');
  }
  seq_putc(s, '\n'); // final flush
} // end declaration block
#endif

	return 0;
}

/*
 * Tie them all together into a set of seq_operations.
 */
static struct seq_operations ct_seq_ops = {
	.start = ct_seq_start,
	.next  = ct_seq_next,
	.stop  = ct_seq_stop,
	.show  = ct_seq_show
};


/*
 * Time to set up the file operations for our /proc file.  In this case,
 * all we need is an open function which sets up the sequence ops.
 */

static int ct_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &ct_seq_ops);
};

// #if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
// static int write_hello (struct file *file,const char * buf,
//     size_t count, loff_t *ppos)
// #else
// static int writeproc_CDD2(struct file *file,const char *buf,
//                 unsigned long count, void *data)
// #endif
// {
// 	int length=count;
// 	struct CDDproc_struct *usrsp=&CDDproc;
//
// 	length = (length<CDD_PROCLEN)? length:CDD_PROCLEN;
//
// 	if (copy_from_user(usrsp->CDD_procvalue, buf, length))
// 		return -EFAULT;
//
// 	usrsp->CDD_procvalue[length-1]=0;
// 	usrsp->CDD_procflag=1;
//
// 	return(length);
// }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
static ssize_t ct_write (struct file *file, const char *buf,
  size_t count, loff_t *ppos)
{
  ssize_t retval  = 0;
#else
static int ct_write(struct file *file,const char *buf,
  unsigned long count, void *data)
  {
    int retval = 0;
#endif
  long testid = -1;
  int err;
  struct task_struct* ftask;
  size_t len = max(count+1, (size_t)10);
  char* buf2 = kmalloc(len, GFP_KERNEL);
  if (buf2 == NULL)
    return -ENOMEM;
  strcpy(buf2,"TEST!");

  strncpy_from_user(buf2, buf, count);
  buf2[count] = 0;
  // can we do atoi()?, no but we have kstrtoXXX() family
  err = kstrtol(buf2, 0, &testid);
  if (err == -ERANGE) {
    printk(KERN_ALERT "Myps WRITE-ERANGE: test PID overflow %s.\n", buf2);
    retval = err;
    goto Done;
  } else if (err == -EINVAL) {
    printk(KERN_ALERT "Myps WRITE-EINVAL: test PID invalid number %s.\n", buf2);
    retval = err;
    goto Done;
  }
  printk(KERN_ALERT "Myps WRITE: test PID set to %s(%ld).\n", buf2, testid);
  // test if this is a valid pid by fuinding the task associated
  ftask = pid_task(find_vpid(testid), PIDTYPE_PID);
  if (ftask == NULL) {
    printk(KERN_ALERT "Myps WRITE: task with PID=%ld not found.\n", testid);
    retval = -EINVAL;
    // actually should probably revert to current task info
    goto Done;
  }
  printk(KERN_ALERT "Myps WRITE: found task named %s(pid=%d).\n", ftask->comm, ftask->pid);
  // actually acquire a semaphore and set the proper variable here
  retval = count;
Done:
  kfree(buf2);
  return retval;
};

/*
 * The file operations structure contains our open function along with
 * set of the canned seq_ ops.
 */
static struct file_operations ct_file_ops = {
	.owner   = THIS_MODULE,
	.open    = ct_open,
  .write   = ct_write, // added hwk.5.2
	.read    = seq_read,
	.llseek  = seq_lseek,
	.release = seq_release
};

int CDDproc_seq_init(void)
{
  struct proc_dir_entry* proc_entry;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0)
  proc_entry = proc_create(PROC_ENTRY, 0777, NULL, &ct_file_ops);
#else
  proc_entry = create_proc_entry(CDD,0,NULL);
  proc_entry->open_proc = ct_open;
  proc_entry->read_proc = seq_read;
  proc_entry->llseek_proc = seq_lseek;
  proc_entry->release_proc = seq_release;
  proc_entry->write_proc = ct_write; // added hwk.5.2
#endif

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,29)
  proc_entry->owner = THIS_MODULE;
#endif

	return 0;
}

void CDDproc_seq_exit(void)
{

	remove_proc_entry (PROC_ENTRY, NULL);

}
