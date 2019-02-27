/*===================================================================*/
/*                                                                   */
/*                   Mapper 101 ()                                   */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 101                                            */
/*-------------------------------------------------------------------*/
void Map101_Init()
{
  /* Initialize Mapper */
  MapperInit = Map101_Init;

  /* Write to Mapper */
  MapperWrite = Map101_Write;

  /* Write to SRAM */
  MapperSram = Map101_Write;

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
/*  Mapper 101 Write & Write to SRAM Function                        */
/*-------------------------------------------------------------------*/
void Map101_Write( WORD wAddr, BYTE byData )
{
  byData &= 0x03;
  byData <<= 3;
  byData %= ( NesHeader.byVRomSize << 3 );

  /* Set PPU Banks */
  PPUBANK[ 0 ] = VROMPAGE( byData );
  PPUBANK[ 1 ] = VROMPAGE( byData + 1 );
  PPUBANK[ 2 ] = VROMPAGE( byData + 2 );
  PPUBANK[ 3 ] = VROMPAGE( byData + 3 );
  PPUBANK[ 4 ] = VROMPAGE( byData + 4 );
  PPUBANK[ 5 ] = VROMPAGE( byData + 5 );
  PPUBANK[ 6 ] = VROMPAGE( byData + 6 );
  PPUBANK[ 7 ] = VROMPAGE( byData + 7 );
  InfoNES_SetupChr();
}
