#include "fs/vfs.h"
#include "stdlib.h"
#include "stddef.h"
#include "stdint.h"

int vfs_open(fs_t fs){
	return ;
}

int vfs_close(fs_t fs){
	return ;
}

dentry_t *vfs_mount(fs_t fs, const char *path, int flags){
	return fs.op.mount(path, flags);
}

int vfs_umount(fs_t fs){
	return fs.op.umount();
}

int vfs_creat(fs_t fs, const char *path, uint32_t stat_type, uint32_t umode){
	return fs.op.create(path, stat_type, umode);
}

int vfs_remove(fs_t fs, const char *path){
	return fs.op.remove(path);
}

int vfs_read(fs_t fs, const char *path, void *data, size_t size, long offset){
	return fs.op.read(path, data, size, offset);
}

int vfs_write(fs_t fs, const char *path, const void *data, size_t size, long offset){
	return fs.op.write(path, data, size, offset);
}

int vfs_mkdir(fs_t fs, const char *path, uint32_t umode){
	return fs.op.mkdir(path, umode);
}

int vfs_rmdir(fs_t fs, const char *path){
	return fs.op.rmdir(path);
}

int vfs_rename(fs_t fs, const char *path, const char *name){
	return fs.op.rename(path, name);
}

