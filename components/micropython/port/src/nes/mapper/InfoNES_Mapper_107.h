/*===================================================================*/
/*                                                                   */
/*             Mapper 107 : Magic Dragon Mapper                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 107                                            */
/*-------------------------------------------------------------------*/
void Map107_Init()
{
  /* Initialize Mapper */
  MapperInit = Map107_Init;

  /* Write to Mapper */
  MapperWrite = Map107_Write;

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

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 107 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map107_Write( WORD wAddr, BYTE byData )
{
  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( ( (((byData>>1)&0x03)<<2) + 0 ) % ( NesHeader.byRomSize << 1 ) );
  ROMBANK1 = ROMPAGE( ( (((byData>>1)&0x03)<<2) + 1 ) % ( NesHeader.byRomSize << 1 ) );
  ROMBANK2 = ROMPAGE( ( (((byData>>1)&0x03)<<2) + 2 ) % ( NesHeader.byRomSize << 1 ) );
  ROMBANK3 = ROMPAGE( ( (((byData>>1)&0x03)<<2) + 3 ) % ( NesHeader.byRomSize << 1 ) );

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 ) {
    PPUBANK[ 0 ] = VROMPAGE( ( ((byData&0x07)<<3) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 1 ] = VROMPAGE( ( ((byData&0x07)<<3) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 2 ] = VROMPAGE( ( ((byData&0x07)<<3) + 2 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 3 ] = VROMPAGE( ( ((byData&0x07)<<3) + 3 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 4 ] = VROMPAGE( ( ((byData&0x07)<<3) + 4 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 5 ] = VROMPAGE( ( ((byData&0x07)<<3) + 5 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 6 ] = VROMPAGE( ( ((byData&0x07)<<3) + 6 ) % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 7 ] = VROMPAGE( ( ((byData&0x07)<<3) + 7 ) % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
  }
}
