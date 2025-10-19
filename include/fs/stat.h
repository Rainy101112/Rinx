#ifndef INCLUDE_STAT_H_
#define INCLUDE_STAT_H_

#include "stdint.h"

#define S_IFMT   0170000	// Filesystem bit mask
#define S_IFSOCK 0140000	// Socket
#define S_IFLNK  0120000	// Symbolic link
#define S_IFREG  0100000	// Normal file
#define S_IFBLK  0060000	// Block device
#define S_IFDIR  0040000	// Directory
#define S_IFCHR  0020000	// Character device
#define S_IFIFO  0010000	// FIFO

#define S_ISUID 04000	// Set user-id on execution
#define S_ISGID 02000	// Set group-id on execution
#define S_ISVRX 01000	// Sticky

#define S_IRGRP 00040	// User group is able to read
#define S_IWGRP 00020	// User group is able to write
#define S_IXGRP 00010	// User group is able to execute

#define S_IROTH 00004	// Other users are able to read
#define S_IWOTH 00002	// Other users are able to write
#define S_IXOTH 00001	// Other users are able to execute

enum STAT_RESULTS {
	/* Result is 0 when stat successfully */
	EBADF = 1,		// Invaild file discription
	EFAULT,			// Unable to access the address space
	ELOOP,			// Too many symbolic links when foreach the paths
	ENAMETOOLONG,	// File name/path too long
	ENOENT,			// No such file or directory
	ENOMEM,			// No enough memory
	ENOTDIR,		// Not a directory
	EEXIST,			// File already exist
	EBUSY,			// Device or resource busy
	ENOTEMPTY,		// Directory not empty
	EINVAL,			// Invaild argument
};

int S_ISDIR(uint32_t mode);

#endif // INCLUDE_STAT_H_
