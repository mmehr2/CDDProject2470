#ifndef PROC_OPS_H
#define PROC_OPS_H

//initialization and shutdown of /proc/CDD
extern int CDDproc_init(int minor_number);
extern void CDDproc_exit(int minor_number);

// directory services to create/remove entries in /proc/CDD
struct proc_dir_entry; // fwd.ref.
struct file_operations; // fwd.ref.
extern struct proc_dir_entry * create_CDD_procdir_entry(const char* name, int permissions, const struct file_operations* fops);
extern void remove_CDD_procdir_entry(const char* name);

#endif
