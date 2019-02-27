/*===================================================================*/
/*                                                                   */
/*                   Mapper 94 (74161/32 Capcom)                     */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 94                                             */
/*-------------------------------------------------------------------*/
void Map94_Init()
{
  /* Initialize Mapper */
  MapperInit = Map94_Init;

  /* Write to Mapper */
  MapperWrite = Map94_Write;

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

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 94 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map94_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr & 0xfff0 )
  {
    /* Set ROM Banks */
    case 0xff00:
      byData = ( byData & 0x1c ) >> 2;
      byData <<= 1;
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK0 = ROMPAGE( byData );
      ROMBANK1 = ROMPAGE( byData + 1 );
      break;
  }
}
