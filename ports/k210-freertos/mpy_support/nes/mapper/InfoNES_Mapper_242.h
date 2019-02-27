/*===================================================================*/
/*                                                                   */
/*                Mapper 242 : Wai Xing Zhan Shi                     */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 242                                            */
/*-------------------------------------------------------------------*/
void Map242_Init()
{
  /* Initialize Mapper */
  MapperInit = Map242_Init;

  /* Write to Mapper */
  MapperWrite = Map242_Write;

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
/*  Mapper 242 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map242_Write( WORD wAddr, BYTE byData )
{
  if( wAddr & 0x01 ) {
    /* Set ROM Banks */
    ROMBANK0 = ROMPAGE((((wAddr&0xF8)>>1)+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE((((wAddr&0xF8)>>1)+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE((((wAddr&0xF8)>>1)+2) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE((((wAddr&0xF8)>>1)+3) % (NesHeader.byRomSize<<1));
  }
}
