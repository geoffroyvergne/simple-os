#pragma once
#include <stdint.h>

#define VFS_NAME_MAX 40

struct vfs_stat {
    uint32_t size;
    uint32_t capacity;
};

struct vfs_dirent {
    char     name[VFS_NAME_MAX];
    uint32_t size;
};

void vfs_init(void);
int  vfs_mounted(void);

int  vfs_open(const char *path);                 /* fd >= 0, or -1 */
int  vfs_read(int fd, void *buf, uint32_t n);    /* bytes, 0 at EOF, -1 error */
int  vfs_write(int fd, const void *buf, uint32_t n);
int  vfs_seek(int fd, uint32_t offset);
uint32_t vfs_tell(int fd);
void vfs_close(int fd);

int  vfs_stat(const char *path, struct vfs_stat *st);
int  vfs_readdir(int index, struct vfs_dirent *out);   /* -1 when past the end */
int  vfs_create(const char *path, uint32_t capacity_bytes);
int  vfs_unlink(const char *path);
