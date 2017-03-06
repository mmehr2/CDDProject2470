#ifndef PROC_OPS_H
#define PROC_OPS_H

extern int CDDproc_init(void);
extern void CDDproc_exit(void);

struct proc_dir_entry;
extern struct proc_dir_entry * get_CDD_dir_entry(void);

#endif
