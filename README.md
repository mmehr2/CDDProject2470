# CDDProject2470

This is the ongoing project work for Raghav Vinjamuri's current class "Linux Device Drivers", ref.number 2470, University of California, Silicon Valley Extension, Winter 2016 term.

The following are the assignments as taken from current files of the class notes PDFs.
I have edited for readability in Markdown.

These can be considered the specs of a multi-capable "Frankenstein" character device driver that is used to show off our capabilities and what we have learned in class.

I will try to mark the source with references such as 4.2.a for Chapter 4, part 2, step a.
In the case there are not such designations, I'll wing it.

# Lab Assignment Questions .. Chapter 1
1. What version of the kernel that you are currently running?
(Hint: Output of ‘uname –a’; and, ‘cat /etc/\*-release’)

# Lab Assignment Questions .. Chapter 2
(Refer to Examples in Appendix E)
For #1, #2 and #3, turn in the output of the lines as displayed in /var/log/messages. If
you do not see any messages pertaining to this question in /var/log/messages, then let
me know. (We will explore further on the topic of console messages in Chapter 4).

1. Modules

  - Develop a module that takes an integer value as a parameter, when it is loaded using
insmod.

  - In the lab assignment for Chapter #1, you had used the output of ‘uname –r’, however,
for this lab assignment .. display your kernel version both in the string form, as well as
the internal notation using the macro’s UTS_RELEASE, LINUX_VERSION_CODE
and KERNEL_VERSION.

  - Load and unload the module, using insmod and rmmod, and modprobe. (Hint: for
modprobe, where do you put the modules?)

2. Stack of Modules

  - Develop a pair of modules. One of the pair uses a function defined in the other to print
a “Welcome” message.

  - Load and unload the modules, using insmod and rmmod, and modprobe. (Hint: for
modprobe, where do you put the modules?)

  - Show the output of lsmod when the module is loaded ..

3. Stack of Modules with Input parameters

  - Use the above Stack of Modules to print a text message passed in ..

  - You can turn in one assignment combining #2 and #3.

# Group Homework .. Chapter 3
1. Write a simple char device driver

  - Major Number should be passed in, as a parameter.
    If the passed-in parameter value is 0, then you should dynamically request a Major
number.

  - It should support open(), release(), read(), write() for each device – i.e. minor#.
    The device methods should be in a separate file.

  - It should correctly support device open flags (.. say) O_APPEND, O_WRONLY,
O_TRUNC, O_RDONLY .. ;

  - You’d need to support a minimum of two flags .. say O_APPEND and O_TRUNC.

  - Add a char device into /dev with the mknod command.

  - Write a program to read/write from this device, using UNIX System Calls ..
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

# Group Homework .. Chapter 6
1. Enhance your char device driver to implement

  a. Multiple Minor#’s.
    For this assignment question,<br/>
     Minor #1 .. /dev/CDD/CDD16 .. implements a 16-byte buffer<br/>
     Minor #2 .. /dev/CDD/CDD64 .. implements a 64-byte buffer<br/>
     Minor #3 .. /dev/CDD/CDD128 .. implements a 128-byte buffer<br/>
     Minor #4 .. /dev/CDD/CDD256 .. implements a 256-byte buffer<br/>

     Create /proc/CDD/CDD16 .. /proc/CDD/CDD256 entries .. similar to
Chapter #4 Question# 1.d,1.e.

  b. ioctl()
    For this assignment question .. on each Minor# CDD ..

    CMD1 .. print of number of open().<br/>
      Use a spinlock to protect the open() counter variable.<br/>
    CMD2 .. print “Buffer Length – Allocated” .. similar to Ch#4 Question# 1.d. ..<br/>
    CMD3 .. print “Buffer Length – Used” .. similar to Ch#4 Question# 1.e.<br/>

  c. llseek() functionality.
    For this assignment question .. on each Minor# CDD ..<br/>
    You do need to handle zero or negative offsets in llseek() , and plan for handling
SEEK_* flags e.g. SEEK_CUR, SEEK_END

    Test it out using an similar to the app you’ve written earlier in Chapter# 3.

  d. Implement a blocking open() .. using a mutex

  e. Implement support for poll() or select() functionality into your driver

  f. Extra-Credit:

    Implement pipe() .. using ioctl() , blocking reads and blocking writes.

or, implement your own system call mypipe(), that follows the makepipe or makefifo
semantics, and takes a filename (viz /dev/CDD ) as an argument.

# Homework .. Chapter 7
1. For Homework,
 Compute the CPU speed (and display in the /proc/CDD entry)

(Hint: the Timestamp counter is incremented at every CPU tick, and measure #clock ticks
across some #HZ)
- What to turn-in: module C program source and Makefile, Sample Output.

# Homework .. Chapter 8
1. In your char driver, allocate memory dynamically to an internal buffer in each of your
/dev/CDD/CDD* devices. Display the size of the allocated memory.

  - Implement the allocation of memory as part of the open method.
  What is the benefit?

  - Make sure to free the memory upon module exit.

  - What to turn-in: C program source and Makefile, Sample Output.

2. Extra-Credit:

  - Allocate Cache using kmem_cache_alloc().

  - Show the statistics from /proc/slabinfo

  - What to turn-in: C program source and Makefile, Sample Output.

# Group Homework .. Chapter 10
### Handling Sleep, Timers, Deferring Functions and Interrupts

Enhance the char device driver to
  a)  Create /proc/CDD/CDDmouse entry .. that will track and print

1. the current location of the mouse,

2. the #times each mouse button – e.g. left mouse button, or the right mouse button
was pressed .. and,

3. printk() which button was pressed.

Refer to ChangeLog for 2.6.20 and look for the string
INIT_WORK:

http://www.kernel.org/pub/linux/kernel/v2.6/

# Group Homework .. Chapter 12
## Add title


# Group Homework .. Chapter 13
## Add title


# Group Homework .. Chapter 15
## Add title


# Group Homework .. Chapter 16
## Add title


# Group Homework .. Chapter 17
## Add title


# Group Homework .. Chapter 11 (Extra credit)
### Data Types in the Kernel

Enhance the char device driver to
  a) Create a /proc/CDD/CDDsysteminfo entry .. that will print

1. the CPU endianness

  (Hint: You can use the cpu_to_be* or the cpu_to_le* calls to determine
endianness of the CPU)

2. the memory Page Size

3. the size of
  - u8, u16, u32, u64,
  - s8, s16, s32, s64,
  - int8_t, int16_t, int32_t, int64_t
datatypes, and

4. whether char is signed or unsigned.
