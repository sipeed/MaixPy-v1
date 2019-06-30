/*===================================================================*/
/*                                                                   */
/*                    Mapper 76 (Namcot 109)                         */
/*                                                                   */
/*===================================================================*/

BYTE Map76_Reg;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 76                                             */
/*-------------------------------------------------------------------*/
void Map76_Init()
{
  /* Initialize Mapper */
  MapperInit = Map76_Init;

  /* Write to Mapper */
  MapperWrite = Map76_Write;

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
  ROMBANK2 = ROMLASTPAGE( 1 );
  ROMBANK3 = ROMLASTPAGE( 0 );

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
/*  Mapper 76 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map76_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x8000:
      Map76_Reg = byData;
      break;

    case 0x8001:
      switch ( Map76_Reg & 0x07 )
      {
        case 0x02:
          byData <<= 1;
          byData %= ( NesHeader.byVRomSize << 3 );
          PPUBANK[ 0 ] = VROMPAGE( byData );
          PPUBANK[ 1 ] = VROMPAGE( byData + 1 );
          InfoNES_SetupChr();
          break;

        case 0x03:
          byData <<= 1;
          byData %= ( NesHeader.byVRomSize << 3 );
          PPUBANK[ 2 ] = VROMPAGE( byData );
          PPUBANK[ 3 ] = VROMPAGE( byData + 1 );
          InfoNES_SetupChr();
          break;

        case 0x04:
          byData <<= 1;
          byData %= ( NesHeader.byVRomSize << 3 );
          PPUBANK[ 4 ] = VROMPAGE( byData );
          PPUBANK[ 5 ] = VROMPAGE( byData + 1 );
          InfoNES_SetupChr();
          break;

        case 0x05:
          byData <<= 1;
          byData %= ( NesHeader.byVRomSize << 3 );
          PPUBANK[ 6 ] = VROMPAGE( byData );
          PPUBANK[ 7 ] = VROMPAGE( byData + 1 );
          InfoNES_SetupChr();
          break;

        case 0x06:
          byData %= ( NesHeader.byRomSize << 1 );
          ROMBANK0 = ROMPAGE( byData );
          break;

        case 0x07:
          byData %= ( NesHeader.byRomSize << 1 );
          ROMBANK1 = ROMPAGE( byData );
          break;
      }
      break;
  }  
}
