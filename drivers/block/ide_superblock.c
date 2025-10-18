#include "fs/sb_devices.h"
#include "alloc.h"
#include "fs/superblock.h"
#include "ide.h"
#include "printk.h"
#include "spin_lock.h"
#include "stdlib.h"
#include "string.h"

extern ide_device_t ide_devices[4];
extern int          package[2];

/* IDE superblocks */
static sb_t ide_superblocks[4] = {0};
static int  ide_sb_initialized = 0;

/* Private data of each IDE driver */
typedef struct {
        uint8_t  drive_num;     // IDE drive number (0-3)
        uint32_t sector_size;   /* Size of sector */
        uint32_t total_sectors; /* Total sector counts */
        int      is_atapi;      /* Whether the drive is ATAPI drive or not */
        char     model[41];     /* Drive model */
} ide_private_data_t;

static sb_result_t ide_sb_read_impl(uint8_t drive_num, uint8_t *data, size_t size, size_t offset);
static sb_result_t ide_sb_write_impl(uint8_t drive_num, const uint8_t *data, size_t size, size_t offset);

static sb_result_t ide_sb_read_0(uint8_t *data, size_t size, size_t offset)
{
    return ide_sb_read_impl(0, data, size, offset);
}
static sb_result_t ide_sb_read_1(uint8_t *data, size_t size, size_t offset)
{
    return ide_sb_read_impl(1, data, size, offset);
}
static sb_result_t ide_sb_read_2(uint8_t *data, size_t size, size_t offset)
{
    return ide_sb_read_impl(2, data, size, offset);
}
static sb_result_t ide_sb_read_3(uint8_t *data, size_t size, size_t offset)
{
    return ide_sb_read_impl(3, data, size, offset);
}

static sb_result_t ide_sb_write_0(const uint8_t *data, size_t size, size_t offset)
{
    return ide_sb_write_impl(0, data, size, offset);
}
static sb_result_t ide_sb_write_1(const uint8_t *data, size_t size, size_t offset)
{
    return ide_sb_write_impl(1, data, size, offset);
}
static sb_result_t ide_sb_write_2(const uint8_t *data, size_t size, size_t offset)
{
    return ide_sb_write_impl(2, data, size, offset);
}
static sb_result_t ide_sb_write_3(const uint8_t *data, size_t size, size_t offset)
{
    return ide_sb_write_impl(3, data, size, offset);
}

static int ide_sb_is_ready_0(void)
{
    return ide_devices[0].reserved;
}
static int ide_sb_is_ready_1(void)
{
    return ide_devices[1].reserved;
}
static int ide_sb_is_ready_2(void)
{
    return ide_devices[2].reserved;
}
static int ide_sb_is_ready_3(void)
{
    return ide_devices[3].reserved;
}

