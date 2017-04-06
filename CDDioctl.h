#ifndef CDDIOCTL_H
#define CDDIOCTL_H

/*
* NOTE: The macros for defining ioctl commands come from <linux/ioctl.h> in kernel space.
*   When calling from user space, use <sys/ioctl.h>.
*   The appropriate header must be included before this file.
*/

#define CDDCMD_DEVSIZE (0)
#define CDDCMD_DEVUSED (1)
#define CDDCMD_DEVOPENS (2)

// Ioctl commands
// NOTE: Since the user provides the result buffer, we us _IOR
#define CDDMAGIC  	(242)  // this 8-bit # must be unique across the systemm for our ioctl commands
#define CDDIO_DEVSIZE  _IOR(CDDMAGIC, CDDCMD_DEVSIZE, char*)
#define CDDIO_DEVUSED  _IOR(CDDMAGIC, CDDCMD_DEVUSED, char*)
#define CDDIO_DEVOPENS  _IOR(CDDMAGIC, CDDCMD_DEVOPENS, char*)

#endif
