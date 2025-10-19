#ifndef INCLUDE_DENTRY_H_
#define INCLUDE_DENTRY_H_

#include "double_list.h"
#include "inode.h"

struct dentry
{
	char name[256];			// File name
	inode_t *inode;			// Point to the inode of this dentry
	struct dentry *parent;	// Parent dentry
	ilist_node_t *list;		// Directory list
	ilist_node_t *child;	// Subdirectory list
};

typedef struct dentry dentry_t;

#endif // INCLUDE_DENTRY_H_
