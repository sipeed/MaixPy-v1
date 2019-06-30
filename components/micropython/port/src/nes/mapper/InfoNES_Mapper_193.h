/*===================================================================*/
/*                                                                   */
/*         Mapper 193 : MEGA SOFT (NTDEC) : Fighting Hero            */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 193                                            */
/*-------------------------------------------------------------------*/
void Map193_Init()
{
  /* Initialize Mapper */
  MapperInit = Map193_Init;

  /* Write to Mapper */
  MapperWrite = Map0_Write;

  /* Write to SRAM */
  MapperSram = Map193_Sram;

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
  ROMBANK0 = ROMPAGE( (NesHeader.byRomSize<<1) - 4 );
  ROMBANK1 = ROMPAGE( (NesHeader.byRomSize<<1) - 3 );
  ROMBANK2 = ROMPAGE( (NesHeader.byRomSize<<1) - 2 );
  ROMBANK3 = ROMPAGE( (NesHeader.byRomSize<<1) - 1 );

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
/*  Mapper 193 Write to SRAM Function                                */
/*-------------------------------------------------------------------*/
void Map193_Sram( WORD wAddr, BYTE byData )
{
  switch( wAddr ) {
  case	0x6000:
    PPUBANK[ 0 ] = VROMPAGE( ((byData&0xfc) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 1 ] = VROMPAGE( ((byData&0xfc) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 2 ] = VROMPAGE( ((byData&0xfc) + 2 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 3 ] = VROMPAGE( ((byData&0xfc) + 3 ) % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0x6001:
    PPUBANK[ 4 ] = VROMPAGE( ( byData + 0 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 5 ] = VROMPAGE( ( byData + 1 ) % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0x6002:
    PPUBANK[ 6 ] = VROMPAGE( ( byData + 0 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 7 ] = VROMPAGE( ( byData + 1 ) % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
    break;
  case	0x6003:
    ROMBANK0 = ROMPAGE( ((byData<<2) + 0 ) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK1 = ROMPAGE( ((byData<<2) + 1 ) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMPAGE( ((byData<<2) + 2 ) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK3 = ROMPAGE( ((byData<<2) + 3 ) % ( NesHeader.byRomSize << 1 ) );
    break;
  }
}
