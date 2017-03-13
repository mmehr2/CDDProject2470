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
#include <linux/mutex.h>
#include <linux/fdtable.h> // for struct files_struct

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

#include "proc_seq_ops.h"

#define PROC_ENTRY		"myps" // proc entry name

typedef struct PSTaskSequenceIterator {
  //struct task_struct * task; // actually local to show() method
  struct list_head * pos;
  struct list_head * head;
  loff_t pos_max;
  struct pid* ptpid; // for getting at the task_struct we are iterating
} seqiter_t;

typedef struct  PSProcRef {
  pid_t testid; // PID of task o report on, or 0 for current task
  // See discussion in <linux/pid.h> for why this is preferred to pid_t or task_struct*
  // DECISION: use the numeric pid_t anyway - unlikely to wrap PID counter
  struct mutex mtx; // lock for this struct
} myps_t;

static myps_t myps_data;

/////////////
// Incorporate seq_file code from standard reference (used in ch.4 ex.4)
/////////////
// MODULE_AUTHOR("Jonathan Corbet");
// MODULE_LICENSE("Dual BSD/GPL");


/*
 * The sequence iterator functions.
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
  myps_t* usrsp = &myps_data; // currently only one global structure
  struct task_struct *test_task;
	if (! spos)
		return NULL;
  // convert pos offset to actual iterator struct
  // for us, only initialize if *pos==0, else just continue
  if (*pos==0) {
    struct list_head *p;
    spos->ptpid = find_vpid(current->pid);
    test_task = current;
    if (mutex_trylock(&usrsp->mtx)) {
      // Hwk 5.2 - if someone has written a PID to use, use it (once only)
      if (usrsp->testid != 0) {
        spos->ptpid = find_vpid(usrsp->testid);
        test_task = pid_task(spos->ptpid, PIDTYPE_PID);
        usrsp->testid = 0;
      } else {
        // contention? just use current? no we should retry somehow TBD NOTE: learn how!
      }
      mutex_unlock(&usrsp->mtx);
    }
    p = &test_task->children;
#ifdef DEBUG_PARENT // special testing just to make sure code works - use parent who always has a child (us)
    test_task = current->parent;
  #ifdef DB2
    if (test_task) {p = &test_task->children; test_task=test_task->parent;}
    if (test_task) {p = &test_task->children; test_task=test_task->parent;}
    if (test_task) {p = &test_task->children; test_task=test_task->parent;}
    if (test_task) {p = &test_task->children; test_task=test_task->parent;}
  #endif
    if (test_task) p = &test_task->children;
#endif
    if (list_empty(p)) {
      printk(KERN_ALERT "Myps SEQ: no children to iterate in task %s (pid %d).\n", test_task->comm, (int) test_task->pid);
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
    if (sspos == NULL) {
      printk(KERN_ALERT "Myps SEQ: final(?) START unable to retrieve iterator from filp, nonzero *pos=%d.\n", (int)*pos);
      return NULL;
    } else if (*pos != sspos->pos_max) {
      // this is the intermediate case for restart in middle of big file output
      // we want to return the old spos, which is still kept in the private field of struct seq_file
      // free the new spos first
      kfree(spos);
      spos = sspos;
      test_task = pid_task(spos->ptpid, PIDTYPE_PID);
      printk(KERN_ALERT "Myps SEQ: middle re-START/free call in task %s (pid %d).\n", test_task->comm, (int) test_task->pid);
    } else {
      // this is where the iterator is pointing to the end of the data (actually at N, or one off the end)
      test_task = pid_task(sspos->ptpid, PIDTYPE_PID);
      printk(KERN_ALERT "Myps SEQ: final START call in task %s (pid %d).\n", test_task->comm, (int) test_task->pid);
      return NULL;
    }
  }
	//*spos = *pos;
  printk(KERN_ALERT "Myps SEQ: first START call in task %s (pid %d) with %d children.\n"
    , test_task->comm, (int) test_task->pid, (int) spos->pos_max);
	return spos;
}

static void *ct_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	seqiter_t *spos = (seqiter_t *) v;
  struct task_struct * test_task = pid_task(spos->ptpid, PIDTYPE_PID);
  // advance the iterator
  // once we have initialized the list, we can get this from the next field directly
  //412         for (pos = (head)->next; pos != (head); pos = pos->next)
  spos->pos = spos->pos->next;
  if (spos->pos == spos->head) {
    printk(KERN_ALERT "Myps SEQ: final NEXT call in task %s (pid %d).\n", test_task->comm, (int) test_task->pid);
    return NULL;
  }
  printk(KERN_ALERT "Myps SEQ: NEXT call in task %s (pid %d).\n", test_task->comm, (int) test_task->pid);
	return spos;
}

static void ct_seq_stop(struct seq_file *s, void *v)
{
  seqiter_t *spos = (seqiter_t *) v;
  struct task_struct * test_task;
  if (v == NULL) {
    printk(KERN_ALERT "MyPS SEQ: STOP NULL iterator.\n"); // (pid=%d).\n", (int)test_task->pid);
    return;
  }
  test_task = pid_task(spos->ptpid, PIDTYPE_PID);
  kfree (v);
  printk(KERN_ALERT "MyPS SEQ: STOP freed up iterator memory (pid=%d).\n", (int)test_task->pid);
}

/*
 * The show function.
 */
 // I use a dynamic dictionary structure to represent the (possibly sparse) list of property name/value pairs
