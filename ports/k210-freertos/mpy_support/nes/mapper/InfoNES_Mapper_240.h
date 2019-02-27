/*===================================================================*/
/*                                                                   */
/*                  Mapper 240 : Gen Ke Le Zhuan                     */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 240                                            */
/*-------------------------------------------------------------------*/
void Map240_Init()
{
  /* Initialize Mapper */
  MapperInit = Map240_Init;

  /* Write to Mapper */
  MapperWrite = Map0_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map240_Apu;

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
/*  Mapper 240 Write to APU Function                                 */
/*-------------------------------------------------------------------*/
void Map240_Apu( WORD wAddr, BYTE byData )
{
  if( wAddr>=0x4020 && wAddr<0x6000 ) {
    /* Set ROM Banks */
    ROMBANK0 = ROMPAGE((((byData&0xF0)>>2)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE((((byData&0xF0)>>2)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE((((byData&0xF0)>>2)+2) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE((((byData&0xF0)>>2)+3) % (NesHeader.byRomSize<<1));

    /* Set PPU Banks */
    PPUBANK[ 0 ] = VROMPAGE((((byData&0x0F)<<3)+0) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 1 ] = VROMPAGE((((byData&0x0F)<<3)+1) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 2 ] = VROMPAGE((((byData&0x0F)<<3)+2) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 3 ] = VROMPAGE((((byData&0x0F)<<3)+3) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 4 ] = VROMPAGE((((byData&0x0F)<<3)+4) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 5 ] = VROMPAGE((((byData&0x0F)<<3)+5) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 6 ] = VROMPAGE((((byData&0x0F)<<3)+6) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 7 ] = VROMPAGE((((byData&0x0F)<<3)+7) % (NesHeader.byVRomSize<<3));
    InfoNES_SetupChr();
  }
}
