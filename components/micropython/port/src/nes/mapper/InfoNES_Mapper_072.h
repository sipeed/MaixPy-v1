/*===================================================================*/
/*                                                                   */
/*           Mapper 72 (Jaleco Early Mapper #0)                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 72                                             */
/*-------------------------------------------------------------------*/
void Map72_Init()
{
  /* Initialize Mapper */
  MapperInit = Map72_Init;

  /* Write to Mapper */
  MapperWrite = Map72_Write;

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

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 )
  {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 72 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map72_Write( WORD wAddr, BYTE byData )
{
  BYTE byBank = byData & 0x0f;

  if ( byData & 0x80 )
  {
    /* Set ROM Banks */
    byBank <<= 1;
    byBank %= ( NesHeader.byRomSize << 1 );
    ROMBANK0 = ROMPAGE( byBank );
    ROMBANK1 = ROMPAGE( byBank + 1 );
  } else 
  if ( byData & 0x40 )
  {
    /* Set PPU Banks */
    byBank <<= 3;
    byBank %= ( NesHeader.byVRomSize << 3 );
    PPUBANK[ 0 ] = VROMPAGE( byBank );
    PPUBANK[ 1 ] = VROMPAGE( byBank + 1 );
    PPUBANK[ 2 ] = VROMPAGE( byBank + 2 );
    PPUBANK[ 3 ] = VROMPAGE( byBank + 3 );
    PPUBANK[ 4 ] = VROMPAGE( byBank + 4 );
    PPUBANK[ 5 ] = VROMPAGE( byBank + 5 );
    PPUBANK[ 6 ] = VROMPAGE( byBank + 6 );
    PPUBANK[ 7 ] = VROMPAGE( byBank + 7 );
    InfoNES_SetupChr();
  }
}
