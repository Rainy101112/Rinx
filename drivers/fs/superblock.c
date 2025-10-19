/*
 *
 *      superblock.c
 *      Superblock manager
 *
 *      2025/10/8 By Rainy101112
 *      Based on GPL-3.0 open source agreement
 *      Rinx Kernel project.
 *
 */

#include "fs/superblock.h"
#include "alloc.h"
#include "printk.h"
#include "spin_lock.h"
#include "string.h"

size_t superblocks_length = 0; // Current length of array superblocks
sb_t  *superblocks;            // All the superblocks

spinlock_t superblock_lock = {
    .lock   = 0, // Unlock
    .rflags = 0  // Initialized sign
};

sb_t sb_dummy = { // Dummy superblock
    .type            = SUPERBLOCK_NULL,
    .sector_size     = 0,
    .block_size      = 0,
    .superblock_size = 0,
    .total_blocks    = 0,
    .free_blocks     = 0,

    .device_id   = 0,
    .device_name = "null",

    .magic    = 0xdeadbeef,
    .version  = 0,
    .features = 0,

    .operations = {0},

    .private_data = (void *)NULL};

int superblock_init()
{
    superblocks = (sb_t *)malloc(sizeof(sb_t) * DEFAULT_SUPERBLOCK_COUNT);

    if (superblocks == NULL) {
        plogk("vfs: Failed to initialize superblock manager.\n");
        return 0;
    }

    superblocks_length = DEFAULT_SUPERBLOCK_COUNT;

    for (size_t i = 0; i < superblocks_length; i++) { // Fill in dummies
        superblocks[i] = sb_dummy;
    }

    plogk("vfs: Superblock manager successfully.\n");

    return 1;
}

int superblock_register(sb_t sb)
{
    spin_lock(&superblock_lock);

    for (size_t i = 0; i < superblocks_length; i++) {
        if (superblocks[i].type == SUPERBLOCK_NULL) {
            superblocks[i] = sb;
            spin_unlock(&superblock_lock);
            plogk("sb: Register superblock type=%d ID=%u name=%s\n", sb.type, sb.device_id, sb.device_name);
            return 1;
        }
    }

    size_t new_length = superblocks_length * 2;
    sb_t  *new_blocks = realloc(superblocks, sizeof(sb_t) * new_length);

    if (!new_blocks) {
        spin_unlock(&superblock_lock);
        plogk("sb: Register superblock failed type=%d ID=%u Name=%s\n", sb.type, sb.device_id, sb.device_name);

        return 0;
    }

    superblocks = new_blocks;

    for (size_t i = superblocks_length; i < new_length; i++) { superblocks[i] = sb_dummy; }
    superblocks_length = new_length;

    superblocks[superblocks_length / 2] = sb;

    spin_unlock(&superblock_lock);
    plogk("sb: Register superblock type=%d ID=%u name=%s\n", sb.type, sb.device_id, sb.device_name);

    return 1;
}

int superblock_unregister(sb_t sb)
{
    spin_lock(&superblock_lock);

    for (size_t i = 0; i < superblocks_length; i++) {
        if (superblocks[i].device_id == sb.device_id && strcmp(superblocks[i].device_name, sb.device_name) == 0) {
            superblocks[i] = sb_dummy;

            spin_unlock(&superblock_lock);
            plogk("sb: Unregister superblock type=%d ID=%u name=%s\n", sb.type, sb.device_id, sb.device_name);

            return 1;
        }
    }

    plogk("sb: Unregister superblock failed type=%d ID=%u name=%s\n", sb.type, sb.device_id, sb.device_name);

    spin_unlock(&superblock_lock);
    return 0;
}

sb_result_t superblock_read(sb_t sb, void *data, size_t size, size_t offset)
{
    if (!data || size == 0 || !sb.operations.read) return SB_ERROR_INVALID_PARAM;

    if (!superblock_is_ready(sb)) { return SB_ERROR_IO; }

    return sb.operations.read(data, size, offset);
}

sb_result_t superblock_write(sb_t sb, const void *data, size_t size, size_t offset)
{
    if (!data || size == 0 || !sb.operations.write) return SB_ERROR_INVALID_PARAM;

    if (!superblock_is_ready(sb)) return SB_ERROR_IO;

    return sb.operations.write(data, size, offset);
}

sb_result_t superblock_flush(sb_t sb)
{
    if (!sb.operations.flush) return SB_ERROR_INVALID_PARAM;

    if (!superblock_is_ready(sb)) return SB_ERROR_IO;

    return sb.operations.flush();
}

sb_result_t superblock_ioctl(sb_t sb, uint32_t cmd, void *arg)
{
    if (!sb.operations.ioctl) return SB_ERROR_INVALID_PARAM;

    if (!superblock_is_ready(sb)) return SB_ERROR_IO;

    return sb.operations.ioctl(cmd, arg);
}

int superblock_is_ready(const sb_t sb)
{
    if (!sb.operations.is_ready) return 0;

    return sb.operations.is_ready();
}

sb_t superblock_find_id(const uint32_t id)
{
    spin_lock(&superblock_lock);

    for (size_t i = 0; i < superblocks_length; i++) {
        if (superblocks[i].device_id == id) {
            spin_unlock(&superblock_lock);
            return superblocks[i];
        }
    }

    spin_unlock(&superblock_lock);
    return sb_dummy;
}

sb_t superblock_find_name(const char *name)
{
    spin_lock(&superblock_lock);

    for (size_t i = 0; i < superblocks_length; i++) {
        if (strcmp(superblocks[i].device_name, name) == 0) {
            spin_unlock(&superblock_lock);
            return superblocks[i];
        }
    }

    spin_unlock(&superblock_lock);
    return sb_dummy;
}

size_t superblock_count()
{
    spin_lock(&superblock_lock);

    size_t count = 0;

    for (size_t i = 0; i < superblocks_length; i++) {
        if (superblocks[i].type != SUPERBLOCK_NULL) count++;
    }

    spin_unlock(&superblock_lock);
    return count;
}

int superblock_foreach(superblock_iter_cb callback, void *arg)
{
    spin_lock(&superblock_lock);

    if (!callback) {
        spin_unlock(&superblock_lock);
        return 0;
    }

    for (size_t i = 0; i < superblocks_length; i++) {
        if (superblocks[i].type != SUPERBLOCK_NULL) {
            if (!callback(&superblocks[i], arg)) break;
        }
    }

    spin_unlock(&superblock_lock);
    return 1;
}
