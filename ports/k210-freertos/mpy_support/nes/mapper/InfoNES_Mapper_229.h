/*===================================================================*/
/*                                                                   */
/*                     Mapper 229 : 31-in-1                          */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 229                                            */
/*-------------------------------------------------------------------*/
void Map229_Init()
{
  /* Initialize Mapper */
  MapperInit = Map229_Init;

  /* Write to Mapper */
  MapperWrite = Map229_Write;

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
/*  Mapper 229 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map229_Write( WORD wAddr, BYTE byData )
{
  if( wAddr & 0x001E ) {
    BYTE byPrg = wAddr&0x001F;

    ROMBANK0 = ROMPAGE(((byPrg<<1)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE(((byPrg<<1)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE(((byPrg<<1)+0) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE(((byPrg<<1)+1) % (NesHeader.byRomSize<<1));

    BYTE byChr = wAddr & 0x0FFF;

    PPUBANK[ 0 ] = VROMPAGE(((byChr<<3) + 0) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 1 ] = VROMPAGE(((byChr<<3) + 1) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 2 ] = VROMPAGE(((byChr<<3) + 2) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 3 ] = VROMPAGE(((byChr<<3) + 3) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 4 ] = VROMPAGE(((byChr<<3) + 4) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 5 ] = VROMPAGE(((byChr<<3) + 5) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 6 ] = VROMPAGE(((byChr<<3) + 6) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 7 ] = VROMPAGE(((byChr<<3) + 7) % (NesHeader.byVRomSize<<3));
    InfoNES_SetupChr();
  } else {
    ROMBANK0 = ROMPAGE( 0 );
    ROMBANK1 = ROMPAGE( 1 );
    ROMBANK2 = ROMPAGE( 2 );
    ROMBANK3 = ROMPAGE( 3 );

    PPUBANK[ 0 ] = VROMPAGE( 0 );
    PPUBANK[ 1 ] = VROMPAGE( 1 );
    PPUBANK[ 2 ] = VROMPAGE( 2 );
    PPUBANK[ 3 ] = VROMPAGE( 3 );
    PPUBANK[ 4 ] = VROMPAGE( 4 );
    PPUBANK[ 5 ] = VROMPAGE( 5 );
    PPUBANK[ 6 ] = VROMPAGE( 6 );
    PPUBANK[ 7 ] = VROMPAGE( 7 );
    InfoNES_SetupChr();
  }

  if( wAddr & 0x0020 ) {
    InfoNES_Mirroring( 0 );
  } else {
    InfoNES_Mirroring( 1 );
  }
}
