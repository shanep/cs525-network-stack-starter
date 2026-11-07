/* Do NOT modify this file! Load at 1 MB, with the Multiboot header up front. */
ENTRY(_start)

SECTIONS {
    . = 1M;
    .text : ALIGN(4K) {
        KEEP(*(.multiboot))
        *(.text*)
    }
    .rodata : ALIGN(4K) { *(.rodata*) }
    .data : ALIGN(4K) { *(.data*) }
    .bss : ALIGN(4K) {
        __bss_start = .;
        *(COMMON)
        *(.bss*)
        __bss_end = .;
    }
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
