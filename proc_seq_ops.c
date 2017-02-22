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
//#define DEBUG_PARENT // uncomment this line to test using current->parent instead of current
#include "proc_seq_ops.h"

#define PROC_ENTRY		"myps" // proc entry name

typedef struct PSTaskSequenceIterator {
  //struct task_struct * task; // actually local to show() method
  struct list_head * pos;
  struct list_head * head;
} seqiter_t;

// global for testing iteration
/*
DISCUSSION:
The sequence of operations is a bit odd.
Empty lists cause START + STOP
Otherwise we get:
 START, SHOW NEXT STOP SHOW(EOL) START(?) STOP
So we need to protect all functions against invalid list access.
Also, STOP and START are sent when a page of output is full (4096 bytes)

This state machine is designed to detect and deal with these odd cases.
Perhaps I will hit on a better way of doing things with some more study.
*/
static struct CDDseqproc_struct {
  int iterating; // state sequencer (0==IDLE)
  int seen_start; // count how many starts
  int seen_stop; // count how many stops
  int seen_next;
  int seen_show;
  int length; // number of chars output so far ()
  int empty_list;
  int long_list;
} CDDseqproc; // = {0};

static void more_iters(int len) {
  int result = CDDseqproc.length + len; // relative
  if (len < 0)
    result = (len+1); // absolute
  CDDseqproc.empty_list = (result == 0) ? 1 : 0;
  CDDseqproc.long_list = (result >= 4096) ? 1 : 0;
  CDDseqproc.length = result;
}

static void start_iters(void) {
  CDDseqproc.iterating=0, CDDseqproc.seen_start=0, CDDseqproc.seen_show=0
  , CDDseqproc.seen_next=0, CDDseqproc.seen_stop=0;
  CDDseqproc.empty_list=1, CDDseqproc.long_list=0, CDDseqproc.length=0;
  more_iters(-1);
}

static int set_state(int new) {
  int old = CDDseqproc.iterating;
  CDDseqproc.iterating = new;
  return old;
}

static inline int get_state(void) { return CDDseqproc.iterating; }

// static int new_state(int override_prev) {
//   int state = CDDseqproc.iterating;
//   if (override_prev != -1)
//     {state = override_prev;}
//   switch (state) {
//     default: state = state + 1; break;
//   }
//   return state;
// }

static void next_iter_start(void) { ++CDDseqproc.seen_start; }
static void next_iter_stop(void) { ++CDDseqproc.seen_stop; }
static void next_iter_next(void) { ++CDDseqproc.seen_next; }
static void next_iter_show(void) { ++CDDseqproc.seen_show; }

static void print_iters(const char * msg) {
  printk(KERN_ALERT "%s => IT%d: Seen %d starts, %d shows, %d nexts, %d stops. LEN=%d%s%s\n"
    , msg, CDDseqproc.iterating, CDDseqproc.seen_start, CDDseqproc.seen_show
    , CDDseqproc.seen_next, CDDseqproc.seen_stop
    , CDDseqproc.length
    , (CDDseqproc.empty_list ? "-EMPTY" : "")
    , (CDDseqproc.long_list ? "-LONG" : "")
  );
}

/////////////
// Incorporate seq_file code from standard reference (used in ch.4 ex.4)
/////////////
#include <linux/seq_file.h>
#include <linux/slab.h> // for kmalloc/kfree

// MODULE_AUTHOR("Jonathan Corbet");
// MODULE_LICENSE("Dual BSD/GPL");


/*
 * The sequence iterator functions.  We simply use the count of the
 * next line as our internal position.
 */
 // MODS: On first entry (offset==0), we will initialize our iterator to point to the first child task
 // subsequent calls to start() might happen if list is long enough (>4096b page)
 // See comments on article here:https://lwn.net/Articles/22355/
