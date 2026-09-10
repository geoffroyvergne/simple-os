#include "sfs.h"
#include "ata.h"
#include "kheap.h"
#include "string.h"
#include "kprintf.h"

static struct sfs_super super;
static struct sfs_dirent *dir;          /* dir_blocks * 512 bytes, from kmalloc */
static int mounted;
static uint8_t sector[SFS_BLOCK];       /* single-threaded bounce buffer */

static uint32_t dlba(uint32_t rel_block) { return SFS_DISK_LBA + rel_block; }

static int flush_dir(void)
{
    return ata_write(dlba(super.dir_start), super.dir_blocks, dir);
}

int sfs_mount(void)
{
    if (ata_read(SFS_DISK_LBA, 1, &super) < 0) {
        kprintf("sfs: superblock read failed\n");
        return -1;
    }
    if (super.magic != SFS_MAGIC) {
        kprintf("sfs: bad magic 0x%x at LBA %u\n", super.magic, SFS_DISK_LBA);
        return -1;
    }

    dir = kmalloc(super.dir_blocks * SFS_BLOCK);
    if (!dir)
        return -1;
    if (ata_read(dlba(super.dir_start), super.dir_blocks, dir) < 0)
        return -1;

    mounted = 1;
    uint32_t tb, ub, nf;
    sfs_statfs(&tb, &ub, &nf);
    kprintf("sfs: mounted, %u/%u blocks used, %u file(s)\n", ub, tb, nf);
    return 0;
}

int sfs_max_files(void) { return mounted ? (int)super.max_files : 0; }

const struct sfs_dirent *sfs_entry(int slot)
{
    if (!mounted || slot < 0 || slot >= (int)super.max_files)
        return 0;
    return dir[slot].used ? &dir[slot] : 0;
}

int sfs_lookup(const char *name)
{
    if (!mounted)
        return -1;
    for (uint32_t i = 0; i < super.max_files; i++)
        if (dir[i].used && strncmp(dir[i].name, name, SFS_NAME_MAX) == 0)
            return (int)i;
    return -1;
}

int sfs_read(int slot, uint32_t offset, void *buf, uint32_t len)
{
    const struct sfs_dirent *e = sfs_entry(slot);
    if (!e)
        return -1;
    if (offset >= e->size)
        return 0;
    if (offset + len > e->size)
        len = e->size - offset;

    uint8_t *out = buf;
    uint32_t got = 0;
    while (got < len) {
        uint32_t pos = offset + got;
        uint32_t blk = e->start_block + pos / SFS_BLOCK;
        uint32_t within = pos % SFS_BLOCK;
        uint32_t n = SFS_BLOCK - within;
        if (n > len - got)
            n = len - got;

        if (ata_read(dlba(blk), 1, sector) < 0)
            return got ? (int)got : -1;
        memcpy(out + got, sector + within, n);
        got += n;
    }
    return (int)got;
}

int sfs_create(const char *name, uint32_t capacity_bytes)
{
    if (!mounted || sfs_lookup(name) >= 0)
        return -1;

    int slot = -1;
    for (uint32_t i = 0; i < super.max_files; i++)
        if (!dir[i].used) { slot = (int)i; break; }
    if (slot < 0)
        return -1;

    uint32_t cap_blocks = (capacity_bytes + SFS_BLOCK - 1) / SFS_BLOCK;
    if (cap_blocks == 0)
        cap_blocks = 1;

    uint32_t cursor = super.data_start;
    for (uint32_t i = 0; i < super.max_files; i++) {
        if (!dir[i].used)
            continue;
        uint32_t end = dir[i].start_block + dir[i].capacity_blocks;
        if (end > cursor)
            cursor = end;
    }
    if (cursor + cap_blocks > super.total_blocks)
        return -1;

    struct sfs_dirent *e = &dir[slot];
    memset(e, 0, sizeof(*e));
    e->used = 1;
    strncpy(e->name, name, SFS_NAME_MAX - 1);
    e->start_block = cursor;
    e->size = 0;
    e->capacity_blocks = cap_blocks;

    memset(sector, 0, SFS_BLOCK);
    for (uint32_t b = 0; b < cap_blocks; b++)
        ata_write(dlba(cursor + b), 1, sector);

    if (flush_dir() < 0)
        return -1;
    return slot;
}

int sfs_write(int slot, uint32_t offset, const void *buf, uint32_t len)
{
    if (!mounted || slot < 0 || slot >= (int)super.max_files || !dir[slot].used)
        return -1;
    struct sfs_dirent *e = &dir[slot];

    uint32_t cap_bytes = e->capacity_blocks * SFS_BLOCK;
    if (offset >= cap_bytes)
        return -1;
    if (offset + len > cap_bytes)
        len = cap_bytes - offset;

    const uint8_t *in = buf;
    uint32_t put = 0;
    while (put < len) {
        uint32_t pos = offset + put;
        uint32_t blk = e->start_block + pos / SFS_BLOCK;
        uint32_t within = pos % SFS_BLOCK;
        uint32_t n = SFS_BLOCK - within;
        if (n > len - put)
            n = len - put;

        if (n != SFS_BLOCK) {
            if (ata_read(dlba(blk), 1, sector) < 0)
                return put ? (int)put : -1;
        }
        memcpy(sector + within, in + put, n);
        if (ata_write(dlba(blk), 1, sector) < 0)
            return put ? (int)put : -1;
        put += n;
    }

    if (offset + put > e->size) {
        e->size = offset + put;
        flush_dir();
    }
    return (int)put;
}

int sfs_unlink(int slot)
{
    if (!mounted || slot < 0 || slot >= (int)super.max_files || !dir[slot].used)
        return -1;
    dir[slot].used = 0;
    return flush_dir();
}

void sfs_statfs(uint32_t *total_blocks, uint32_t *used_blocks, uint32_t *files)
{
    uint32_t used = mounted ? super.data_start : 0;
    uint32_t nf = 0;
    if (mounted) {
        for (uint32_t i = 0; i < super.max_files; i++) {
            if (dir[i].used) {
                used += dir[i].capacity_blocks;
                nf++;
            }
        }
    }
    if (total_blocks) *total_blocks = mounted ? super.total_blocks : 0;
    if (used_blocks)  *used_blocks = used;
    if (files)        *files = nf;
}
