/*===================================================================*/
/*                                                                   */
/*                         Mapper 185  (Tecmo)                       */
/*                                                                   */
/*===================================================================*/

BYTE Map185_Dummy_Chr_Rom[ 0x400 ];

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 185                                            */
/*-------------------------------------------------------------------*/
void Map185_Init()
{
  /* Initialize Mapper */
  MapperInit = Map185_Init;

  /* Write to Mapper */
  MapperWrite = Map185_Write;

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

  /* Initialize Dummy VROM */
  for ( int nPage = 0; nPage < 0x400; nPage++ )
  {
    Map185_Dummy_Chr_Rom[ nPage ] = 0xff;
  }

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 185 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map185_Write( WORD wAddr, BYTE byData )
{
  /* Set PPU Banks */ 
  if ( byData & 0x03 )
  {
    PPUBANK[ 0 ] = VROMPAGE( 0 );
    PPUBANK[ 1 ] = VROMPAGE( 1 );
    PPUBANK[ 2 ] = VROMPAGE( 2 );
    PPUBANK[ 3 ] = VROMPAGE( 3 );
    PPUBANK[ 4 ] = VROMPAGE( 4 );
    PPUBANK[ 5 ] = VROMPAGE( 5 );
    PPUBANK[ 6 ] = VROMPAGE( 6 );
    PPUBANK[ 7 ] = VROMPAGE( 7 );
    InfoNES_SetupChr();
  } else {
    PPUBANK[ 0 ] = Map185_Dummy_Chr_Rom;
    PPUBANK[ 1 ] = Map185_Dummy_Chr_Rom;
    PPUBANK[ 2 ] = Map185_Dummy_Chr_Rom;
    PPUBANK[ 3 ] = Map185_Dummy_Chr_Rom;
    PPUBANK[ 4 ] = Map185_Dummy_Chr_Rom;
    PPUBANK[ 5 ] = Map185_Dummy_Chr_Rom;
    PPUBANK[ 6 ] = Map185_Dummy_Chr_Rom;
    PPUBANK[ 7 ] = Map185_Dummy_Chr_Rom;
    InfoNES_SetupChr();
  }
}