static void *ct_seq_start(struct seq_file *s, loff_t *pos)
{
	seqiter_t *spos = kmalloc(sizeof(seqiter_t), GFP_KERNEL);
	if (! spos)
		return NULL;
    set_state(0);
  // convert pos offset to actual iterator struct
  // for us, only initialize if *pos==0, else just continue
  if (pos && *pos==0) {
    struct list_head *p = &current->children;
#ifdef DEBUG_PARENT // special testing just to make sure code works - use parent who always has a child (us)
    struct task_struct *q = current->parent;
    if (q) p = &q->children;
#endif
    start_iters();
    if (list_empty(p)) {
      next_iter_start();   print_iters("MyPS SEQ: START(E)");
      printk(KERN_ALERT "Myps SEQ: no children to iterate in task %s (pid %d).\n", current->comm, (int) current->pid);
      return NULL;
    }
    next_iter_start();   print_iters("MyPS SEQ: START");
    spos->head = p;
    spos->pos = p->next;
    // spos->task = NULL; // don't dereference this until we know there is a child
  }
	//*spos = *pos;
	return spos;
}

static void *ct_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	seqiter_t *spos = (seqiter_t *) v;
	*pos += 1; // next offset
  set_state(1);
  next_iter_next();   print_iters("MyPS SEQ: NEXT");
  // advance the iterator
  // once we have initialized the list, we can get this from the next field directly
  //412         for (pos = (head)->next; pos != (head); pos = pos->next)
  spos->pos = spos->pos->next;
  if (spos->pos == spos->head) {
    set_state(2); // detected end of list here
    return NULL;
  }
	return spos;
}

static void ct_seq_stop(struct seq_file *s, void *v)
{
  // according to the article above, this can be called multiple times, including up to twice when done
  // EMPTY LIST: start stop
  // ONE PAGE OR LESS: start(pos=0) [ show(pos) next(pos) ]+ stop start(pos==N) stop
  // LONGER THAN 4kb PAGE:
  //  start(pos=0) [ show(pos) next(pos) ]+ [ start(pos in middle) stop ]+(**) stop start(pos==N) stop
  //  (**) Actually, I'm unclear when the start/stop in the middle happen rel.to show/next
  if (get_state() == 0) {
    // if previous op was START, we're actually terminating
    kfree (v);
    printk(KERN_ALERT "MyPS SEQ: Freed up iterator memory (pid=%d).", (int)current->pid);
  }
  next_iter_stop();   print_iters("MyPS SEQ: STOP");
}

/*
 * The show function.
 */
static int ct_seq_show(struct seq_file *s, void *v)
{
	seqiter_t *spos = (seqiter_t *) v;
	// seq_printf(s, "%Ld\n", *spos);
  if (get_state() != 2) {
    // DO NOT SHOW RECORD IF INVALID - at end of list
    // NOTE: NEXT will not allow spos to be updated if at end of list (returns NULL), so we must use ITS internal test
    struct task_struct* task = list_entry(spos->pos, struct task_struct, sibling);
    // seems to get here in spite of protections
    int tpid = task->pid;
    int oops = (tpid < 0 || tpid > 50000); // semi-BOGUS test!
    if (oops) {
      printk(KERN_ALERT "Myps SEQ: child whoops: IT%d (pid %d).\n", get_state(), tpid);
    } else {
      printk(KERN_ALERT "Myps SEQ: child iterated: task %s (pid %d).\n", task->comm, (int) task->pid);
      seq_printf(s, "Task %s (pid %d), child of %s (pid %d).\n", task->comm, (int) task->pid, task->parent->comm, (int) task->parent->pid);
      set_state(1); // only prevent kfree on STOP if still iterating list
    }
  }
  next_iter_show();   print_iters("MyPS SEQ: SHOW");
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
 * The file operations structure contains our open function along with
 * set of the canned seq_ ops.
 */
static struct file_operations ct_file_ops = {
	.owner   = THIS_MODULE,
	.open    = ct_open,
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
