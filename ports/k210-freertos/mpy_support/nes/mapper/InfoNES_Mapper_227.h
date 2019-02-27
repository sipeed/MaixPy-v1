/*===================================================================*/
/*                                                                   */
/*                    Mapper 227 : 1200-in-1                         */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 227                                            */
/*-------------------------------------------------------------------*/
void Map227_Init()
{
  /* Initialize Mapper */
  MapperInit = Map227_Init;

  /* Write to Mapper */
  MapperWrite = Map227_Write;

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

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 227 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map227_Write( WORD wAddr, BYTE byData )
{
  BYTE	byBank = ((wAddr&0x0100)>>4)|((wAddr&0x0078)>>3);

  if( wAddr & 0x0001 ) {
    ROMBANK0 = ROMPAGE(((byBank<<2)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE(((byBank<<2)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE(((byBank<<2)+2) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE(((byBank<<2)+3) % (NesHeader.byRomSize<<1));
  } else {
    if( wAddr & 0x0004 ) {
      ROMBANK0 = ROMPAGE(((byBank<<2)+2) % (NesHeader.byRomSize<<1));
      ROMBANK1 = ROMPAGE(((byBank<<2)+3) % (NesHeader.byRomSize<<1));
      ROMBANK2 = ROMPAGE(((byBank<<2)+2) % (NesHeader.byRomSize<<1));
      ROMBANK3 = ROMPAGE(((byBank<<2)+3) % (NesHeader.byRomSize<<1));
    } else {
      ROMBANK0 = ROMPAGE(((byBank<<2)+0) % (NesHeader.byRomSize<<1));
      ROMBANK1 = ROMPAGE(((byBank<<2)+1) % (NesHeader.byRomSize<<1));
      ROMBANK2 = ROMPAGE(((byBank<<2)+0) % (NesHeader.byRomSize<<1));
      ROMBANK3 = ROMPAGE(((byBank<<2)+1) % (NesHeader.byRomSize<<1));
    }
  }
  
  if( !(wAddr & 0x0080) ) {
    if( wAddr & 0x0200 ) {
      ROMBANK2 = ROMPAGE((((byBank&0x1C)<<2)+14) % (NesHeader.byRomSize<<1));
      ROMBANK3 = ROMPAGE((((byBank&0x1C)<<2)+15) % (NesHeader.byRomSize<<1));
    } else {
      ROMBANK2 = ROMPAGE((((byBank&0x1C)<<2)+0) % (NesHeader.byRomSize<<1));
      ROMBANK3 = ROMPAGE((((byBank&0x1C)<<2)+1) % (NesHeader.byRomSize<<1));
    }
  }
  if( wAddr & 0x0002 ) {
    InfoNES_Mirroring( 0 );
  } else {
    InfoNES_Mirroring( 1 );
  }
}
