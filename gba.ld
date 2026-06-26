MEMORY
{
    ROM (rx)  : ORIGIN = 0x08000000, LENGTH = 32M
    EWRAM (w) : ORIGIN = 0x02000000, LENGTH = 256K
    IWRAM (w) : ORIGIN = 0x03000000, LENGTH = 32K
}

SECTIONS
{
    .text : ALIGN(4)
    {
        KEEP(*(.text.start))
        *(.text*)
        *(.rodata*)
    } > ROM

    .data : ALIGN(4)
    {
        __data_start__ = .;
        *(.data*)
        __data_end__ = .;
    } > EWRAM AT > ROM

    __data_load__ = LOADADDR(.data);

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        __bss_end__ = .;
    } > EWRAM

    __irq_stack_top = ORIGIN(IWRAM) + LENGTH(IWRAM);
    __svc_stack_top = __irq_stack_top - 0x100;
    __sys_stack_top = __svc_stack_top - 0x100;
}