/* Common flush */
static sb_result_t ide_sb_flush_common(void)
{
    return SB_SUCCESS;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static sb_result_t ide_sb_ioctl_common(uint32_t cmd, void *arg)
{
    switch (cmd) {
        case 0x1000 : /* Get device information */
            return SB_SUCCESS;
        case 0x1001 : /* Get device capacity */
            return SB_SUCCESS;
        default :
            return SB_ERROR_UNSUPPORT;
    }
}
#pragma GCC diagnostic pop

/* Operations */
static sb_op_t ide_sb_operations[4] = {
    {.read = ide_sb_read_0, .write = ide_sb_write_0, .flush = ide_sb_flush_common, .ioctl = ide_sb_ioctl_common, .is_ready = ide_sb_is_ready_0},
    {.read = ide_sb_read_1, .write = ide_sb_write_1, .flush = ide_sb_flush_common, .ioctl = ide_sb_ioctl_common, .is_ready = ide_sb_is_ready_1},
    {.read = ide_sb_read_2, .write = ide_sb_write_2, .flush = ide_sb_flush_common, .ioctl = ide_sb_ioctl_common, .is_ready = ide_sb_is_ready_2},
    {.read = ide_sb_read_3, .write = ide_sb_write_3, .flush = ide_sb_flush_common, .ioctl = ide_sb_ioctl_common, .is_ready = ide_sb_is_ready_3}
};

/* Common read */
static sb_result_t ide_sb_read_impl(uint8_t drive_num, uint8_t *data, size_t size, size_t offset)
{
    /* Check parameter */
    if (!data || size == 0) { return SB_ERROR_INVALID_PARAM; }

    /* Check if the device is exist */
    if (!ide_devices[drive_num].reserved) { return SB_ERROR_IO; }

    ide_private_data_t *priv = (ide_private_data_t *)ide_superblocks[drive_num].private_data;
    if (!priv) { return SB_ERROR_INVALID_PARAM; }

    /* Sector information */
    uint32_t sector_size     = priv->sector_size;
    uint32_t start_sector    = offset / sector_size;
    uint32_t sector_offset   = offset % sector_size;
    uint32_t sectors_to_read = (size + sector_offset + sector_size - 1) / sector_size;

    /* Check space */
    if (start_sector + sectors_to_read > priv->total_sectors) { return SB_ERROR_NO_SPACE; }

    /* Read data */
    uint16_t *sector_buffer = (uint16_t *)malloc(sectors_to_read * sector_size);
    if (!sector_buffer) { return SB_ERROR_NO_SPACE; }

    /* Read sectors */
    ide_read_sectors(drive_num, sectors_to_read, start_sector, sector_buffer);

    /* Check result */
    if (package[0] != 0) {
        free(sector_buffer);
        return SB_ERROR_IO;
    }

    /* Copy request data to output buffer */
    size_t bytes_copied = 0;
    for (uint32_t i = 0; i < sectors_to_read && bytes_copied < size; i++) {
        uint8_t *sector_start = (uint8_t *)(sector_buffer + i * (sector_size / 2));
        size_t   copy_size    = sector_size - sector_offset;
        if (copy_size > size - bytes_copied) { copy_size = size - bytes_copied; }

        memcpy(data + bytes_copied, sector_start + sector_offset, copy_size);
        bytes_copied += copy_size;
        sector_offset = 0;
    }

    free(sector_buffer);
    return SB_SUCCESS;
}

static sb_result_t ide_sb_write_impl(uint8_t drive_num, const uint8_t *data, size_t size, size_t offset)
{
    /* Check parameter */
    if (!data || size == 0) { return SB_ERROR_INVALID_PARAM; }

    /* Check if the device is exist */
    if (!ide_devices[drive_num].reserved) { return SB_ERROR_IO; }

    ide_private_data_t *priv = (ide_private_data_t *)ide_superblocks[drive_num].private_data;
    if (!priv) { return SB_ERROR_INVALID_PARAM; }

    /* ATAPI device does not support writing */
    if (priv->is_atapi) { return SB_ERROR_UNSUPPORT; }

    /* Sector information */
    uint32_t sector_size      = priv->sector_size;
    uint32_t start_sector     = offset / sector_size;
    uint32_t sector_offset    = offset % sector_size;
    uint32_t sectors_to_write = (size + sector_offset + sector_size - 1) / sector_size;

    if (start_sector + sectors_to_write > priv->total_sectors) { return SB_ERROR_NO_SPACE; }

    if (sector_offset > 0 || size < sector_size) {
        uint16_t *sector_buffer = (uint16_t *)malloc(sectors_to_write * sector_size);
        if (!sector_buffer) { return SB_ERROR_NO_SPACE; }

        /* Read */
        ide_read_sectors(drive_num, sectors_to_write, start_sector, sector_buffer);
        if (package[0] != 0) {
            free(sector_buffer);
            return SB_ERROR_IO;
        }

        /* Process data */
        size_t bytes_copied = 0;
        for (uint32_t i = 0; i < sectors_to_write && bytes_copied < size; i++) {
            uint8_t *sector_start = (uint8_t *)(sector_buffer + i * (sector_size / 2));
            size_t   copy_size    = sector_size - sector_offset;
            if (copy_size > size - bytes_copied) { copy_size = size - bytes_copied; }

            memcpy(sector_start + sector_offset, data + bytes_copied, copy_size);
            bytes_copied += copy_size;
            sector_offset = 0;
        }

        /* Write back */
        ide_write_sectors(drive_num, sectors_to_write, start_sector, sector_buffer);
        free(sector_buffer);
    } else {
        /* Write directly */
        ide_write_sectors(drive_num, sectors_to_write, start_sector, (uint16_t *)data);
    }

    /* Check result */
    if (package[0] != 0) { return SB_ERROR_IO; }

    return SB_SUCCESS;
}

/* Initialize IDE superblock */
void ide_superblock_init(void)
{
    if (ide_sb_initialized) { return; }

    /* Forreach */
    for (int i = 0; i < 4; i++) {
        if (ide_devices[i].reserved) {
            ide_private_data_t *priv_data = (ide_private_data_t *)malloc(sizeof(ide_private_data_t));
            if (!priv_data) {
                plogk("ide_sb: Failed to allocate private data for drive %d\n", i);
                continue;
            }

            /* Private data */
            priv_data->drive_num     = i;
            priv_data->sector_size   = 512; // Size of sector
            priv_data->total_sectors = ide_devices[i].size;
            priv_data->is_atapi      = (ide_devices[i].type == IDE_ATAPI);
            strncpy(priv_data->model, (char *)ide_devices[i].model, 40);
            priv_data->model[40] = '\0';

            /* Create superblock */
            sb_t sb = {.type            = SUPERBLOCK_IDE,
                       .sector_size     = 512,
                       .block_size      = 4096, // 4KB block
                       .superblock_size = 1024, // 1KB superblock
                       .total_blocks    = ide_devices[i].size * 512 / 4096,
                       .free_blocks     = ide_devices[i].size * 512 / 4096, // All the blocks are free when initialize

                       .device_id   = 0x1000 + i, // IDE device ID scope
                       .device_name = {0},

                       .magic    = 0xDEADBEEF,
                       .version  = 1,
                       .features = priv_data->is_atapi ? 0 : (1 << 0), // ATA device supports writing

                       .operations   = ide_sb_operations[i],
                       .private_data = priv_data};

            /* Set device name */
            snprintf(sb.device_name, sizeof(sb.device_name), "ide%d", i);

            /* Register to superblock manager */
            if (superblock_register(sb)) {
                ide_superblocks[i] = sb;
                plogk("ide_sb: Registered IDE device %d as superblock: %s\n", i, sb.device_name);
            } else {
                plogk("ide_sb: Failed to register IDE device %d\n", i);
                free(priv_data);
            }
        }
    }

    ide_sb_initialized = 1;
    plogk("ide_sb: IDE superblock initialization completed\n");
}
