/*===================================================================*/
/*                                                                   */
/*                       Mapper 88 (Namco 118)                       */
/*                                                                   */
/*===================================================================*/

BYTE  Map88_Regs[ 1 ];

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 88                                             */
/*-------------------------------------------------------------------*/
void Map88_Init()
{
  /* Initialize Mapper */
  MapperInit = Map88_Init;

  /* Write to Mapper */
  MapperWrite = Map88_Write;

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
/*  Mapper 88 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map88_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x8000:
      Map88_Regs[ 0 ] = byData;
      break;

    case 0x8001:
      switch ( Map88_Regs[ 0 ] & 0x07 )
      { 
        case 0x00:
          PPUBANK[ 0 ] = VROMPAGE( ( ( byData & 0xfe ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
          PPUBANK[ 1 ] = VROMPAGE( ( ( byData & 0xfe ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
          InfoNES_SetupChr();
          break;

        case 0x01:
          PPUBANK[ 2 ] = VROMPAGE( ( ( byData & 0xfe ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
          PPUBANK[ 3 ] = VROMPAGE( ( ( byData & 0xfe ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
          InfoNES_SetupChr();
          break;

        case 0x02:
          PPUBANK[ 4 ] = VROMPAGE( ( byData + 0x40 ) % ( NesHeader.byVRomSize << 3 ) );
          InfoNES_SetupChr();
          break;

        case 0x03:
          PPUBANK[ 5 ] = VROMPAGE( ( byData + 0x40 ) % ( NesHeader.byVRomSize << 3 ) );
          InfoNES_SetupChr();
          break;

        case 0x04:
          PPUBANK[ 6 ] = VROMPAGE( ( byData + 0x40 ) % ( NesHeader.byVRomSize << 3 ) );
          InfoNES_SetupChr();
          break;

        case 0x05:
          PPUBANK[ 7 ] = VROMPAGE( ( byData + 0x40 ) % ( NesHeader.byVRomSize << 3 ) );
          InfoNES_SetupChr();
          break;

        case 0x06:
          ROMBANK0 = ROMPAGE( byData % ( NesHeader.byRomSize << 1 ) );
          break;

        case 0x07:
          ROMBANK1 = ROMPAGE( byData % ( NesHeader.byRomSize << 1 ) );
          break;
      }
      break;

    case 0xc000:
      if ( byData ) 
      {
        InfoNES_Mirroring( 2 );
      } else {
        InfoNES_Mirroring( 3 );
      }
      break;
  }
}
