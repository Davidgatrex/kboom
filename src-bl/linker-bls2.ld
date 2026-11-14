OUTPUT_FORMAT(elf32-i386 )
ENTRY(_start)

SECTIONS
{
  . = 0x100000;

  .multiboot : {
    *(.multiboot)
  }

  .text : {
_TEXT_START_ = .;
    *(.text)
_TEXT_END_ = .;
  }
  .data : {
_DATA_START_ = .;
    *(.data)
_DATA_END_ = .;
  }

  .rodata : {
_RODATA_START_ = .;
    *(.rodata)
_RODATA_END_ = .;
  }

  .bss : {
_BSS_START_ = .;
    *(.bss)
_BSS_END_ = .;
  }
  /DISCARD/ : { * }
}