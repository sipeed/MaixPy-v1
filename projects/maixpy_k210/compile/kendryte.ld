/*
 * The MEMORY command describes the location and size of blocks of memory
 * in the target. You can use it to describe which memory regions may be
 * used by the linker, and which memory regions it must avoid.
 */
MEMORY
{
  /*
   * Memory with CPU cache.
   *6M CPU SRAM
   */
  ram (wxa!ri) : ORIGIN = 0x80000000, LENGTH = (6 * 1024 * 1024)
  /*
   * Memory without CPU cache
   * 6M CPU SRAM
  */
  ram_nocache (wxa!ri) : ORIGIN = 0x40000000, LENGTH = (6 * 1024 * 1024)
}

PROVIDE( _rom_start  = ORIGIN(rom) );
PROVIDE( _rom_end    = ORIGIN(rom) + LENGTH(rom) );
PROVIDE( _ram_start  = ORIGIN(ram) );
PROVIDE( _ram_end    = ORIGIN(ram) + LENGTH(ram) );
PROVIDE( _io_start   = 0x40000000 );
PROVIDE( _io_end     = _io_start + LENGTH(ram) );
PROVIDE( _stack_size = 1 << 15 );


/*
 * The OUTPUT_ARCH command specifies the machine architecture where the
 * argument is one of the names used in the Kendryte library.
 */
OUTPUT_ARCH( "riscv" )

/*
 * The ENTRY command specifies the entry point (ie. first instruction to
 * execute). The symbol _start is defined in crt0.S
 */
ENTRY(_start)

/*
 * The GROUP command is special since the listed archives will be
 * searched repeatedly until there are no new undefined references. We
 * need this since -lc depends on -lgloss and -lgloss depends on -lc. I
 * thought gcc would automatically include -lgcc when needed, but
 * in this file includes it explicitly here and I was seeing link errors
 * without it.
 */
/* GROUP( -lc -lgloss -lgcc ) */

/*
 * The linker only pays attention to the PHDRS command when generating
 * an ELF output file. In other cases, the linker will simply ignore PHDRS.
 */
PHDRS
{
  ram_ro   PT_LOAD;
  ram_init PT_LOAD;
  ram      PT_NULL;
}

/*
 * This is where we specify how the input sections map to output
 * sections.
 */
SECTIONS
{
  /* Program code segment, also known as a text segment */
  .text :
  {
    PROVIDE( _text = ABSOLUTE(.) );
    /* Initialization code segment */
    KEEP( *(.text.start) )
    KEEP( *(.text.systick) )
    *(.text.unlikely .text.unlikely.*)
    *(.text.startup .text.startup.*)
    /* Normal code segment */
    *(.text .text.*)
    *(.gnu.linkonce.t.*)

    . = ALIGN(8);
    PROVIDE( _etext = ABSOLUTE(.) );
  } >ram AT>ram :ram_ro

  /* Read-only data segment */
  .rodata :
  {
    *(.rdata)
    *(.rodata .rodata.*)
    *(.gnu.linkonce.r.*)
  } >ram AT>ram :ram_ro

  . = ALIGN(8);

  /* Init array and fini array */
  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >ram AT>ram :ram_ro

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
    KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >ram AT>ram :ram_ro

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
    KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >ram AT>ram :ram_ro

  .ctors :
  {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin.o(.ctors))
    KEEP (*crtbegin?.o(.ctors))
    /* We don't want to include the .ctor section from
       the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  } >ram AT>ram :ram_ro

  .dtors :
  {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*crtbegin?.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  } >ram AT>ram :ram_ro

  . = ALIGN(8);

  .lalign :
  {
    . = ALIGN(8);
    PROVIDE( _data_lma = . );
  } >ram AT>ram :ram_ro

  .dalign :
  {
    . = ALIGN(8);
    PROVIDE( _data = . );
  } >ram AT>ram :ram_init

  . = ALIGN(8);

  /* .data, .sdata and .srodata segment */
  .data :
  {
    /* Writable data segment (.data segment) */
    *(.data .data.*)
    *(.gnu.linkonce.d.*)
    /* Have _gp point to middle of sdata/sbss to maximize displacement range */
    . = ALIGN(8);
    PROVIDE( __global_pointer$ = ABSOLUTE(.) + 0x800);
    /* Writable small data segment (.sdata segment) */
    *(.sdata .sdata.*)
    *(.gnu.linkonce.s.*)
    /* Read-only small data segment (.srodata segment) */
    . = ALIGN(8);
    *(.srodata.cst16)
    *(.srodata.cst8)
    *(.srodata.cst4)
    *(.srodata.cst2)
    *(.srodata .srodata.*)
    /* Align _edata to cache line size */
    . = ALIGN(64);
    PROVIDE( _edata = ABSOLUTE(.) );
  } >ram AT>ram :ram_init

  /* .bss and .sbss segment */
  .bss :
  {
    PROVIDE( _bss = ABSOLUTE(.) );
    /* Writable uninitialized small data segment (.sbss segment)*/
    *(.sbss .sbss.*)
    *(.gnu.linkonce.sb.*)
    *(.scommon)
    /* Uninitialized writeable data section (.bss segment)*/
    *(.bss .bss.*)
    *(.gnu.linkonce.b.*)
    *(COMMON)

    . = ALIGN(8);
    PROVIDE( _ebss = ABSOLUTE(.) );
  } >ram AT>ram :ram

  PROVIDE( _tls_data = ABSOLUTE(.) );
  /*
   * Thread Local Storage (TLS) are per-thread global variables.
   * Compilers such as GCC provide a __thread keyword to mark global
   * variables as per-thread. Support is required in the program loader
   * and thread creator.
   */

  /* Thread-local data segment, .tdata (initialized tls). */
  .tdata :
  {
    KEEP( *(.tdata.begin) )
    *(.tdata .tdata.*)
    *(.gnu.linkonce.td.*)
    KEEP( *(.tdata.end) )
  } >ram AT>ram :ram

  /* Thread-local bss segment, .tbss (zero-initialized tls). */
  .tbss :
  {
    *(.tbss .tbss.*)
    *(.gnu.linkonce.tb.*)
    KEEP( *(.tbss.end) )
  } >ram AT>ram :ram

  /*
   * End of uninitalized data segement
   *
   * Actually the stack needs 16B alignment, and it won't hurt to also slightly
   * increase the alignment to 32 or even 64 (cache line size).
   *
   * Align _heap_start to cache line size
   */
  . = ALIGN(64);
  PROVIDE( _end = ABSOLUTE(.) );
  /* Leave 2 holes for stack & TLS, the size can set in kconfig */
  PROVIDE( _heap_start = ABSOLUTE(.) + _stack_size * 2 );
  PROVIDE( _tp0 = (_end + 63) & (-64) );
  PROVIDE( _tp1 = _tp0 + _stack_size );
  PROVIDE( _sp0 = _tp0 + _stack_size );
  PROVIDE( _sp1 = _tp1 + _stack_size );

  /* Heap end is at the end of memory, the memory size can set in kconfig */
  PROVIDE( _heap_end = _ram_end );
}

