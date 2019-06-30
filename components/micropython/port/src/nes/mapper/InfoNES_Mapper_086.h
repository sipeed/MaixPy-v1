/*===================================================================*/
/*                                                                   */
/*                        Mapper 86 (Jaleco)                         */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 86                                             */
/*-------------------------------------------------------------------*/
void Map86_Init()
{
  /* Initialize Mapper */
  MapperInit = Map86_Init;

  /* Write to Mapper */
  MapperWrite = Map0_Write;

  /* Write to SRAM */
  MapperSram = Map86_Sram;

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
  if ( NesHeader.byVRomSize > 0 )
  {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 86 Write to Sram Function                                 */
/*-------------------------------------------------------------------*/
void Map86_Sram( WORD wAddr, BYTE byData )
{
  BYTE byChrBank;
  BYTE byPrgBank;

  switch ( wAddr )
  {
    case 0x6000:
      byChrBank = byData & 0x03 | ( byData & 0x40 ) >> 4;
      byPrgBank = ( byData & 0x30 ) >> 4;

      byPrgBank = ( byPrgBank << 2 ) % ( NesHeader.byRomSize << 1 );
      byChrBank = ( byChrBank << 3 ) % ( NesHeader.byVRomSize << 3 );

      /* Set ROM Banks */
      ROMBANK0 = ROMPAGE( byPrgBank + 0 );
      ROMBANK1 = ROMPAGE( byPrgBank + 1 );
      ROMBANK2 = ROMPAGE( byPrgBank + 2 );
      ROMBANK3 = ROMPAGE( byPrgBank + 3 );

      /* Set PPU Banks */
      PPUBANK[ 0 ] = VROMPAGE( byChrBank + 0 );
      PPUBANK[ 1 ] = VROMPAGE( byChrBank + 1 );
      PPUBANK[ 2 ] = VROMPAGE( byChrBank + 2 );
      PPUBANK[ 3 ] = VROMPAGE( byChrBank + 3 );
      PPUBANK[ 4 ] = VROMPAGE( byChrBank + 4 );
      PPUBANK[ 5 ] = VROMPAGE( byChrBank + 5 );
      PPUBANK[ 6 ] = VROMPAGE( byChrBank + 6 );
      PPUBANK[ 7 ] = VROMPAGE( byChrBank + 7 );
      InfoNES_SetupChr();
      break;
  }
}
