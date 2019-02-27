/*===================================================================*/
/*                                                                   */
/*            Mapper 71 (Camerica Custom Mapper)                     */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 71                                             */
/*-------------------------------------------------------------------*/
void Map71_Init()
{
  /* Initialize Mapper */
  MapperInit = Map71_Init;

  /* Write to Mapper */
  MapperWrite = Map71_Write;

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
/*  Mapper 71 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map71_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr & 0xf000 )
  {
    case 0x9000:
      if ( byData & 0x10 )
      {
        InfoNES_Mirroring( 2 );
      } else {
        InfoNES_Mirroring( 3 );
      }
      break;

    /* Set ROM Banks */
    case 0xc000:
    case 0xd000:
    case 0xe000:
    case 0xf000:
      ROMBANK0 = ROMPAGE( ( ( byData << 1 ) + 0 ) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK1 = ROMPAGE( ( ( byData << 1 ) + 1 ) % ( NesHeader.byRomSize << 1 ) );
      break;
  }
}
