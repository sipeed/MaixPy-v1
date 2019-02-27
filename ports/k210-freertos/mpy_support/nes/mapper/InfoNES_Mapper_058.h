/*===================================================================*/
/*                                                                   */
/*                           Mapper 58                               */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 58                                             */
/*-------------------------------------------------------------------*/
void Map58_Init()
{
  /* Initialize Mapper */
  MapperInit = Map58_Init;

  /* Write to Mapper */
  MapperWrite = Map58_Write;

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
  ROMBANK2 = ROMPAGE( 0 );
  ROMBANK3 = ROMPAGE( 1 );

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
/*  Mapper 58 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map58_Write( WORD wAddr, BYTE byData )
{
  if( wAddr & 0x40 ) {
    ROMBANK0 = ROMPAGE((((wAddr&0x07)<<1)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE((((wAddr&0x07)<<1)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE((((wAddr&0x07)<<1)+0) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE((((wAddr&0x07)<<1)+1) % (NesHeader.byRomSize<<1));
  } else {
    ROMBANK0 = ROMPAGE((((wAddr&0x06)<<1)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE((((wAddr&0x06)<<1)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE((((wAddr&0x06)<<1)+2) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE((((wAddr&0x06)<<1)+3) % (NesHeader.byRomSize<<1));
  }

  if ( NesHeader.byVRomSize > 0 ) {
    PPUBANK[ 0 ] = VROMPAGE(((wAddr&0x38)+0) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 1 ] = VROMPAGE(((wAddr&0x38)+1) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 2 ] = VROMPAGE(((wAddr&0x38)+2) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 3 ] = VROMPAGE(((wAddr&0x38)+3) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 4 ] = VROMPAGE(((wAddr&0x38)+4) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 5 ] = VROMPAGE(((wAddr&0x38)+5) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 6 ] = VROMPAGE(((wAddr&0x38)+6) % (NesHeader.byVRomSize<<3));
    PPUBANK[ 7 ] = VROMPAGE(((wAddr&0x38)+7) % (NesHeader.byVRomSize<<3));
    InfoNES_SetupChr();
  }

  if( byData & 0x02 ) InfoNES_Mirroring( 1 );
  else		      InfoNES_Mirroring( 0 );
}