typedef long prop_table_index_t;
typedef struct prop_table_map_st {
  prop_table_index_t property;
  const char* name;
} prop_table_mapval_t;

static const char* lookup_name_helper(const prop_table_mapval_t table[], prop_table_index_t index) {
  const prop_table_mapval_t* pv = table;
  while (pv->name != NULL) {
    if (pv->property == index)
      return pv->name;
    ++pv;
  }
  return "__UNKNOWN__";
}

static prop_table_mapval_t task_state_names[] = {
  {TASK_RUNNING, "TASK_RUNNING"},
  {TASK_INTERRUPTIBLE, "TASK_INTERRUPTIBLE"},
  {TASK_UNINTERRUPTIBLE, "TASK_UNINTERRUPTIBLE"},
  {__TASK_TRACED, "__TASK_TRACED"},
  {__TASK_STOPPED, "__TASK_STOPPED"},
  {0, NULL}, // must always be last entry in a map to terminate lookup
};


static int ct_seq_show(struct seq_file *s, void *v)
{
	seqiter_t *spos = (seqiter_t *) v;
  //struct task_struct * test_task = pid_task(spos->ptpid, PIDTYPE_PID);
	// seq_printf(s, "%Ld\n", *spos);
  struct task_struct* task = list_entry(spos->pos, struct task_struct, sibling);
  int open_files = ((task->files)? atomic_read(&task->files->count) : 0);

  printk(KERN_ALERT "Myps SEQ: child shown: task %s (pid %d).\n", task->comm, (int) task->pid);
  seq_printf(s, "Task %s (pid %d), child of %s (pid %d). State=%ld(%s), priority=%d(RT=%d). %d open files.\n"
    , task->comm, (int) task->pid
    , task->parent->comm, (int) task->parent->pid
    , task->state, lookup_name_helper(task_state_names, task->state)
    , task->prio, (int) task->rt_priority
    , open_files
  );

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

/*
 * Hwk. Ch.5.2
 * ct_write()
 * Accepts a number, representing an active task PID.
 *  If the task is found, the number is stored, and the next call to open the
 *  file will use this PID's task for analysis instead of the current task.
 */

static ssize_t ct_write (struct file *file, const char *buf,
  size_t count, loff_t *ppos)
{
  ssize_t retval  = 0;
  long testid = -1;
  int err;
  struct task_struct* ftask;
  size_t len = max(count+1, (size_t)10);
  char* buf2 = kmalloc(len, GFP_KERNEL);
  myps_t* usrsp = &myps_data; // currently only one global structure

  if (buf2 == NULL)
    return -ENOMEM;
  strcpy(buf2,"TEST!");

// enter critical section - read from user space, write to data struct
  if (0 == mutex_trylock(&usrsp->mtx)) {
    retval = -ERESTARTSYS;
    goto Done;
  }

  strncpy_from_user(buf2, buf, count);
  buf2[count] = 0;
  // can we do atoi()?, no but we have kstrtoXXX() family
  err = kstrtol(buf2, 0, &testid);
  if (err == -ERANGE) {
    printk(KERN_ALERT "Myps WRITE-ERANGE: test PID overflow %s.\n", buf2);
    retval = err;
    goto DoneSem;
  } else if (err == -EINVAL) {
    printk(KERN_ALERT "Myps WRITE-EINVAL: test PID invalid number %s.\n", buf2);
    retval = err;
    goto DoneSem;
  }
  printk(KERN_ALERT "Myps WRITE: test PID set to %s(%ld).\n", buf2, testid);
  // test if this is a valid pid by fuinding the task associated
  ftask = pid_task(find_vpid(testid), PIDTYPE_PID);
  if (ftask == NULL) {
    printk(KERN_ALERT "Myps WRITE: task with PID=%ld not found.\n", testid);
    retval = -EINVAL;
    // actually should probably revert to current task info
    goto DoneSem;
  }

  // actually acquire a mutex and set the proper variable here
  printk(KERN_ALERT "Myps WRITE: found task named %s(pid=%d).\n", ftask->comm, ftask->pid);
  usrsp->testid = ftask->pid;
  retval = count;

DoneSem:
  // exit critical section before exiting
  mutex_unlock(&usrsp->mtx);
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

  mutex_init(&myps_data.mtx); // hwk5.2
  myps_data.testid = 0; // use current by default

	return 0;
}

void CDDproc_seq_exit(void)
{

	remove_proc_entry (PROC_ENTRY, NULL);

}
