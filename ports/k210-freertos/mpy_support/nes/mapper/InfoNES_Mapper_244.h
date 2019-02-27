/*===================================================================*/
/*                                                                   */
/*                          Mapper 244                               */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 244                                            */
/*-------------------------------------------------------------------*/
void Map244_Init()
{
  /* Initialize Mapper */
  MapperInit = Map244_Init;

  /* Write to Mapper */
  MapperWrite = Map244_Write;

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

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 244 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map244_Write( WORD wAddr, BYTE byData )
{
  if( wAddr>=0x8065 && wAddr<=0x80A4 ) {
    /* Set ROM Banks */
    ROMBANK0 = ROMPAGE(((((wAddr-0x8065)&0x3)<<2)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE(((((wAddr-0x8065)&0x3)<<2)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE(((((wAddr-0x8065)&0x3)<<2)+2) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE(((((wAddr-0x8065)&0x3)<<2)+3) % (NesHeader.byRomSize<<1));
  }
  
  if( wAddr>=0x80A5 && wAddr<=0x80E4 ) {
    /* Set ROM Banks */
    ROMBANK0 = ROMPAGE(((((wAddr-0x80A5)&0x7)<<2)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE(((((wAddr-0x80A5)&0x7)<<2)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE(((((wAddr-0x80A5)&0x7)<<2)+2) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE(((((wAddr-0x80A5)&0x7)<<2)+3) % (NesHeader.byRomSize<<1));
  }
}
