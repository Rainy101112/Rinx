#include "fs/ramfs.h"
#include "fs/stat.h"
#include "fs/vfs.h"
#include "fs/inode.h"
#include "fs/dentry.h"

#include "heap.h"
#include "alloc.h"
#include "string.h"
#include "time.h"
#include "singly_list.h"
#include "spin_lock.h"

sb_t ramfs_init(void){
	size_t total_blocks = KERNEL_HEAP_SIZE / BLOCK_SIZE_4K;

	sb_op_t ramfs_op = {
		.read = NULL,
		.write = NULL,
		.flush = NULL,
		.ioctl = NULL,
		.is_ready = NULL
	};

	slist_t list;
	slist_init(&list);

	sb_t ramfs_sb = {
		.type = SUPERBLOCK_RAM,
		.sector_size = BLOCK_SIZE_4K,
		.block_size = BLOCK_SIZE_4K,
		.total_blocks = total_blocks,
		.free_blocks = total_blocks,		// Using 0 blocks on initializing.
		.device_id = 0x2000,
		.device_name = "ramfs",
		.magic = RAMFS_MAGIC,
		.version = 0x1,
		.features = (1 << 0),
		.operations = ramfs_op,
		.inode_count = 0,
		.inode_list = list,
		.private_data = NULL
	};

	superblock_register(ramfs_sb);

	return ramfs_sb;
}

inode_t *ramfs_inode_create(sb_t sb, uint32_t umode){
	inode_t *inode = (inode_t *)malloc(sizeof(inode_t));
	if (!inode) return NULL;

	memset((void *)inode, 0, sizeof(inode_t));
	inode->mode = umode;
	inode->uid = 0;
	inode->gid = 0;
	inode->sb = sb;
	inode->bytes = 0;
	inode->block = 0;
	inode->atime = inode->mtime = inode->ctime = get_current_time();
	inode->links = 1;
	inode->data = NULL;

	slist_insert_tail(&inode->list, &sb.inode_list);

	return inode;
}

int ramfs_create(sb_t sb, const char *name, inode_t *dir){
	/* Check if the name is already exist */
    if (ramfs_lookup(sb, name, dir)) {
        return -EEXIST;
    }
    
    /* Create new inode */
    inode_t *inode = ramfs_create_inode(sb, S_IFREG | 0644);
    if (!inode) return -ENOMEM;
    
    /* Create dentry */
	dentry_t *dentry = malloc(sizeof(dentry_t));
    if (!dentry) {
        free(inode);
        return -ENOMEM;
    }

    strncpy(dentry->name, name, sizeof(dentry->name)-1);
    dentry->inode = inode;

    slist_insert_tail(&dentry->child, (struct list_head*)dir->data);
    
    return 0;
}

int ramfs_remove(sb_t sb, inode_t *inode);

long ramfs_write(inode_t *inode, const void *data, size_t count, long *offset){
	size_t new_size = *offset + count;
    
    if (new_size > inode->bytes) {
        void *new_data = realloc(inode->data, new_size);
        if (!new_data) return -ENOMEM;
        inode->data = new_data;
        inode->bytes = new_size;
    }

    memcpy((char*)inode->data + *offset, data, count);
    *offset += count;
    inode->mtime = get_current_time();
    
    return count;
}

long ramfs_read(inode_t *inode, void *data, size_t count, long *offset){
	if (*offset >= inode->bytes) return 0;
    
    size_t remaining = inode->bytes - *offset;
    size_t to_read = count < remaining ? count : remaining;
    
    memcpy(data, (char*)inode->data + *offset, to_read);
    *offset += to_read;
    inode->atime = get_current_time();
    
    return to_read;
}

int ramfs_mkdir(sb_t sb, const char *name, inode_t *parent){
	inode_t *dir_inode = ramfs_create_inode(sb, S_IFDIR | 0755);
    if (!dir_inode) return -ENOMEM;
    
    // 初始化目录内容链表
    dir_inode->data = malloc(sizeof(slist_t));
    slist_init((slist_t *)dir_inode->data);
    
    // 创建目录项
    dentry_t *dentry = malloc(sizeof(dentry_t));
    strncpy(dentry->name, name, sizeof(dentry->name)-1);
    dentry->inode = dir_inode;
    
    // 添加到父目录
    slist_insert_tail(&dentry->child, (struct list_head*)parent->data);
    
    // 创建 . 和 .. 条目
    ramfs_add_dot_entries(dir_inode, parent);
    
    return 0;
}

