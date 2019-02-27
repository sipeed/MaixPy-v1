/*===================================================================*/
/*                                                                   */
/*                   Mapper 133 : SACHEN CHEN                        */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 133                                            */
/*-------------------------------------------------------------------*/
void Map133_Init()
{
  /* Initialize Mapper */
  MapperInit = Map133_Init;

  /* Write to Mapper */
  MapperWrite = Map0_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map133_Apu;

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
/*  Mapper 133 Write to APU Function                                 */
/*-------------------------------------------------------------------*/
void Map133_Apu( WORD wAddr, BYTE byData )
{
  if ( wAddr == 0x4120 ) {
    /* Set ROM Banks */
    ROMBANK0 = ROMPAGE( ((byData&0x04) + 0 ) % (NesHeader.byRomSize << 1) );
    ROMBANK1 = ROMPAGE( ((byData&0x04) + 1 ) % (NesHeader.byRomSize << 1) );
    ROMBANK2 = ROMPAGE( ((byData&0x04) + 2 ) % (NesHeader.byRomSize << 1) );
    ROMBANK3 = ROMPAGE( ((byData&0x04) + 3 ) % (NesHeader.byRomSize << 1) );
  
    /* Set PPU Banks */
    PPUBANK[ 0 ] = VROMPAGE( (((byData&0x03)<<3) + 0) % (NesHeader.byVRomSize << 3) );
    PPUBANK[ 1 ] = VROMPAGE( (((byData&0x03)<<3) + 1) % (NesHeader.byVRomSize << 3) );
    PPUBANK[ 2 ] = VROMPAGE( (((byData&0x03)<<3) + 2) % (NesHeader.byVRomSize << 3) );
    PPUBANK[ 3 ] = VROMPAGE( (((byData&0x03)<<3) + 3) % (NesHeader.byVRomSize << 3) );
    PPUBANK[ 4 ] = VROMPAGE( (((byData&0x03)<<3) + 4) % (NesHeader.byVRomSize << 3) );
    PPUBANK[ 5 ] = VROMPAGE( (((byData&0x03)<<3) + 5) % (NesHeader.byVRomSize << 3) );
    PPUBANK[ 6 ] = VROMPAGE( (((byData&0x03)<<3) + 6) % (NesHeader.byVRomSize << 3) );
    PPUBANK[ 7 ] = VROMPAGE( (((byData&0x03)<<3) + 7) % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
  }
  //Map133_Wram[ wAddr & 0x1fff ] = byData;
}
