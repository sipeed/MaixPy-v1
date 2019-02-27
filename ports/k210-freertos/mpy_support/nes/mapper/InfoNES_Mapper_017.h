/*===================================================================*/
/*                                                                   */
/*                 Mapper 17 (FFE F8 Series)                         */
/*                                                                   */
/*===================================================================*/

BYTE  Map17_IRQ_Enable;
NES_DWORD Map17_IRQ_Cnt;
NES_DWORD Map17_IRQ_Latch;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 17                                             */
/*-------------------------------------------------------------------*/
void Map17_Init()
{
  /* Initialize Mapper */
  MapperInit = Map17_Init;

  /* Write to Mapper */
  MapperWrite = Map0_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map17_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map17_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

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

  /* Initialize State Registers */
  Map17_IRQ_Enable = 0;
  Map17_IRQ_Cnt = 0;
  Map17_IRQ_Latch = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 17 Write to APU Function                                  */
/*-------------------------------------------------------------------*/
void Map17_Apu( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x42fe:
      if ( ( byData & 0x10 ) == 0 )
      {
        InfoNES_Mirroring( 3 );
      } else {
        InfoNES_Mirroring( 2 );
      }
      break;

    case 0x42ff:
      if ( ( byData & 0x10 ) == 0 )
      {
        InfoNES_Mirroring( 1 );
      } else {
        InfoNES_Mirroring( 0 );
      }
      break;

    case 0x4501:
      Map17_IRQ_Enable = 0;
      break;

    case 0x4502:
      Map17_IRQ_Latch = ( Map17_IRQ_Latch & 0xff00 ) | byData;
      break;

    case 0x4503:
      Map17_IRQ_Latch = ( Map17_IRQ_Latch & 0x00ff ) | ( (NES_DWORD)byData << 8 );
      Map17_IRQ_Cnt = Map17_IRQ_Latch;
      Map17_IRQ_Enable = 1;
      break;

    case 0x4504:
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK0 = ROMPAGE( byData );
      break;

    case 0x4505:
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK1 = ROMPAGE( byData );
      break;

    case 0x4506:
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK2 = ROMPAGE( byData );
      break;

    case 0x4507:
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK3 = ROMPAGE( byData );
      break;

    case 0x4510:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 0 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0x4511:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 1 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0x4512:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 2 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0x4513:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 3 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0x4514:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 4 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0x4515:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 5 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0x4516:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 6 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0x4517:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 7 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 17 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map17_HSync()
{
  if ( Map17_IRQ_Enable )
  {
    if ( Map17_IRQ_Cnt >= 0xffff - 113 )
    {
      IRQ_REQ;
      Map17_IRQ_Cnt = 0;
      Map17_IRQ_Enable = 0;
    } else {
      Map17_IRQ_Cnt += 113;
    }
  }
}
