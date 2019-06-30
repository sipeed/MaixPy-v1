/*===================================================================*/
/*                                                                   */
/*                   Mapper 225 : 72-in-1                            */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 225                                            */
/*-------------------------------------------------------------------*/
void Map225_Init()
{
  /* Initialize Mapper */
  MapperInit = Map225_Init;

  /* Write to Mapper */
  MapperWrite = Map225_Write;

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

  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( 0 );
  ROMBANK1 = ROMPAGE( 1 );
  ROMBANK2 = ROMPAGE( 2 );
  ROMBANK3 = ROMPAGE( 3 );

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 ) {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 225 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map225_Write( WORD wAddr, BYTE byData )
{
  BYTE byPrgBank = (wAddr & 0x0F80) >> 7;
  BYTE byChrBank = wAddr & 0x003F;
  
  PPUBANK[ 0 ] = VROMPAGE(((byChrBank<<3)+0) % ( NesHeader.byVRomSize << 3 ) );
  PPUBANK[ 1 ] = VROMPAGE(((byChrBank<<3)+1) % ( NesHeader.byVRomSize << 3 ) );
  PPUBANK[ 2 ] = VROMPAGE(((byChrBank<<3)+2) % ( NesHeader.byVRomSize << 3 ) );
  PPUBANK[ 3 ] = VROMPAGE(((byChrBank<<3)+3) % ( NesHeader.byVRomSize << 3 ) );
  PPUBANK[ 4 ] = VROMPAGE(((byChrBank<<3)+4) % ( NesHeader.byVRomSize << 3 ) );
  PPUBANK[ 5 ] = VROMPAGE(((byChrBank<<3)+5) % ( NesHeader.byVRomSize << 3 ) );
  PPUBANK[ 6 ] = VROMPAGE(((byChrBank<<3)+6) % ( NesHeader.byVRomSize << 3 ) );
  PPUBANK[ 7 ] = VROMPAGE(((byChrBank<<3)+7) % ( NesHeader.byVRomSize << 3 ) );
  InfoNES_SetupChr();

  if( wAddr & 0x2000 ) {
    InfoNES_Mirroring( 0 );
  } else {
    InfoNES_Mirroring( 1 );
  }
  
  if( wAddr & 0x1000 ) {
    // 16KBbank
    if( wAddr & 0x0040 ) {
      ROMBANK0 = ROMPAGE(((byPrgBank<<2)+2) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK1 = ROMPAGE(((byPrgBank<<2)+3) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK2 = ROMPAGE(((byPrgBank<<2)+2) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK3 = ROMPAGE(((byPrgBank<<2)+3) % ( NesHeader.byRomSize << 1 ) );
    } else {
      ROMBANK0 = ROMPAGE(((byPrgBank<<2)+0) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK1 = ROMPAGE(((byPrgBank<<2)+1) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK2 = ROMPAGE(((byPrgBank<<2)+0) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK3 = ROMPAGE(((byPrgBank<<2)+1) % ( NesHeader.byRomSize << 1 ) );
    }
  } else {
    ROMBANK0 = ROMPAGE(((byPrgBank<<2)+0) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK1 = ROMPAGE(((byPrgBank<<2)+1) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMPAGE(((byPrgBank<<2)+2) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK3 = ROMPAGE(((byPrgBank<<2)+3) % ( NesHeader.byRomSize << 1 ) );
  }
}
