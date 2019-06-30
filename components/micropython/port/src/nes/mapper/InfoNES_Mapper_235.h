/*===================================================================*/
/*                                                                   */
/*                     Mapper 235 : 150-in-1                         */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 235                                            */
/*-------------------------------------------------------------------*/
void Map235_Init()
{
  /* Initialize Mapper */
  MapperInit = Map235_Init;

  /* Write to Mapper */
  MapperWrite = Map235_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map0_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Set Registers */
  for( int i = 0; i < 0x2000; i++ ) {
    DRAM[i] = 0xFF;
  }

  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( 0 );
  ROMBANK1 = ROMPAGE( 1 );
  ROMBANK2 = ROMPAGE( 2 );
  ROMBANK3 = ROMPAGE( 3 );

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 235 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map235_Write( WORD wAddr, BYTE byData )
{
  BYTE	byPrg = ((wAddr&0x0300)>>3)|(wAddr&0x001F);
  BYTE	byBus = 0;

  if( (NesHeader.byRomSize<<1) == 64*2 ) {
    // 100-in-1
    switch( wAddr & 0x0300 ) {
    case	0x0000:	break;
    case	0x0100:	byBus = 1; break;
    case	0x0200:	byBus = 1; break;
    case	0x0300:	byBus = 1; break;
    }
  } else if( (NesHeader.byRomSize<<1) == 128*2 ) {
    // 150-in-1
    switch( wAddr & 0x0300 ) {
    case	0x0000:	break;
    case	0x0100:	byBus = 1; break;
    case	0x0200:	byPrg = (byPrg&0x1F)|0x20; break;
    case	0x0300:	byBus = 1; break;
    }
  } else if( (NesHeader.byRomSize<<1) == 192*2 ) {
    // 150-in-1
    switch( wAddr & 0x0300 ) {
    case	0x0000:	break;
    case	0x0100:	byBus = 1; break;
    case	0x0200:	byPrg = (byPrg&0x1F)|0x20; break;
    case	0x0300:	byPrg = (byPrg&0x1F)|0x40; break;
    }
  } else if( (NesHeader.byRomSize<<1) == 256*2 ) {
  }
  
  if( wAddr & 0x0800 ) {
    if( wAddr & 0x1000 ) {
      ROMBANK0 = ROMPAGE(((byPrg<<2)+2) % (NesHeader.byRomSize<<1));
      ROMBANK1 = ROMPAGE(((byPrg<<2)+3) % (NesHeader.byRomSize<<1));
      ROMBANK2 = ROMPAGE(((byPrg<<2)+2) % (NesHeader.byRomSize<<1));
      ROMBANK3 = ROMPAGE(((byPrg<<2)+3) % (NesHeader.byRomSize<<1));
    } else {
      ROMBANK0 = ROMPAGE(((byPrg<<2)+0) % (NesHeader.byRomSize<<1));
      ROMBANK1 = ROMPAGE(((byPrg<<2)+1) % (NesHeader.byRomSize<<1));
      ROMBANK2 = ROMPAGE(((byPrg<<2)+0) % (NesHeader.byRomSize<<1));
      ROMBANK3 = ROMPAGE(((byPrg<<2)+1) % (NesHeader.byRomSize<<1));
    }
  } else {
    ROMBANK0 = ROMPAGE(((byPrg<<2)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE(((byPrg<<2)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE(((byPrg<<2)+2) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE(((byPrg<<2)+3) % (NesHeader.byRomSize<<1));
  }
  
  if( byBus ) {
    ROMBANK0 = DRAM;
    ROMBANK1 = DRAM;
    ROMBANK2 = DRAM;
    ROMBANK3 = DRAM;
  }

  if( wAddr & 0x0400 ) {
    InfoNES_Mirroring( 3 );
  } else if( wAddr & 0x2000 ) {
    InfoNES_Mirroring( 0 );
  } else {
    InfoNES_Mirroring( 1 );
  }
}

