/* GD32F130C8: 64KB Flash, 8KB RAM */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 8K
}

ENTRY(Reset_Handler)

SECTIONS
{
  /* Interrupt vector table must be first */
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  /* Code and read-only data */
  .text :
  {
    *(.text*)
    *(.rodata*)
    _etext = .;
  } > FLASH

  /* Initialized data: stored in FLASH, copied to RAM at startup */
  _sidata = LOADADDR(.data);
  .data :
  {
    _sdata = .;
    *(.data*)
    _edata = .;
  } > RAM AT > FLASH

  /* Zero-initialized data */
  .bss :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM

  /DISCARD/ : { *(.eh_frame) }
}
