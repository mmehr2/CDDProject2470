# CDDProject2470

This is the ongoing project work for Raghav Vinjamuri's current class "Linux Device Drivers", ref.number 2470, University of California, Silicon Valley Extension, Winter 2016 term.

The following are the assignments as taken from current files of the class notes PDFs.
I have edited for readability in Markdown.

These can be considered the specs of a multi-capable "Frankenstein" character device driver that is used to show off our capabilities and what we have learned in class.

I will try to mark the source with references such as 4.2.a for Chapter 4, part 2, step a.
In the case there are not such designations, I'll wing it.

# Group Homework .. Chapter 3
1. Write a simple char device driver

  * Major Number should be passed in, as a parameter.
    If the passed-in parameter value is 0, then you should dynamically request a Major
number.

  * It should support open(), release(), read(), write() for each device – i.e. minor#.
    The device methods should be in a separate file.

  * It should correctly support device open flags (.. say) O_APPEND, O_WRONLY,
O_TRUNC, O_RDONLY .. ;

  * You’d need to support a minimum of two flags .. say O_APPEND and O_TRUNC.

  * Add a char device into /dev with the mknod command.
  
  * Write a program to read/write from this device, using UNIX System Calls ..
open(), read(), write() and close().

2. What to turn-in: working program source and, sample output.

# Group Homework .. Chapter 4
1. Using the /proc filesystem

  a. Modify your char device driver in Chapter #3 to create a read-only /proc filesystem
entry myCDD.. under the /proc/CDD directory.
    Please have all your functions pertaining to the /proc filesystem, in a separate file ..
in addition to the main .c file, and the file for your device methods.

  b. Display “Group Number: <your Group Number>”.
    For this assignment question, use the group number passed in as a parameter.

  c. Display “Team Members: <your Name, your Name, ..>”
    You can hard-code your Names in the Driver ...

  d. Display “Buffer Length - Allocated: <length of buffer allocated>”.
    For this assignment question, provide the length of the internal buffer in the CDD
that is used to hold content accessed by read() or write().

  e. Display “Buffer Length - Used: <length of buffer used>”.
    For this assignment question, use the string written to the CDD.

  f. Enable the /proc filesystem entry above, for writes.

  g. What to turn-in: module C program source and Makefile, Sample Output.

2. Extra Credit

  a. Use the seq_file example to create a /proc/myps entry that will print one line of
output per process in the task_struct. Print(decode) the task flags.

## Note:
In /proc/kallsyms, a lowercase character designates the symbol as local (or non-exported). An
uppercase character should designate the symbol as global (or external).

# Group Homework .. Chapter 5
1. Using semaphores & Spinlocks

  a. Modify your CDD in Chapter #4 to implement a mutex for the copy_from_user() and
copy_to_user() functions for read() and write() calls.

  b. Enhance your CDD to keep track of the cumulative number of times open() is called on
CDD. Use a spinlock to protect accesses to the #opencalls counter.

  c. Display “# Opens: <#opens>” in the read-only /proc filesystem entry .. in the CDD of
Chapter #4.

2. Using the seq-file based /proc/myps from previous chapter, to,

  a. Enable for writes and accept a PID

  b. If PID is found, print process state & process priorities for that specific process
Test using userspace application that resets its scheduler priority – for dynamic & RT..

  c. Also, for same PID, print the fd’s that are open in a specific process.
Test using userspace application that opens multiple/various files.

What to turn-in: module C program source and Makefile, Sample Output.
