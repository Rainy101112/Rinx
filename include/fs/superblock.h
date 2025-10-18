/*
 *
 *      superblock.h
 *      Superblock manager
 *
 *      2025/10/8 By Rainy101112
 *      Based on GPL-3.0 open source agreement
 *      Rinx Kernel project.
 *
 */

#ifndef INCLUDE_SUPERBLOCK_H_
#define INCLUDE_SUPERBLOCK_H_

#include "stddef.h"
#include "stdint.h"

#define DEFAULT_SUPERBLOCK_COUNT 26

/* Superblock types */
typedef enum superblock_types {
    SUPERBLOCK_IDE = 0,  // IDE drive
    SUPERBLOCK_AHCI,     // AHCI SATA drive
    SUPERBLOCK_NETDRIVE, // Network drive
    SUPERBLOCK_NULL,     // NULL
} sb_type_t;

/* Operation results */
typedef enum superblock_result {
    SB_SUCCESS = 0,         // Success
    SB_ERROR_IO,            // I/O error
    SB_ERROR_INVALID_PARAM, // Invaild parameter
    SB_ERROR_NO_SPACE,      // No enough space
    SB_ERROR_UNSUPPORT,     // Unsupport operation
    SB_ERROR_CORRUPTED,     // Data corrupted
} sb_result_t;

/* Superblock operations structure */
typedef struct superblock_operations {
        sb_result_t (*read)(uint8_t *data, size_t size, size_t offset);
        sb_result_t (*write)(const uint8_t *data, size_t size, size_t offset);
        sb_result_t (*flush)(void);
        sb_result_t (*ioctl)(uint32_t cmd, void *arg);

        int (*is_ready)(void);
} sb_op_t;

/* Superblock structure */
typedef struct superblock {
        sb_type_t type; // Device type

        /* Size information */
        size_t sector_size;     // Sector size (byte)
        size_t block_size;      // Block size (byte)
        size_t superblock_size; // Superblock size (byte)
        size_t total_blocks;    // Total blocks
        size_t free_blocks;     // Total free blocks

        /* Device sign */
        uint32_t device_id;       // Device ID
        char     device_name[32]; // Device name

        /* Filesystem information */
        uint32_t magic;    // Magic sign
        uint32_t version;  // Version
        uint32_t features; // Supported features

        /* Superblock operations */
        sb_op_t operations;

        /* Private data pointer */
        void *private_data;
} sb_t;

typedef int (*superblock_iter_cb)(sb_t *sb, void *arg);

/**
 * @brief Initialize superblock manager
 * @return Operation result
 */
int superblock_init(void);

int superblock_register(sb_t sb);

int superblock_unregister(sb_t sb);

/**
 * @brief Read data from a superblock
 * @param sb Target structure
 * @param data Data buffer
 * @param size Data size
 * @param offset Offset
 * @return Result
 */
sb_result_t superblock_read(sb_t sb, uint8_t *data, size_t size, size_t offset);

/**
 * @brief Write data to a superblock
 * @param sb Target structure
 * @param data Data buffer
 * @param size Data size
 * @param offset Offset
 * @return Result
 */
sb_result_t superblock_write(sb_t sb, const uint8_t *data, size_t size, size_t offset);

/**
 * @brief Flush a superblock
 * @param sb Target structure
 * @return Result
 */
sb_result_t superblock_flush(sb_t sb);

/**
 * @brief Do IOCTL to a superblock
 * @param sb Target structure
 * @param cmd Command
 * @param arg Arguments
 * @return Result
 */
sb_result_t superblock_ioctl(sb_t sb, uint32_t cmd, void *arg);

/**
 * @brief Check if a superblock device is ready
 * @param sb Target structure
 * @return 1 Ready, 0 Not ready
 */
int superblock_is_ready(const sb_t sb);

/* Find a superblock device by ID */
sb_t superblock_find_id(const uint32_t id);

/* Find a superblock device by name */
sb_t superblock_find_name(const char *name);

/* Get current superblock count */
size_t superblock_count(void);

/* Foreach all the superblock devices */
int superblock_foreach(superblock_iter_cb callback, void *arg);

#endif // INCLUDE_SUPERBLOCK_H_
