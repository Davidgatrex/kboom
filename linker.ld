OUTPUT_FORMAT(binary)
ENTRY(_start)

SECTIONS
{
  . = 0xFFFF800000000000;

  _VIRT_BASE_ = .;

  .kboom_1 :
  {
    *(.kboom_1)
  }

  .text : {
_TEXT_START_ = .;
    *crt0.o(.text)
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

_FILE_SZ_ = . - _VIRT_BASE_;

  .bss : {
_BSS_START_ = .;
    *(.bss)
_BSS_END_ = .;
  }
  /DISCARD/ : { * }
}