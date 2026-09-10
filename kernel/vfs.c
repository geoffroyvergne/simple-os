#include "vfs.h"
#include "sfs.h"
#include "string.h"

/* Thin layer over the single SFS volume: an open-file table and path handling.
 * Generalises to real mounts once there is more than one filesystem. */

#define MAXFD 16

struct file {
    int      used;
    int      slot;        /* sfs directory slot */
    uint32_t pos;
};

static struct file table[MAXFD];
static int mounted;

static const char *strip(const char *path)
{
    while (*path == '/')
        path++;
    return path;
}

void vfs_init(void)
{
    mounted = (sfs_mount() == 0);
}

int vfs_mounted(void) { return mounted; }

int vfs_open(const char *path)
{
    if (!mounted)
        return -1;
    int slot = sfs_lookup(strip(path));
    if (slot < 0)
        return -1;

    for (int fd = 0; fd < MAXFD; fd++) {
        if (!table[fd].used) {
            table[fd] = (struct file){ .used = 1, .slot = slot, .pos = 0 };
            return fd;
        }
    }
    return -1;
}

static struct file *get(int fd)
{
    if (fd < 0 || fd >= MAXFD || !table[fd].used)
        return 0;
    return &table[fd];
}

int vfs_read(int fd, void *buf, uint32_t n)
{
    struct file *f = get(fd);
    if (!f)
        return -1;
    int r = sfs_read(f->slot, f->pos, buf, n);
    if (r > 0)
        f->pos += (uint32_t)r;
    return r;
}

int vfs_write(int fd, const void *buf, uint32_t n)
{
    struct file *f = get(fd);
    if (!f)
        return -1;
    int w = sfs_write(f->slot, f->pos, buf, n);
    if (w > 0)
        f->pos += (uint32_t)w;
    return w;
}

int vfs_seek(int fd, uint32_t offset)
{
    struct file *f = get(fd);
    if (!f)
        return -1;
    f->pos = offset;
    return 0;
}

uint32_t vfs_tell(int fd)
{
    struct file *f = get(fd);
    return f ? f->pos : 0;
}

void vfs_close(int fd)
{
    struct file *f = get(fd);
    if (f)
        f->used = 0;
}

int vfs_stat(const char *path, struct vfs_stat *st)
{
    if (!mounted)
        return -1;
    int slot = sfs_lookup(strip(path));
    const struct sfs_dirent *e = sfs_entry(slot);
    if (!e)
        return -1;
    if (st) {
        st->size = e->size;
        st->capacity = e->capacity_blocks * SFS_BLOCK;
    }
    return 0;
}

int vfs_readdir(int index, struct vfs_dirent *out)
{
    if (!mounted || index < 0)
        return -1;
    int seen = 0;
    for (int i = 0; i < sfs_max_files(); i++) {
        const struct sfs_dirent *e = sfs_entry(i);
        if (!e)
            continue;
        if (seen == index) {
            if (out) {
                memcpy(out->name, e->name, VFS_NAME_MAX);
                out->name[VFS_NAME_MAX - 1] = '\0';
                out->size = e->size;
            }
            return 0;
        }
        seen++;
    }
    return -1;
}

int vfs_create(const char *path, uint32_t capacity_bytes)
{
    if (!mounted)
        return -1;
    return sfs_create(strip(path), capacity_bytes) >= 0 ? 0 : -1;
}

int vfs_unlink(const char *path)
{
    if (!mounted)
        return -1;
    return sfs_unlink(sfs_lookup(strip(path)));
}
