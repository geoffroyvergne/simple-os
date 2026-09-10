#pragma once
#include <stdint.h>

/* Polled ATA PIO on the primary bus, master drive (where the BIOS found us).
 * 28-bit LBA, 512-byte sectors. Returns 0 on success, negative on error. */

int ata_init(void);
int ata_read(uint32_t lba, uint32_t count, void *buf);
int ata_write(uint32_t lba, uint32_t count, const void *buf);
