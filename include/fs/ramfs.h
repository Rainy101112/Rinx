#ifndef INCLUDE_RAMFS_H_
#define INCLUDE_RAMFS_H_

#include "fs/superblock.h"
#include "fs/inode.h"
#include "fs/dentry.h"

#include "stdint.h"
#include "stddef.h"

#define RAMFS_MAGIC 0x72616D66		// "ramf"

sb_t ramfs_init(void);

inode_t *ramfs_inode_create(sb_t sb, uint32_t umode);

int ramfs_create(sb_t sb, const char *name, inode_t *dir);
int ramfs_remove(sb_t sb, inode_t *inode);

long ramfs_write(inode_t *inode, const void *data, size_t count, long *offset);
long ramfs_read(inode_t *inode, void *data, size_t count, long *offset);

int ramfs_mkdir(sb_t sb, const char *name, inode_t *parent);
int ramfs_rmdir(sb_t sb, inode_t *inode);

int ramfs_mount(sb_t sb, const char *path, int flags);
int ramfs_umount(sb_t sb);

int ramfs_rename(sb_t sb, dentry_t *dentry, const char *name);
dentry_t *ramfs_lookup(sb_t sb, inode_t *dir, const char *name);

#endif // INCLUDE_RAMFS_H_
