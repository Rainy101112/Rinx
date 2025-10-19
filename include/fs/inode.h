#ifndef INCLUDE_INODE_H_
#define INCLUDE_INODE_H_

#include "singly_list.h"
#include "stdint.h"
#include "superblock.h"
#include "dentry.h"

struct inode
{
	uint32_t mode;		// Inode mode
	uint32_t uid;		// User UID
	uint32_t gid;		// Group GID
	sb_t sb;			// The superblock where the inode is located
	dentry_t *dentry;	// The dentry where related.
	struct inode parent;// Parent inode
	uint64_t bytes;		// Size (bytes)
	uint64_t block;		// Block count
	uint64_t atime;		// Access time
	uint64_t mtime;		// Modify time
	uint64_t ctime;		// Create time
	uint32_t links;		// Hard link count
	void *data;			// Inode data
	slist_t list;
};

typedef struct inode inode_t;

#endif // INCLUDE_INODE_H_
