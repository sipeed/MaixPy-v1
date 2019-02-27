/*===================================================================*/
/*                                                                   */
/*                      Mapper 34 (Nina-1)                           */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 34                                             */
/*-------------------------------------------------------------------*/
void Map34_Init()
{
  /* Initialize Mapper */
  MapperInit = Map34_Init;

  /* Write to Mapper */
  MapperWrite = Map34_Write;

  /* Write to SRAM */
  MapperSram = Map34_Sram;

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
/*  Mapper 34 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map34_Write( WORD wAddr, BYTE byData )
{
  /* Set ROM Banks */
  byData <<= 2;
  byData %= ( NesHeader.byRomSize << 1);

  ROMBANK0 = ROMPAGE( byData );
  ROMBANK1 = ROMPAGE( byData + 1 );
  ROMBANK2 = ROMPAGE( byData + 2 );
  ROMBANK3 = ROMPAGE( byData + 3 );
}

/*-------------------------------------------------------------------*/
/*  Mapper 34 Write to SRAM Function                                 */
/*-------------------------------------------------------------------*/
void Map34_Sram( WORD wAddr, BYTE byData )
{
  switch(wAddr)
  {
    /* Set ROM Banks */
    case 0x7ffd:
      byData <<= 2;
      byData %= ( NesHeader.byRomSize << 1 );

      ROMBANK0 = ROMPAGE( byData );
      ROMBANK1 = ROMPAGE( byData + 1 );
      ROMBANK2 = ROMPAGE( byData + 2 );
      ROMBANK3 = ROMPAGE( byData + 3 );
      break;

    /* Set PPU Banks */
    case 0x7ffe:
      byData <<= 2;
      byData %= ( NesHeader.byVRomSize << 3 );
      
      PPUBANK[ 0 ] = VROMPAGE( byData );
      PPUBANK[ 1 ] = VROMPAGE( byData + 1 );
      PPUBANK[ 2 ] = VROMPAGE( byData + 2 );
      PPUBANK[ 3 ] = VROMPAGE( byData + 3 );
      InfoNES_SetupChr();
      break;

    /* Set PPU Banks */
    case 0x7fff:
      byData <<= 2;
      byData %= ( NesHeader.byVRomSize << 3 );
      
      PPUBANK[ 4 ] = VROMPAGE( byData );
      PPUBANK[ 5 ] = VROMPAGE( byData + 1 );
      PPUBANK[ 6 ] = VROMPAGE( byData + 2 );
      PPUBANK[ 7 ] = VROMPAGE( byData + 3 );
      InfoNES_SetupChr();
      break;
  }
}
