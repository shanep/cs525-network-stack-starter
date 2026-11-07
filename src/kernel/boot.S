/*
 * The first code that runs. QEMU's -kernel flag understands Multiboot 1, so it
 * loads our ELF file at 1 MB and jumps to _start in 32 bit protected mode with
 * paging off. That means a pointer is a physical address, which is exactly what
 * a network card wants to be handed for DMA.
 *
 * Do NOT modify this file!
 */
.set MB_MAGIC, 0x1BADB002
.set MB_FLAGS, 0x00000003          /* align modules, give us the memory map */

.section .multiboot, "a"
.align 4
.long MB_MAGIC
.long MB_FLAGS
.long -(MB_MAGIC + MB_FLAGS)

.section .text
.global _start
_start:
    cli
    /* Multiboot leaves the GDT undefined, so install our own flat one. */
    lgdt gdt_desc
    ljmp $0x08, $reload_cs
reload_cs:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs
    mov %ax, %ss
    mov $stack_top, %esp

    /* The C code expects its uninitialized globals to be zero. */
    mov $__bss_start, %edi
    mov $__bss_end, %ecx
    sub %edi, %ecx
    xor %eax, %eax
    cld
    rep stosb

    call kmain
halt:
    hlt
    jmp halt

.section .rodata
.align 8
gdt:
    .quad 0x0000000000000000
    .quad 0x00CF9A000000FFFF       /* 0x08 flat code */
    .quad 0x00CF92000000FFFF       /* 0x10 flat data */
gdt_desc:
    .word gdt_desc - gdt - 1
    .long gdt

.section .bss
.align 16
.skip 65536
stack_top:

.section .note.GNU-stack, "", @progbits
