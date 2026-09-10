#include "elf.h"
#include "vfs.h"
#include "vmm.h"
#include "pmm.h"
#include "kheap.h"
#include "string.h"
#include "kprintf.h"

#define MAX_IMAGE (1u << 20)     /* 1 MiB cap on an executable */

static int load_segment(uint32_t *dir, const uint8_t *file,
                        const Elf32_Phdr *ph)
{
    for (uint32_t off = 0; off < ph->p_memsz; ) {
        uint32_t va = ph->p_vaddr + off;
        uint32_t page = va & ~0xFFFu;

        uint32_t phys = vmm_phys(dir, page);
        if (!phys) {
            phys = pmm_alloc_frame();
            if (!phys)
                return -1;
            memset((void *)phys, 0, PAGE_SIZE);
            vmm_map_user(dir, page, phys, 1);   /* W for simplicity; see notes */
        }

        uint32_t within = va & 0xFFFu;
        uint32_t n = PAGE_SIZE - within;
        if (n > ph->p_memsz - off)
            n = ph->p_memsz - off;

        uint32_t from_file = (off < ph->p_filesz) ? (ph->p_filesz - off) : 0;
        uint32_t copy = n < from_file ? n : from_file;
        if (copy)
            memcpy((void *)((phys & ~0xFFFu) + within),
                   file + ph->p_offset + off, copy);

        off += n;
    }
    return 0;
}

int elf_load(const char *path, uint32_t *dir, uint32_t *entry)
{
    struct vfs_stat st;
    if (vfs_stat(path, &st) < 0)
        return -1;
    if (st.size < sizeof(Elf32_Ehdr) || st.size > MAX_IMAGE)
        return -2;

    uint8_t *buf = kmalloc(st.size);
    if (!buf)
        return -3;

    int fd = vfs_open(path);
    if (fd < 0) {
        kfree(buf);
        return -1;
    }
    uint32_t got = 0;
    int r;
    while (got < st.size && (r = vfs_read(fd, buf + got, st.size - got)) > 0)
        got += (uint32_t)r;
    vfs_close(fd);

    int rc = -4;
    const Elf32_Ehdr *eh = (const Elf32_Ehdr *)buf;
    if (got != st.size)
        goto done;
    if (!(eh->e_ident[0] == 0x7F && eh->e_ident[1] == 'E' &&
          eh->e_ident[2] == 'L' && eh->e_ident[3] == 'F'))
        goto done;
    if (eh->e_ident[4] != 1 /* ELFCLASS32 */ || eh->e_machine != EM_386 ||
        eh->e_type != ET_EXEC)
        goto done;

    for (uint16_t i = 0; i < eh->e_phnum; i++) {
        const Elf32_Phdr *ph =
            (const Elf32_Phdr *)(buf + eh->e_phoff + (uint32_t)i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD || ph->p_memsz == 0)
            continue;
        if (ph->p_vaddr < USER_BASE || ph->p_vaddr + ph->p_memsz >= USER_STACK_TOP) {
            kprintf("elf: segment vaddr %p out of the user region\n",
                    (void *)ph->p_vaddr);
            goto done;
        }
        if (load_segment(dir, buf, ph) < 0)
            goto done;
    }

    *entry = eh->e_entry;
    rc = 0;
done:
    kfree(buf);
    return rc;
}
