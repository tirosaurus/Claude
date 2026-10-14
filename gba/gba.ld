/* Mapa de memoria de la Game Boy Advance */
OUTPUT_FORMAT("elf32-littlearm")
OUTPUT_ARCH(arm)
ENTRY(_start)
MEMORY {
  rom   (rx)  : ORIGIN = 0x08000000, LENGTH = 32M
  iwram (rwx) : ORIGIN = 0x03000000, LENGTH = 32K - 0x200
  ewram (rwx) : ORIGIN = 0x02000000, LENGTH = 256K
}
SECTIONS {
  .text : {
    KEEP(*(.crt0))
    *(.text .text.*)
    *(.rodata .rodata.*)
    . = ALIGN(4);
  } > rom
  .ARM.exidx : { *(.ARM.exidx*) } > rom
  __data_lma = .;
  .iwram : AT(__data_lma) {
    __iwram_start = .;
    *(.iwram .iwram.*)
    *(.data .data.*)
    . = ALIGN(4);
    __iwram_end = .;
  } > iwram
  .bss (NOLOAD) : {
    __bss_start = .;
    *(.bss .bss.*) *(COMMON)
    . = ALIGN(4);
    __bss_end = .;
  } > iwram
  __ewram_lma = __data_lma + SIZEOF(.iwram);
  .ewram : AT(__ewram_lma) {
    __ewram_start = .;
    *(.ewram .ewram.*)
    . = ALIGN(4);
    __ewram_end = .;
  } > ewram
  .sbss (NOLOAD) : {
    __sbss_start = .;
    *(.sbss .sbss.*)
    . = ALIGN(4);
    __sbss_end = .;
    end = .; _end = .; __end__ = .;
  } > ewram
  /DISCARD/ : { *(.ARM.attributes) *(.comment) }
}
