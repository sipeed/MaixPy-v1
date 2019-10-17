/*===================================================================*/
/*                                                                   */
/*                    Mapper 19 (Namcot 106)                         */
/*                                                                   */
/*===================================================================*/

BYTE  Map19_Chr_Ram[ 0x2000 ];
BYTE  Map19_Regs[ 2 ];

BYTE  Map19_IRQ_Enable;
NES_DWORD Map19_IRQ_Cnt;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 19                                             */
/*-------------------------------------------------------------------*/
void Map19_Init()
{
  /* Initialize Mapper */
  MapperInit = Map19_Init;

  /* Write to Mapper */
  MapperWrite = Map19_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map19_Apu;

  /* Read from APU */
  MapperReadApu = Map19_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map19_HSync;

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
    NES_DWORD dwLastPage = (NES_DWORD)NesHeader.byVRomSize << 3;
    PPUBANK[ 0 ] = VROMPAGE( dwLastPage - 8 );
    PPUBANK[ 1 ] = VROMPAGE( dwLastPage - 7 );
    PPUBANK[ 2 ] = VROMPAGE( dwLastPage - 6 );
    PPUBANK[ 3 ] = VROMPAGE( dwLastPage - 5 );
    PPUBANK[ 4 ] = VROMPAGE( dwLastPage - 4 );
    PPUBANK[ 5 ] = VROMPAGE( dwLastPage - 3 );
    PPUBANK[ 6 ] = VROMPAGE( dwLastPage - 2 );
    PPUBANK[ 7 ] = VROMPAGE( dwLastPage - 1 );
    InfoNES_SetupChr();
  }

  /* Initialize State Register */
  Map19_Regs[ 0 ] = 0x00;
  Map19_Regs[ 1 ] = 0x00;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 19 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map19_Write( WORD wAddr, BYTE byData )
{
  /* Set PPU Banks */
  switch ( wAddr & 0xf800 )
  {
    case 0x8000:  /* $8000-87ff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ 0 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ 0 ] = Map19_VROMPAGE( 0 );
      }
      InfoNES_SetupChr();
      break;

    case 0x8800:  /* $8800-8fff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ 1 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ 1 ] = Map19_VROMPAGE( 1 );
      }
      InfoNES_SetupChr();
      break;

    case 0x9000:  /* $9000-97ff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ 2 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ 2 ] = Map19_VROMPAGE( 2 );
      }
      InfoNES_SetupChr();
      break;

    case 0x9800:  /* $9800-9fff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ 3 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ 3 ] = Map19_VROMPAGE( 3 );
      }
      InfoNES_SetupChr();
      break;

    case 0xa000:  /* $a000-a7ff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ 4 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ 4 ] = Map19_VROMPAGE( 4 );
      }
      InfoNES_SetupChr();
      break;

    case 0xa800:  /* $a800-afff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ 5 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ 5 ] = Map19_VROMPAGE( 5 );
      }
      InfoNES_SetupChr();
      break;

    case 0xb000:  /* $b000-b7ff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ 6 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ 6 ] = Map19_VROMPAGE( 6 );
      }
      InfoNES_SetupChr();
      break;

    case 0xb800:  /* $b800-bfff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ 7 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ 7 ] = Map19_VROMPAGE( 7 );
      }
      InfoNES_SetupChr();
      break;

    case 0xc000:  /* $c000-c7ff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ NAME_TABLE0 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ NAME_TABLE0 ] = VRAMPAGE( byData & 0x01 );
      }
      break;

    case 0xc800:  /* $c800-cfff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ NAME_TABLE1 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ NAME_TABLE1 ] = VRAMPAGE( byData & 0x01 );
      }
      break;

    case 0xd000:  /* $d000-d7ff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ NAME_TABLE2 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ NAME_TABLE2 ] = VRAMPAGE( byData & 0x01 );
      }
      break;

    case 0xd800:  /* $d800-dfff */
      if ( byData < 0xe0 || Map19_Regs[ 0 ] == 1 )
      {
        byData %= ( NesHeader.byVRomSize << 3 );
        PPUBANK[ NAME_TABLE3 ] = VROMPAGE( byData );
      } else {
        PPUBANK[ NAME_TABLE3 ] = VRAMPAGE( byData & 0x01 );
      }
      break;

    case 0xe000:  /* $e000-e7ff */
      byData &= 0x3f;
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK0 = ROMPAGE( byData );
      break;

    case 0xe800:  /* $e800-efff */
      Map19_Regs[ 0 ] = ( byData & 0x40 ) >> 6;
      Map19_Regs[ 1 ] = ( byData & 0x80 ) >> 7;

      byData &= 0x3f;
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK1 = ROMPAGE( byData );
      break;

    case 0xf000:  /* $f000-f7ff */
      byData &= 0x3f;
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK2 = ROMPAGE( byData );
      break;

    case 0xf800:  /* $e800-efff */
      if ( wAddr == 0xf800 )
      {
        // Extra Sound
      }
      break;    
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 19 Write to APU Function                                  */
/*-------------------------------------------------------------------*/
void Map19_Apu( WORD wAddr, BYTE byData )
{
  switch ( wAddr & 0xf800 )
  {
    case 0x4800:
      if ( wAddr == 0x4800 )
      {
        // Extra Sound
      }
      break;

    case 0x5000:  /* $5000-57ff */
      Map19_IRQ_Cnt = ( Map19_IRQ_Cnt & 0xff00 ) | byData;
      break;

    case 0x5800:  /* $5800-5fff */
      Map19_IRQ_Cnt = ( Map19_IRQ_Cnt & 0x00ff ) | ( (NES_DWORD)( byData & 0x7f ) << 8 );
      Map19_IRQ_Enable = ( byData & 0x80 ) >> 7;
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 19 Read from APU Function                                 */
/*-------------------------------------------------------------------*/
BYTE Map19_ReadApu( WORD wAddr )
{
  switch ( wAddr & 0xf800 )
  {
    case 0x4800:
      if ( wAddr == 0x4800 )
      {
        // Extra Sound
      }
      return (BYTE)( wAddr >> 8 );

    case 0x5000:  /* $5000-57ff */
      return (BYTE)(Map19_IRQ_Cnt & 0x00ff );

    case 0x5800:  /* $5800-5fff */
      return (BYTE)( (Map19_IRQ_Cnt & 0x7f00) >> 8 );

    default:
      return (BYTE)( wAddr >> 8 );
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 19 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map19_HSync()
{
/*
 *  Callback at HSync
 *
 */
  BYTE Map19_IRQ_Timing = 113;

  if ( Map19_IRQ_Enable )
  {
    if ( Map19_IRQ_Cnt >= (NES_DWORD)(0x7fff - Map19_IRQ_Timing) )
    {
      Map19_IRQ_Cnt = 0x7fff;
      IRQ_REQ;
    } else {
      Map19_IRQ_Cnt += Map19_IRQ_Timing;
    }
  }
}