void ramfs_add_dot_entries(inode_t *dir, inode_t *parent) {
    /* Add "." dentry */
    dentry_t *dot = malloc(sizeof(dentry_t));
    strcpy(dot->name, ".");
    dot->inode = dir;
    list_add(&dot->child, (struct list_head* )dir->data);
    
    /* Add ".." dentry */
    dentry_t *dotdot = malloc(sizeof(dentry_t));
    strcpy(dotdot->name, "..");
    dotdot->inode = parent;
    slist_insert_tail(&dotdot->child, (struct list_head*)dir->data);
}

int ramfs_rmdir(sb_t *sb, inode_t *dir) {
    if (!sb || !dir) return -EINVAL;
    
    // 检查是否是目录
    if (!S_ISDIR(dir->mode)) return -ENOTDIR;
    
    // 获取目录项链表
    struct list_head *dir_list = (struct list_head *)dir->data;
    if (!dir_list) return -EINVAL;
    
    // 检查目录是否为空（除了 . 和 ..）
    int entry_count = 0;
    dentry_t *dentry;
    list_for_each_entry(dentry, dir_list, dentry->child) {
        entry_count++;
    }
    
    // 如果目录不为空（超过2个条目：. 和 ..），返回错误
    if (entry_count > 2) return -ENOTEMPTY;
    
    // 从父目录中移除当前目录的目录项
    dentry_t *parent_dentry = NULL;
    slist_t *parent_list = (slist_t *)dir->parent->data;
    
    list_for_each_entry(dentry, parent_list, dentry->child) {
        if (dentry->inode == dir) {
            parent_dentry = dentry;
            break;
        }
    }
    
    if (parent_dentry) {
        slist_remove(&parent_dentry->child);
        free(parent_dentry);
    }
    
    // 释放目录内容
    if (dir_list) {
        // 释放 . 和 .. 条目
        struct ramfs_dentry *dentry, *tmp;
        list_for_each_entry_safe(dentry, tmp, dir_list, d_child) {
            list_del(&dentry->child);
            kfree(dentry);
        }
        kfree(dir_list);
    }
    
    // 减少链接计数
    dir->links--;
    if (dir->links == 0) {
        // 如果没有其他链接，释放inode
        slist_remove(&dir->list);
        free(dir);
    }
    
    return 0;
}

// 挂载点结构
struct ramfs_mount {
    slist_t m_list;
    char m_path[256];
    sb_t *m_sb;
    inode_t *m_root;
};

struct ramfs_mount mount_point;

slist_t mount_list;
spinlock_t mount_lock;

int ramfs_mount(sb_t *sb, const char *path, int flags) {
    if (!sb || !path) return -EINVAL;
    
    // 检查路径是否已挂载
    spin_lock(&mount_lock);
    struct ramfs_mount *mount;
    list_for_each_entry(mount, &mount_list, mount_point.m_list) {
        if (strcmp(mount->m_path, path) == 0) {
            spin_unlock(&mount_lock);
            return -EBUSY; // 挂载点已被占用
        }
    }
    spin_unlock(&mount_lock);
    
    // 创建挂载点
    mount = malloc(sizeof(struct ramfs_mount));
    if (!mount) return -ENOMEM;
    
    strncpy(mount->m_path, path, sizeof(mount->m_path) - 1);
    mount->m_sb = sb;
    mount->m_root = sb->root_inode;
    
    // 添加到挂载列表
    spin_lock(&mount_lock);
    slist_insert_tail(&mount->m_list, &mount_list);
    spin_unlock(&mount_lock);
    
    printk("RAMFS mounted at %s\n", path);
    return 0;
}

int ramfs_umount(sb_t sb) {
    if (!&sb) return -EINVAL;
    
    spin_lock(&mount_lock);
    
    // 查找对应的挂载点
    struct ramfs_mount *mount, *tmp;
    list_for_each_entry_safe(mount, tmp, &mount_list, m_list) {
        if (mount->m_sb == sb) {
            // 检查挂载点是否正在使用
            if (mount->m_root->i_links > 1) { // 除了挂载点自身的引用
                spin_unlock(&mount_lock);
                return -EBUSY;
            }
            
            // 从挂载列表移除
            list_remove(&mount->m_list);
            free(mount);
            
            spin_unlock(&mount_lock);
            printk("RAMFS unmounted\n");
            return 0;
        }
    }
    
    spin_unlock(&mount_lock);
    return -EINVAL; // 未找到挂载点
}

int ramfs_umount(sb_t sb);

int ramfs_rename(sb_t sb, dentry_t *dentry, const char *name);
dentry_t *ramfs_lookup(sb_t sb, inode_t *dir, const char *name);
