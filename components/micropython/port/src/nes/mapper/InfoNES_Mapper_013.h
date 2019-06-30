/*===================================================================*/
/*                                                                   */
/*                        Mapper 13 : CPROM                          */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 13                                             */
/*-------------------------------------------------------------------*/
void Map13_Init()
{
  /* Initialize Mapper */
  MapperInit = Map13_Init;

  /* Write to Mapper */
  MapperWrite = Map13_Write;

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
  PPUBANK[ 0 ] = CRAMPAGE( 0 );
  PPUBANK[ 1 ] = CRAMPAGE( 1 );
  PPUBANK[ 2 ] = CRAMPAGE( 2 );
  PPUBANK[ 3 ] = CRAMPAGE( 3 );
  PPUBANK[ 4 ] = CRAMPAGE( 0 );
  PPUBANK[ 5 ] = CRAMPAGE( 1 );
  PPUBANK[ 6 ] = CRAMPAGE( 2 );
  PPUBANK[ 7 ] = CRAMPAGE( 3 );
  InfoNES_SetupChr();

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 13 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map13_Write( WORD wAddr, BYTE byData )
{
  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE((((byData&0x30)>>2)+0) % (NesHeader.byRomSize<<1));
  ROMBANK1 = ROMPAGE((((byData&0x30)>>2)+1) % (NesHeader.byRomSize<<1));
  ROMBANK2 = ROMPAGE((((byData&0x30)>>2)+2) % (NesHeader.byRomSize<<1));
  ROMBANK3 = ROMPAGE((((byData&0x30)>>2)+3) % (NesHeader.byRomSize<<1));

  /* Set PPU Banks */
  PPUBANK[ 4 ] = CRAMPAGE(((byData&0x03)<<2)+0);
  PPUBANK[ 5 ] = CRAMPAGE(((byData&0x03)<<2)+1);
  PPUBANK[ 6 ] = CRAMPAGE(((byData&0x03)<<2)+2);
  PPUBANK[ 7 ] = CRAMPAGE(((byData&0x03)<<2)+3);
  InfoNES_SetupChr();
}
