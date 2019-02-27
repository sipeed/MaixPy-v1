/*===================================================================*/
/*                                                                   */
/*                          Mapper 222                               */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 222                                            */
/*-------------------------------------------------------------------*/
void Map222_Init()
{
  /* Initialize Mapper */
  MapperInit = Map222_Init;

  /* Write to Mapper */
  MapperWrite = Map222_Write;

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
  if ( NesHeader.byVRomSize > 0 ) {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Set Mirroring */
  InfoNES_Mirroring( 1 );

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 222 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map222_Write( WORD wAddr, BYTE byData )
{
  switch( wAddr & 0xF003 ) {
  case	0x8000:
    ROMBANK0 = ROMPAGE( byData % ( NesHeader.byRomSize << 1 ) );
    break;
  case	0xA000:
    ROMBANK1 = ROMPAGE( byData % ( NesHeader.byRomSize << 1 ) );
    break;
  case	0xB000:
    PPUBANK[ 0 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0xB002:
    PPUBANK[ 1 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0xC000:
    PPUBANK[ 2 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0xC002:
    PPUBANK[ 3 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0xD000:
    PPUBANK[ 4 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0xD002:
    PPUBANK[ 5 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0xE000:
    PPUBANK[ 6 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0xE002:
    PPUBANK[ 7 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  }
}
