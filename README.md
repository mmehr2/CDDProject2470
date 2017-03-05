# CDDProject2470

This is the ongoing project work for Raghav Vinjamuri's current class "Linux Device Drivers", ref.number 2470, University of California, Silicon Valley Extension, Winter 2016 term.

The following are the assignments as taken from current files of the class notes PDFs.
I have edited for readability in Markdown.

These can be considered the specs of a multi-capable "Frankenstein" character device driver that is used to show off our capabilities and what we have learned in class.

I will try to mark the source with references such as 4.2.a for Chapter 4, part 2, step a.
In the case there are not such designations, I'll wing it.

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
