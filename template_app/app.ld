SECTIONS
{
  /* Text and literals (IRAM) */
  .text : ALIGN(4)
  {
    *(.literal .text .literal.* .text.*)
  }

  /* Read-only data (DRAM) */
  .rodata : ALIGN(4)
  {
    *(.rodata .rodata.*)
  }

  /* Initialized data (DRAM) */
  .data : ALIGN(4)
  {
    *(.data .data.*)
  }

  /* Uninitialized data (DRAM) */
  .bss : ALIGN(4)
  {
    *(.bss .bss.* COMMON)
  }

  /* Relocation tables */
  .rela.text : { *(.rela.text .rela.text.*) }
  .rela.data : { *(.rela.data .rela.data.*) }
  .rela.rodata : { *(.rela.rodata .rela.rodata.*) }
}
