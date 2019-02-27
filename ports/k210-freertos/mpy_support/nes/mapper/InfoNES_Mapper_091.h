/*===================================================================*/
/*                                                                   */
/*                        Mapper 91 (Pirates)                        */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 91                                             */
/*-------------------------------------------------------------------*/
void Map91_Init()
{
  /* Initialize Mapper */
  MapperInit = Map91_Init;

  /* Write to Mapper */
  MapperWrite = Map0_Write;

  /* Write to SRAM */
  MapperSram = Map91_Sram;

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
  ROMBANK0 = ROMLASTPAGE( 1 );
  ROMBANK1 = ROMLASTPAGE( 0 );
  ROMBANK2 = ROMLASTPAGE( 1 );
  ROMBANK3 = ROMLASTPAGE( 0 );

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 )
  {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Set Name Table Mirroring */
  InfoNES_Mirroring( 1 );

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 91 Write to Sram Function                                 */
/*-------------------------------------------------------------------*/
void Map91_Sram( WORD wAddr, BYTE byData )
{
  switch( wAddr & 0xF00F)
  {
    /* Set PPU Banks */
    case 0x6000:
      PPUBANK[ 0 ] = VROMPAGE( (byData*2+0) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( (byData*2+1) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0x6001:
      PPUBANK[ 2 ] = VROMPAGE( (byData*2+0) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( (byData*2+1) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0x6002:
      PPUBANK[ 4 ] = VROMPAGE( (byData*2+0) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( (byData*2+1) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0x6003:
      PPUBANK[ 6 ] = VROMPAGE( (byData*2+0) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( (byData*2+1) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    /* Set CPU Banks */
    case 0x7000:
      ROMBANK0 = ROMPAGE( byData % ( NesHeader.byRomSize << 1 ) ); 
      break;

    case 0x7001:
      ROMBANK1 = ROMPAGE( byData % ( NesHeader.byRomSize << 1 ) ); 
      break;
  }
}
