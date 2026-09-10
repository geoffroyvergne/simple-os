#include "ata.h"
#include "io.h"
#include "kprintf.h"

#define IO   0x1F0            /* primary bus I/O base */
#define CTRL 0x3F6            /* primary bus control */

#define REG_DATA    (IO + 0)
#define REG_ERROR   (IO + 1)
#define REG_SECCNT  (IO + 2)
#define REG_LBA0    (IO + 3)
#define REG_LBA1    (IO + 4)
#define REG_LBA2    (IO + 5)
#define REG_DRIVE   (IO + 6)
#define REG_STATUS  (IO + 7)
#define REG_COMMAND (IO + 7)

#define ST_ERR  0x01
#define ST_DRQ  0x08
#define ST_SRV  0x10
#define ST_DF   0x20
#define ST_RDY  0x40
#define ST_BSY  0x80

#define CMD_READ_PIO   0x20
#define CMD_WRITE_PIO  0x30
#define CMD_FLUSH      0xE7
#define CMD_IDENTIFY   0xEC

static void delay400(void)
{
    for (int i = 0; i < 4; i++)
        (void)inb(CTRL);
}

static int wait_ready(void)
{
    for (int i = 0; i < 1000000; i++) {
        uint8_t s = inb(REG_STATUS);
        if (s & ST_BSY)
            continue;
        if (s & (ST_ERR | ST_DF))
            return -1;
        return 0;
    }
    return -2;
}

static int wait_drq(void)
{
    for (int i = 0; i < 1000000; i++) {
        uint8_t s = inb(REG_STATUS);
        if (s & (ST_ERR | ST_DF))
            return -1;
        if (!(s & ST_BSY) && (s & ST_DRQ))
            return 0;
    }
    return -2;
}

static void select_lba(uint32_t lba, uint32_t count)
{
    outb(REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));   /* master, LBA mode */
    delay400();
    outb(REG_ERROR, 0x00);
    outb(REG_SECCNT, (uint8_t)count);               /* 0 would mean 256 */
    outb(REG_LBA0, (uint8_t)(lba));
    outb(REG_LBA1, (uint8_t)(lba >> 8));
    outb(REG_LBA2, (uint8_t)(lba >> 16));
}

int ata_read(uint32_t lba, uint32_t count, void *buf)
{
    uint8_t *p = buf;
    for (uint32_t done = 0; done < count; ) {
        uint32_t chunk = count - done;
        if (chunk > 255)
            chunk = 255;

        if (wait_ready() < 0)
            return -1;
        select_lba(lba + done, chunk);
        outb(REG_COMMAND, CMD_READ_PIO);

        for (uint32_t s = 0; s < chunk; s++) {
            if (wait_drq() < 0)
                return -1;
            insw(REG_DATA, p, 256);
            p += 512;
            delay400();
        }
        done += chunk;
    }
    return 0;
}

int ata_write(uint32_t lba, uint32_t count, const void *buf)
{
    const uint8_t *p = buf;
    for (uint32_t done = 0; done < count; ) {
        uint32_t chunk = count - done;
        if (chunk > 255)
            chunk = 255;

        if (wait_ready() < 0)
            return -1;
        select_lba(lba + done, chunk);
        outb(REG_COMMAND, CMD_WRITE_PIO);

        for (uint32_t s = 0; s < chunk; s++) {
            if (wait_drq() < 0)
                return -1;
            outsw(REG_DATA, p, 256);
            p += 512;
            delay400();
        }
        outb(REG_COMMAND, CMD_FLUSH);
        if (wait_ready() < 0)
            return -1;
        done += chunk;
    }
    return 0;
}

int ata_init(void)
{
    outb(REG_DRIVE, 0xA0);
    delay400();
    if (inb(REG_STATUS) == 0xFF) {
        kprintf("ata: no drive on the primary bus\n");
        return -1;
    }

    outb(REG_SECCNT, 0);
    outb(REG_LBA0, 0);
    outb(REG_LBA1, 0);
    outb(REG_LBA2, 0);
    outb(REG_COMMAND, CMD_IDENTIFY);

    if (inb(REG_STATUS) == 0) {
        kprintf("ata: IDENTIFY reports no drive\n");
        return -1;
    }
    if (wait_drq() < 0) {
        kprintf("ata: IDENTIFY failed\n");
        return -1;
    }

    uint16_t id[256];
    insw(REG_DATA, id, 256);
    uint32_t sectors = id[60] | ((uint32_t)id[61] << 16);   /* 28-bit LBA count */
    kprintf("ata: primary master, %u sectors (%u MiB)\n", sectors, sectors / 2048);
    return 0;
}
