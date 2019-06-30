/*===================================================================*/
/*                                                                   */
/*                      Mapper 78 (74161/32 Irem)                    */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 78                                             */
/*-------------------------------------------------------------------*/
void Map78_Init()
{
  /* Initialize Mapper */
  MapperInit = Map78_Init;

  /* Write to Mapper */
  MapperWrite = Map78_Write;

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
/*  Mapper 78 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map78_Write( WORD wAddr, BYTE byData )
{
  BYTE byPrgBank = byData & 0x0f;
  BYTE byChrBank = ( byData & 0xf0 ) >> 4;

  /* Set ROM Banks */
  byPrgBank <<= 1;
  byPrgBank %= ( NesHeader.byRomSize << 1 );
  ROMBANK0 = ROMPAGE( byPrgBank );
  ROMBANK1 = ROMPAGE( byPrgBank + 1);

  /* Set PPU Banks */
  byChrBank <<= 3;
  byChrBank %= ( NesHeader.byVRomSize << 3 );
  PPUBANK[ 0 ] = VROMPAGE( byChrBank );  
  PPUBANK[ 1 ] = VROMPAGE( byChrBank + 1 ); 
  PPUBANK[ 2 ] = VROMPAGE( byChrBank + 2 ); 
  PPUBANK[ 3 ] = VROMPAGE( byChrBank + 3 );
  PPUBANK[ 4 ] = VROMPAGE( byChrBank + 4 ); 
  PPUBANK[ 5 ] = VROMPAGE( byChrBank + 5 ); 
  PPUBANK[ 6 ] = VROMPAGE( byChrBank + 6 ); 
  PPUBANK[ 7 ] = VROMPAGE( byChrBank + 7 ); 
  InfoNES_SetupChr();  

  /* Set Name Table Mirroring */
  if ( ( wAddr & 0xfe00 ) != 0xfe00 )
  {
    if ( byData & 0x08 )
    {
      InfoNES_Mirroring( 2 );
    } else {
      InfoNES_Mirroring( 3 );
    }
  }
}
