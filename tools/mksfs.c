/* Host tool: build a SimpleFS (SFS1) image.
 *
 *   mksfs <out.img> <size-bytes> [file ...]
 *
 * Layout: superblock (block 0), directory table (blocks 1..8), then each input
 * file placed contiguously in the data region. Compiled with the host compiler,
 * not the cross toolchain. Keep struct layout in sync with kernel/sfs.h. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SFS_MAGIC     0x31534653u
#define SFS_BLOCK     512u
#define SFS_NAME_MAX  40
#define DIR_BLOCKS    8            /* 8 blocks * 8 entries = 64 files */

struct sfs_super {
    uint32_t magic, block_size, total_blocks, dir_start, dir_blocks, data_start,
             max_files, reserved[121];
};

struct sfs_dirent {
    uint8_t  used, pad[3];
    char     name[SFS_NAME_MAX];
    uint32_t start_block, size, capacity_blocks, reserved[2];
};

static const char *basename_of(const char *p)
{
    const char *s = strrchr(p, '/');
    return s ? s + 1 : p;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <out.img> <size-bytes> [file ...]\n", argv[0]);
        return 2;
    }

    const char *out_path = argv[1];
    uint64_t size_bytes = strtoull(argv[2], NULL, 0);
    uint32_t total_blocks = (uint32_t)(size_bytes / SFS_BLOCK);
    uint32_t data_start = 1 + DIR_BLOCKS;

    if (total_blocks <= data_start) {
        fprintf(stderr, "image too small\n");
        return 2;
    }

    uint8_t *img = calloc(total_blocks, SFS_BLOCK);
    if (!img) { perror("calloc"); return 1; }

    struct sfs_super *sb = (struct sfs_super *)img;
    sb->magic = SFS_MAGIC;
    sb->block_size = SFS_BLOCK;
    sb->total_blocks = total_blocks;
    sb->dir_start = 1;
    sb->dir_blocks = DIR_BLOCKS;
    sb->data_start = data_start;
    sb->max_files = DIR_BLOCKS * (SFS_BLOCK / (uint32_t)sizeof(struct sfs_dirent));

    struct sfs_dirent *dir = (struct sfs_dirent *)(img + SFS_BLOCK);
    uint32_t cursor = data_start;

    for (int i = 3; i < argc; i++) {
        FILE *f = fopen(argv[i], "rb");
        if (!f) { fprintf(stderr, "cannot open %s\n", argv[i]); return 1; }
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fseek(f, 0, SEEK_SET);

        uint32_t nblocks = (uint32_t)((fsize + SFS_BLOCK - 1) / SFS_BLOCK);
        if (nblocks == 0)
            nblocks = 1;

        if ((int)(i - 3) >= (int)sb->max_files) {
            fprintf(stderr, "too many files\n");
            return 1;
        }
        if (cursor + nblocks > total_blocks) {
            fprintf(stderr, "out of space at %s\n", argv[i]);
            return 1;
        }

        struct sfs_dirent *e = &dir[i - 3];
        e->used = 1;
        snprintf(e->name, SFS_NAME_MAX, "%s", basename_of(argv[i]));
        e->start_block = cursor;
        e->size = (uint32_t)fsize;
        e->capacity_blocks = nblocks;

        if (fsize > 0 && fread(img + (size_t)cursor * SFS_BLOCK, 1, fsize, f) != (size_t)fsize) {
            fprintf(stderr, "short read on %s\n", argv[i]);
            return 1;
        }
        fclose(f);

        printf("  %-20s %8ld bytes  blocks %u..%u\n",
               e->name, fsize, cursor, cursor + nblocks - 1);
        cursor += nblocks;
    }

    FILE *o = fopen(out_path, "wb");
    if (!o) { perror("fopen"); return 1; }
    if (fwrite(img, SFS_BLOCK, total_blocks, o) != total_blocks) {
        perror("fwrite");
        return 1;
    }
    fclose(o);
    free(img);

    printf("mksfs: %s  %u blocks (%llu bytes), %d file(s), data used through block %u\n",
           out_path, total_blocks, (unsigned long long)size_bytes, argc - 3, cursor - 1);
    return 0;
}
