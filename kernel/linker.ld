/* SimpleOS kernel link script.
 *
 * The kernel is a flat binary loaded by Stage 2 at physical 0x10000 and entered
 * there directly (no ELF parsing yet), so it is both loaded and linked at that
 * address. _start must be the very first byte of the image.
 */

ENTRY(_start)

SECTIONS
{
    . = 0x00010000;
    __kernel_start = .;

    .text : ALIGN(16)
    {
        *(.text.boot)          /* entry stub first */
        *(.text .text.*)
    }

    .rodata : ALIGN(16) { *(.rodata .rodata.*) }
    .data   : ALIGN(16) { *(.data .data.*) }

    .bss : ALIGN(16)
    {
        __bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        __bss_end = .;
    }

    . = ALIGN(4K);
    __kernel_end = .;

    /DISCARD/ : { *(.comment) *(.note .note.*) *(.eh_frame) *(.eh_frame_hdr) }
}
