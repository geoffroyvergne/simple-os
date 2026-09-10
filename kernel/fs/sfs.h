#pragma once
#include <stdint.h>

/* SimpleFS (SFS1) -- a deliberately tiny on-disk format:
 *   block 0                : superblock
 *   blocks [1 .. data_start): flat directory table (64-byte entries)
 *   blocks [data_start ..)  : file data, each file a contiguous run
 *
 * No subdirectories, contiguous allocation, fixed per-file capacity. All block
 * numbers are relative to the start of the filesystem on disk. */

#define SFS_MAGIC     0x31534653u      /* "SFS1" */
#define SFS_BLOCK     512u
#define SFS_NAME_MAX  40
#define SFS_DIR_ENTRY 64

/* Filesystem starts at this LBA on the boot disk. MUST match the Makefile. */
#define SFS_DISK_LBA  2048u

struct sfs_super {
    uint32_t magic;
    uint32_t block_size;
    uint32_t total_blocks;
    uint32_t dir_start;         /* = 1 */
    uint32_t dir_blocks;
    uint32_t data_start;
    uint32_t max_files;
    uint32_t reserved[121];
};

struct sfs_dirent {
    uint8_t  used;
    uint8_t  pad[3];
    char     name[SFS_NAME_MAX];
    uint32_t start_block;
    uint32_t size;             /* bytes */
    uint32_t capacity_blocks;
    uint32_t reserved[2];
};

_Static_assert(sizeof(struct sfs_super) == SFS_BLOCK, "super must be one block");
_Static_assert(sizeof(struct sfs_dirent) == SFS_DIR_ENTRY, "dirent must be 64 bytes");

/* ---- in-kernel API (backed by the ATA driver) ---- */

int sfs_mount(void);
int sfs_lookup(const char *name);                 /* dir slot, or -1 */
const struct sfs_dirent *sfs_entry(int slot);     /* NULL if slot invalid/free */
int sfs_max_files(void);

int sfs_read(int slot, uint32_t offset, void *buf, uint32_t len);   /* bytes, or -1 */
int sfs_create(const char *name, uint32_t capacity_bytes);          /* slot, or -1 */
int sfs_write(int slot, uint32_t offset, const void *buf, uint32_t len);
int sfs_unlink(int slot);

void sfs_statfs(uint32_t *total_blocks, uint32_t *used_blocks, uint32_t *files);
