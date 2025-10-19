#ifndef INCLUDE_VFS_H_
#define INCLUDE_VFS_H_

#include "fs/inode.h"
#include "fs/dentry.h"

#include "stdint.h"
#include "stddef.h"
#include "stdlib.h"

struct fs_operations
{
	int (*mount)(const char *path, int flags);
	int (*umount)(void);

	int (*create)(const char *path, uint32_t stat_type, uint32_t umode);
	int (*remove)(const char *path);

	long (*read)(const char *path, void *data, size_t size, long offset);
	long (*write)(const char *path, const void *data, size_t size, long offset);

	int (*mkdir)(const char *path, uint32_t umode);
	int (*rmdir)(const char *path);

	int (*rename)(const char *path, const char *name);

	dentry_t *(*lookup)(inode_t *dir, const char *name);
};

typedef struct fs_operations fs_op_t;

struct fs_type
{
	char name[32];		// Filesystem name
	int flags;			// Filesystem flags
	sb_t sb;			// The superblock where the filesystem is located
	fs_op_t op;			// Filesystem operations
};

typedef struct fs_type fs_t;

int register_filesystem(fs_t fs);
int unregister_filesystem(fs_t fs);

int vfs_open(fs_t fs);
int vfs_close(fs_t fs);
dentry_t *vfs_mount(fs_t fs, const char *path, int flags);
int vfs_umount(fs_t fs);
int vfs_creat(fs_t fs, const char *path, uint32_t stat_type, uint32_t umode);
int vfs_remove(fs_t fs, const char *path);
int vfs_read(fs_t fs, const char *path, void *data, size_t size, long offset);
int vfs_write(fs_t fs, const char *path, const void *data, size_t size, long offset);
int vfs_mkdir(fs_t fs, const char *path, uint32_t umode);
int vfs_rmdir(fs_t fs, const char *path);
int vfs_rename(fs_t fs, const char *path, const char *name);

#endif // INCLUDE_VFS_H_
