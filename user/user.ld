/* User programs are ET_EXEC linked at USER_BASE (1 GiB), above the kernel's
 * identity map so they get a private region in each process address space. */

ENTRY(_start)

SECTIONS
{
    . = 0x40000000;

    .text   : ALIGN(16) { *(.text .text.*) }
    .rodata : ALIGN(16) { *(.rodata .rodata.*) }
    .data   : ALIGN(16) { *(.data .data.*) }
    .bss    : ALIGN(16) { *(.bss .bss.*) *(COMMON) }

    /DISCARD/ : { *(.comment) *(.note .note.*) *(.eh_frame) *(.eh_frame_hdr) }
}
