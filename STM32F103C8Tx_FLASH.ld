ENTRY(Reset_Handler)

/* STM32F103C8T6:
 * Flash: 64 KB
 * RAM:   20 KB
 */

_estack = ORIGIN(RAM) + LENGTH(RAM);

_Min_Heap_Size  = 0x200; /* 512 byte */
_Min_Stack_Size = 0x400; /* 1 KB */

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
    /* Vector table */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Code và dữ liệu chỉ đọc */
    .text :
    {
        . = ALIGN(4);

        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* ARM exception tables */
    .ARM.extab :
    {
        *(.ARM.extab*)
        *(.gnu.linkonce.armextab.*)
    } > FLASH

    .ARM.exidx :
    {
        __exidx_start = .;

        *(.ARM.exidx*)
        *(.gnu.linkonce.armexidx.*)

        __exidx_end = .;
    } > FLASH

    /* Constructor arrays của thư viện C */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    /*
     * Địa chỉ trong Flash chứa giá trị khởi tạo
     * để startup.c copy sang RAM.
     */
    _sidata = LOADADDR(.data);

    /* Biến global/static có giá trị khởi tạo */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;

        *(.data)
        *(.data*)

        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    /* Biến global/static chưa khởi tạo */
    .bss (NOLOAD) :
    {
        . = ALIGN(4);
        _sbss = .;

        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /*
     * Newlib/libnosys cần symbol "end" để _sbrk()
     * biết heap bắt đầu ở đâu.
     */
    .heap_stack (NOLOAD) :
    {
        . = ALIGN(8);

        _end = .;
        PROVIDE(end = .);

        /* Dành tối thiểu 512 byte cho heap. */
        . = . + _Min_Heap_Size;

        /* Dành tối thiểu 1 KB cho stack. */
        . = . + _Min_Stack_Size;

        . = ALIGN(8);
    } > RAM

    /DISCARD/ :
    {
        *(.note.GNU-stack)
        *(.gnu_debuglink)
        *(.gnu.lto_*)
    }
}