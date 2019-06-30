/*===================================================================*/
/*                                                                   */
/*                  Mapper 228 : Action 52                           */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 228                                            */
/*-------------------------------------------------------------------*/
void Map228_Init()
{
  /* Initialize Mapper */
  MapperInit = Map228_Init;

  /* Write to Mapper */
  MapperWrite = Map228_Write;

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
/*  Mapper 228 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map228_Write( WORD wAddr, BYTE byData )
{
  BYTE	byPrg = (wAddr&0x0780)>>7;

  switch( (wAddr&0x1800)>>11 ) {
  case	1:
    byPrg |= 0x10;
    break;
  case	3:
    byPrg |= 0x20;
    break;
  }

  if( wAddr & 0x0020 ) {
    byPrg <<= 1;
    if( wAddr & 0x0040 ) {
      byPrg++;
    }
    ROMBANK0 = ROMPAGE(((byPrg<<2)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE(((byPrg<<2)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE(((byPrg<<2)+0) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE(((byPrg<<2)+1) % (NesHeader.byRomSize<<1));
  } else {
    ROMBANK0 = ROMPAGE(((byPrg<<2)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE(((byPrg<<2)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE(((byPrg<<2)+2) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE(((byPrg<<2)+3) % (NesHeader.byRomSize<<1));
  }

  BYTE byChr = ((wAddr&0x000F)<<2)|(byData&0x03);

  PPUBANK[ 0 ] = VROMPAGE(((byChr<<3)+0) % (NesHeader.byVRomSize<<3)); 
  PPUBANK[ 1 ] = VROMPAGE(((byChr<<3)+1) % (NesHeader.byVRomSize<<3)); 
  PPUBANK[ 2 ] = VROMPAGE(((byChr<<3)+2) % (NesHeader.byVRomSize<<3)); 
  PPUBANK[ 3 ] = VROMPAGE(((byChr<<3)+3) % (NesHeader.byVRomSize<<3)); 
  PPUBANK[ 4 ] = VROMPAGE(((byChr<<3)+4) % (NesHeader.byVRomSize<<3)); 
  PPUBANK[ 5 ] = VROMPAGE(((byChr<<3)+5) % (NesHeader.byVRomSize<<3)); 
  PPUBANK[ 6 ] = VROMPAGE(((byChr<<3)+6) % (NesHeader.byVRomSize<<3)); 
  PPUBANK[ 7 ] = VROMPAGE(((byChr<<3)+7) % (NesHeader.byVRomSize<<3)); 
  InfoNES_SetupChr();
  
  if( wAddr & 0x2000 ) {
    InfoNES_Mirroring( 0 );
  } else {
    InfoNES_Mirroring( 1 );
  }
}
