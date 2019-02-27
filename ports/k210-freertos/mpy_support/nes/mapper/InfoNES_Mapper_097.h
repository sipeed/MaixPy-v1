/*===================================================================*/
/*                                                                   */
/*                  Mapper 97 (74161/32 Irem)                        */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 97                                             */
/*-------------------------------------------------------------------*/
void Map97_Init()
{
  /* Initialize Mapper */
  MapperInit = Map97_Init;

  /* Write to Mapper */
  MapperWrite = Map97_Write;

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
  ROMBANK0 = ROMLASTPAGE( 1 );
  ROMBANK1 = ROMLASTPAGE( 0 );
  ROMBANK2 = ROMPAGE( 0 );
  ROMBANK3 = ROMPAGE( 1 );

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
/*  Mapper 97 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map97_Write( WORD wAddr, BYTE byData )
{
  /* Set ROM Banks */
  if ( wAddr < 0xc000 )
  {
    BYTE byPrgBank = byData & 0x0f;

    byPrgBank <<= 1;
    byPrgBank %= ( NesHeader.byRomSize << 1 );

    ROMBANK2 = ROMPAGE( byPrgBank );
    ROMBANK3 = ROMPAGE( byPrgBank + 1 );

    if ( ( byData & 0x80 ) == 0 )
    {
      InfoNES_Mirroring( 0 );
    } else {
      InfoNES_Mirroring( 1 );
    }
  }
}
