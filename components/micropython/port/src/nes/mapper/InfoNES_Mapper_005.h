/*===================================================================*/
/*                                                                   */
/*                        Mapper 5 (MMC5)                            */
/*                                                                   */
/*===================================================================*/

BYTE Map5_Wram[ 0x2000 * 8 ];
BYTE Map5_Ex_Ram[ 0x400 ]; 
BYTE Map5_Ex_Vram[ 0x400 ];
BYTE Map5_Ex_Nam[ 0x400 ];

BYTE Map5_Prg_Reg[ 8 ];
BYTE Map5_Wram_Reg[ 8 ];
BYTE Map5_Chr_Reg[ 8 ][ 2 ];

BYTE Map5_IRQ_Enable;
BYTE Map5_IRQ_Status;
BYTE Map5_IRQ_Line;

NES_DWORD Map5_Value0;
NES_DWORD Map5_Value1;

BYTE Map5_Wram_Protect0;
BYTE Map5_Wram_Protect1;
BYTE Map5_Prg_Size;
BYTE Map5_Chr_Size;
BYTE Map5_Gfx_Mode;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 5                                              */
/*-------------------------------------------------------------------*/
void Map5_Init()
{
  int nPage;

  /* Initialize Mapper */
  MapperInit = Map5_Init;

  /* Write to Mapper */
  MapperWrite = Map5_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map5_Apu;

  /* Read from APU */
  MapperReadApu = Map5_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map5_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map5_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Set ROM Banks */
  ROMBANK0 = ROMLASTPAGE( 0 );
  ROMBANK1 = ROMLASTPAGE( 0 );
  ROMBANK2 = ROMLASTPAGE( 0 );
  ROMBANK3 = ROMLASTPAGE( 0 );

  /* Set PPU Banks */
  for ( nPage = 0; nPage < 8; ++nPage )
    PPUBANK[ nPage ] = VROMPAGE( nPage );
  InfoNES_SetupChr();

  /* Initialize State Registers */
  for ( nPage = 4; nPage < 8; ++nPage )
  {
    Map5_Prg_Reg[ nPage ] = ( NesHeader.byRomSize << 1 ) - 1;
    Map5_Wram_Reg[ nPage ] = 0xff;
  }
  Map5_Wram_Reg[ 3 ] = 0xff;

  for ( BYTE byPage = 4; byPage < 8; ++byPage )
  {
    Map5_Chr_Reg[ byPage ][ 0 ] = byPage;
    Map5_Chr_Reg[ byPage ][ 1 ] = ( byPage & 0x03 ) + 4;
  }

  InfoNES_MemorySet( Map5_Wram, 0x00, sizeof( Map5_Wram ) );
  InfoNES_MemorySet( Map5_Ex_Ram, 0x00, sizeof( Map5_Ex_Ram ) );
  InfoNES_MemorySet( Map5_Ex_Vram, 0x00, sizeof( Map5_Ex_Vram ) );
  InfoNES_MemorySet( Map5_Ex_Nam, 0x00, sizeof( Map5_Ex_Nam ) );

  Map5_Prg_Size = 3;
  Map5_Wram_Protect0 = 0;
  Map5_Wram_Protect1 = 0;
  Map5_Chr_Size = 3;
  Map5_Gfx_Mode = 0;

  Map5_IRQ_Enable = 0;
  Map5_IRQ_Status = 0;
  Map5_IRQ_Line = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 5 Read from APU Function                                  */
/*-------------------------------------------------------------------*/
BYTE Map5_ReadApu( WORD wAddr )
{
  BYTE byRet = (BYTE)( wAddr >> 8 );

  switch ( wAddr )
  {
    case 0x5204:
      byRet = Map5_IRQ_Status;
      Map5_IRQ_Status = 0;
      break;

    case 0x5205:
      byRet = (BYTE)( ( Map5_Value0 * Map5_Value1 ) & 0x00ff );
      break;

    case 0x5206:
      byRet = (BYTE)( ( ( Map5_Value0 * Map5_Value1 ) & 0xff00 ) >> 8 );
      break;

    default:
      if ( 0x5c00 <= wAddr && wAddr <= 0x5fff )
      {
        byRet = Map5_Ex_Ram[ wAddr - 0x5c00 ];
      }
      break;
  }
  return byRet;
}

/*-------------------------------------------------------------------*/
/*  Mapper 5 Write to APU Function                                   */
/*-------------------------------------------------------------------*/
void Map5_Apu( WORD wAddr, BYTE byData )
{
  int nPage;

  switch ( wAddr )
  {
    case 0x5100:
      Map5_Prg_Size = byData & 0x03;
      break;

    case 0x5101:
      Map5_Chr_Size = byData & 0x03;
      break;

    case 0x5102:
      Map5_Wram_Protect0 = byData & 0x03;
      break;

    case 0x5103:
      Map5_Wram_Protect1 = byData & 0x03;
      break;

    case 0x5104:
      Map5_Gfx_Mode = byData & 0x03;
      break;

    case 0x5105:
      for ( nPage = 0; nPage < 4; nPage++ )
      {
        BYTE byNamReg;
        
        byNamReg = byData & 0x03;
        byData = byData >> 2;

        switch ( byNamReg )
        {
          case 0:
#if 1
            PPUBANK[ nPage + 8 ] = VRAMPAGE( 0 );
#else
            PPUBANK[ nPage + 8 ] = CRAMPAGE( 8 );
#endif
            break;
          case 1:
#if 1
            PPUBANK[ nPage + 8 ] = VRAMPAGE( 1 );
#else
            PPUBANK[ nPage + 8 ] = CRAMPAGE( 9 );
#endif
            break;
          case 2:
            PPUBANK[ nPage + 8 ] = Map5_Ex_Vram;
            break;
          case 3:
            PPUBANK[ nPage + 8 ] = Map5_Ex_Nam;
            break;
        }
      }
      break;

    case 0x5106:
      InfoNES_MemorySet( Map5_Ex_Nam, byData, 0x3c0 );
      break;

    case 0x5107:
      byData &= 0x03;
      byData = byData | ( byData << 2 ) | ( byData << 4 ) | ( byData << 6 );
      InfoNES_MemorySet( &( Map5_Ex_Nam[ 0x3c0 ] ), byData, 0x400 - 0x3c0 );
      break;

    case 0x5113:
      Map5_Wram_Reg[ 3 ] = byData & 0x07;
      SRAMBANK = Map5_ROMPAGE( byData & 0x07 );
      break;

    case 0x5114:
    case 0x5115:
    case 0x5116:
    case 0x5117:
      Map5_Prg_Reg[ wAddr & 0x07 ] = byData;
      Map5_Sync_Prg_Banks();
      break;

    case 0x5120:
    case 0x5121:
    case 0x5122:
    case 0x5123:
    case 0x5124:
    case 0x5125:
    case 0x5126:
    case 0x5127:
      Map5_Chr_Reg[ wAddr & 0x07 ][ 0 ] = byData;
      Map5_Sync_Prg_Banks();
      break;

    case 0x5128:
    case 0x5129:
    case 0x512a:
    case 0x512b:
      Map5_Chr_Reg[ ( wAddr & 0x03 ) + 0 ][ 1 ] = byData;
      Map5_Chr_Reg[ ( wAddr & 0x03 ) + 4 ][ 1 ] = byData;
      break;

    case 0x5200:
    case 0x5201:
    case 0x5202:
      /* Nothing to do */
      break;

    case 0x5203:
      if ( Map5_IRQ_Line >= 0x40 )
      {
        Map5_IRQ_Line = byData;
      } else {
        Map5_IRQ_Line += byData;
      }
      break;

    case 0x5204:
      Map5_IRQ_Enable = byData;
      break;

    case 0x5205:
      Map5_Value0 = byData;
      break;

    case 0x5206:
      Map5_Value1 = byData;
      break;

    default:
      if ( 0x5000 <= wAddr && wAddr <= 0x5015 )
      {
        /* Extra Sound */
      } else 
      if ( 0x5c00 <= wAddr && wAddr <= 0x5fff )
      {
        switch ( Map5_Gfx_Mode )
        {
          case 0:
            Map5_Ex_Vram[ wAddr - 0x5c00 ] = byData;
            break;
          case 2:
            Map5_Ex_Ram[ wAddr - 0x5c00 ] = byData;
            break;
        }
      }
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 5 Write to SRAM Function                                  */
/*-------------------------------------------------------------------*/
void Map5_Sram( WORD wAddr, BYTE byData )
{
  if ( Map5_Wram_Protect0 == 0x02 && Map5_Wram_Protect1 == 0x01 )
  {
    if ( Map5_Wram_Reg[ 3 ] != 0xff )
    {
      Map5_Wram[ 0x2000 * Map5_Wram_Reg[ 3 ] + ( wAddr - 0x6000) ] = byData;
    }
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 5 Write Function                                          */
/*-------------------------------------------------------------------*/
void Map5_Write( WORD wAddr, BYTE byData )
{
  if ( Map5_Wram_Protect0 == 0x02 && Map5_Wram_Protect1 == 0x01 )
  {
    switch ( wAddr & 0xe000 )
    {
      case 0x8000:      /* $8000-$9fff */
        if ( Map5_Wram_Reg[ 4 ] != 0xff )
        {
          Map5_Wram[ 0x2000 * Map5_Wram_Reg[ 4 ] + ( wAddr - 0x8000) ] = byData;
        }
        break;

      case 0xa000:      /* $a000-$bfff */
        if ( Map5_Wram_Reg[ 5 ] != 0xff )
        {
          Map5_Wram[ 0x2000 * Map5_Wram_Reg[ 5 ] + ( wAddr - 0xa000) ] = byData;
        }
        break;

      case 0xc000:      /* $c000-$dfff */
        if ( Map5_Wram_Reg[ 6 ] != 0xff )
        {
          Map5_Wram[ 0x2000 * Map5_Wram_Reg[ 6 ] + ( wAddr - 0xc000) ] = byData;
        }
        break;
    }
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 5 H-Sync Function                                         */
/*-------------------------------------------------------------------*/
void Map5_HSync()
{
  if ( PPU_Scanline <= 240 )
  {
    if ( PPU_Scanline == Map5_IRQ_Line )
    {
      Map5_IRQ_Status |= 0x80;

      if ( Map5_IRQ_Enable && Map5_IRQ_Line < 0xf0 )
      {
        IRQ_REQ;
      }
      if ( Map5_IRQ_Line >= 0x40 )
      {
        Map5_IRQ_Enable = 0;
      }
    }
  } else {
    Map5_IRQ_Status |= 0x40;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 5 Rendering Screen Function                               */
/*-------------------------------------------------------------------*/
void Map5_RenderScreen( BYTE byMode )
{
  NES_DWORD dwPage[ 8 ];

  switch ( Map5_Chr_Size )
  {
    case 0:
      dwPage[ 7 ] = ( (NES_DWORD)Map5_Chr_Reg[7][byMode] << 3 ) % ( NesHeader.byVRomSize << 3 );

      PPUBANK[ 0 ] = VROMPAGE( dwPage[ 7 ] + 0 );
      PPUBANK[ 1 ] = VROMPAGE( dwPage[ 7 ] + 1 );
      PPUBANK[ 2 ] = VROMPAGE( dwPage[ 7 ] + 2 );
      PPUBANK[ 3 ] = VROMPAGE( dwPage[ 7 ] + 3 );
      PPUBANK[ 4 ] = VROMPAGE( dwPage[ 7 ] + 4 );
      PPUBANK[ 5 ] = VROMPAGE( dwPage[ 7 ] + 5 );
      PPUBANK[ 6 ] = VROMPAGE( dwPage[ 7 ] + 6 );
      PPUBANK[ 7 ] = VROMPAGE( dwPage[ 7 ] + 7 );
      InfoNES_SetupChr();
      break;

    case 1:
      dwPage[ 3 ] = ( (NES_DWORD)Map5_Chr_Reg[3][byMode] << 2 ) % ( NesHeader.byVRomSize << 3 );
      dwPage[ 7 ] = ( (NES_DWORD)Map5_Chr_Reg[7][byMode] << 2 ) % ( NesHeader.byVRomSize << 3 );

      PPUBANK[ 0 ] = VROMPAGE( dwPage[ 3 ] + 0 );
      PPUBANK[ 1 ] = VROMPAGE( dwPage[ 3 ] + 1 );
      PPUBANK[ 2 ] = VROMPAGE( dwPage[ 3 ] + 2 );
      PPUBANK[ 3 ] = VROMPAGE( dwPage[ 3 ] + 3 );
      PPUBANK[ 4 ] = VROMPAGE( dwPage[ 7 ] + 0 );
      PPUBANK[ 5 ] = VROMPAGE( dwPage[ 7 ] + 1 );
      PPUBANK[ 6 ] = VROMPAGE( dwPage[ 7 ] + 2 );
      PPUBANK[ 7 ] = VROMPAGE( dwPage[ 7 ] + 3 );
     InfoNES_SetupChr();
      break;

    case 2:
      dwPage[ 1 ] = ( (NES_DWORD)Map5_Chr_Reg[1][byMode] << 1 ) % ( NesHeader.byVRomSize << 3 );
      dwPage[ 3 ] = ( (NES_DWORD)Map5_Chr_Reg[3][byMode] << 1 ) % ( NesHeader.byVRomSize << 3 );
      dwPage[ 5 ] = ( (NES_DWORD)Map5_Chr_Reg[5][byMode] << 1 ) % ( NesHeader.byVRomSize << 3 );
      dwPage[ 7 ] = ( (NES_DWORD)Map5_Chr_Reg[7][byMode] << 1 ) % ( NesHeader.byVRomSize << 3 );

      PPUBANK[ 0 ] = VROMPAGE( dwPage[ 1 ] + 0 );
      PPUBANK[ 1 ] = VROMPAGE( dwPage[ 1 ] + 1 );
      PPUBANK[ 2 ] = VROMPAGE( dwPage[ 3 ] + 0 );
      PPUBANK[ 3 ] = VROMPAGE( dwPage[ 3 ] + 1 );
      PPUBANK[ 4 ] = VROMPAGE( dwPage[ 5 ] + 0 );
      PPUBANK[ 5 ] = VROMPAGE( dwPage[ 5 ] + 1 );
      PPUBANK[ 6 ] = VROMPAGE( dwPage[ 7 ] + 0 );
      PPUBANK[ 7 ] = VROMPAGE( dwPage[ 7 ] + 1 );
      InfoNES_SetupChr();
      break;

    default:
      dwPage[ 0 ] = (NES_DWORD)Map5_Chr_Reg[0][byMode] % ( NesHeader.byVRomSize << 3 );
      dwPage[ 1 ] = (NES_DWORD)Map5_Chr_Reg[1][byMode] % ( NesHeader.byVRomSize << 3 );
      dwPage[ 2 ] = (NES_DWORD)Map5_Chr_Reg[2][byMode] % ( NesHeader.byVRomSize << 3 );
      dwPage[ 3 ] = (NES_DWORD)Map5_Chr_Reg[3][byMode] % ( NesHeader.byVRomSize << 3 );
      dwPage[ 4 ] = (NES_DWORD)Map5_Chr_Reg[4][byMode] % ( NesHeader.byVRomSize << 3 );
      dwPage[ 5 ] = (NES_DWORD)Map5_Chr_Reg[5][byMode] % ( NesHeader.byVRomSize << 3 );
      dwPage[ 6 ] = (NES_DWORD)Map5_Chr_Reg[6][byMode] % ( NesHeader.byVRomSize << 3 );
      dwPage[ 7 ] = (NES_DWORD)Map5_Chr_Reg[7][byMode] % ( NesHeader.byVRomSize << 3 );

      PPUBANK[ 0 ] = VROMPAGE( dwPage[ 0 ] );
      PPUBANK[ 1 ] = VROMPAGE( dwPage[ 1 ] );
      PPUBANK[ 2 ] = VROMPAGE( dwPage[ 2 ] );
      PPUBANK[ 3 ] = VROMPAGE( dwPage[ 3 ] );
      PPUBANK[ 4 ] = VROMPAGE( dwPage[ 4 ] );
      PPUBANK[ 5 ] = VROMPAGE( dwPage[ 5 ] );
      PPUBANK[ 6 ] = VROMPAGE( dwPage[ 6 ] );
      PPUBANK[ 7 ] = VROMPAGE( dwPage[ 7 ] );
      InfoNES_SetupChr();
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 5 Sync Program Banks Function                             */
/*-------------------------------------------------------------------*/
void Map5_Sync_Prg_Banks( void )
{
  switch( Map5_Prg_Size )
  {
    case 0:
      Map5_Wram_Reg[ 4 ] = 0xff;
      Map5_Wram_Reg[ 5 ] = 0xff;
      Map5_Wram_Reg[ 6 ] = 0xff;

      ROMBANK0 = ROMPAGE( ( (Map5_Prg_Reg[7] & 0x7c) + 0 ) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK1 = ROMPAGE( ( (Map5_Prg_Reg[7] & 0x7c) + 1 ) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK2 = ROMPAGE( ( (Map5_Prg_Reg[7] & 0x7c) + 2 ) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK3 = ROMPAGE( ( (Map5_Prg_Reg[7] & 0x7c) + 3 ) % ( NesHeader.byRomSize << 1 ) );
      break;

    case 1:
      if ( Map5_Prg_Reg[ 5 ] & 0x80 )
      {
        Map5_Wram_Reg[ 4 ] = 0xff;
        Map5_Wram_Reg[ 5 ] = 0xff;
        ROMBANK0 = ROMPAGE( ( (Map5_Prg_Reg[7] & 0x7e) + 0 ) % ( NesHeader.byRomSize << 1 ) );
        ROMBANK1 = ROMPAGE( ( (Map5_Prg_Reg[7] & 0x7e) + 1 ) % ( NesHeader.byRomSize << 1 ) );
      } else {
        Map5_Wram_Reg[ 4 ] = ( Map5_Prg_Reg[ 5 ] & 0x06 ) + 0;
        Map5_Wram_Reg[ 5 ] = ( Map5_Prg_Reg[ 5 ] & 0x06 ) + 1;
        ROMBANK0 = Map5_ROMPAGE( Map5_Wram_Reg[ 4 ] );
        ROMBANK1 = Map5_ROMPAGE( Map5_Wram_Reg[ 5 ] );
      }

      Map5_Wram_Reg[ 6 ] = 0xff;
      ROMBANK2 = ROMPAGE( ( (Map5_Prg_Reg[7] & 0x7e) + 0 ) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK3 = ROMPAGE( ( (Map5_Prg_Reg[7] & 0x7e) + 1 ) % ( NesHeader.byRomSize << 1 ) );
      break;

    case 2:
      if ( Map5_Prg_Reg[ 5 ] & 0x80 )
      {
        Map5_Wram_Reg[ 4 ] = 0xff;
        Map5_Wram_Reg[ 5 ] = 0xff;
        ROMBANK0 = ROMPAGE( ( (Map5_Prg_Reg[5] & 0x7e) + 0 ) % ( NesHeader.byRomSize << 1 ) );
        ROMBANK1 = ROMPAGE( ( (Map5_Prg_Reg[5] & 0x7e) + 1 ) % ( NesHeader.byRomSize << 1 ) );
      } else {
        Map5_Wram_Reg[ 4 ] = ( Map5_Prg_Reg[ 5 ] & 0x06 ) + 0;
        Map5_Wram_Reg[ 5 ] = ( Map5_Prg_Reg[ 5 ] & 0x06 ) + 1;
        ROMBANK0 = Map5_ROMPAGE( Map5_Wram_Reg[ 4 ] );
        ROMBANK1 = Map5_ROMPAGE( Map5_Wram_Reg[ 5 ] );
      }

      if ( Map5_Prg_Reg[ 6 ] & 0x80 )
      {
        Map5_Wram_Reg[ 6 ] = 0xff;
        ROMBANK2 = ROMPAGE( (Map5_Prg_Reg[6] & 0x7f) % ( NesHeader.byRomSize << 1 ) );
      } else {
        Map5_Wram_Reg[ 6 ] = Map5_Prg_Reg[ 6 ] & 0x07;
        ROMBANK2 = Map5_ROMPAGE( Map5_Wram_Reg[ 6 ] );
      }

      ROMBANK3 = ROMPAGE( (Map5_Prg_Reg[7] & 0x7f) % ( NesHeader.byRomSize << 1 ) );
      break;

    default:
      if ( Map5_Prg_Reg[ 4 ] & 0x80 )
      {
        Map5_Wram_Reg[ 4 ] = 0xff;
        ROMBANK0 = ROMPAGE( (Map5_Prg_Reg[4] & 0x7f) % ( NesHeader.byRomSize << 1 ) );
      } else {
        Map5_Wram_Reg[ 4 ] = Map5_Prg_Reg[ 4 ] & 0x07;
        ROMBANK0 = Map5_ROMPAGE( Map5_Wram_Reg[ 4 ] );
      }

      if ( Map5_Prg_Reg[ 5 ] & 0x80 )
      {
        Map5_Wram_Reg[ 5 ] = 0xff;
        ROMBANK1 = ROMPAGE( (Map5_Prg_Reg[5] & 0x7f) % ( NesHeader.byRomSize << 1 ) );
      } else {
        Map5_Wram_Reg[ 5 ] = Map5_Prg_Reg[ 5 ] & 0x07;
        ROMBANK1 = Map5_ROMPAGE( Map5_Wram_Reg[ 5 ] );
      }

      if ( Map5_Prg_Reg[ 6 ] & 0x80 )
      {
        Map5_Wram_Reg[ 6 ] = 0xff;
        ROMBANK2 = ROMPAGE( (Map5_Prg_Reg[6] & 0x7f) % ( NesHeader.byRomSize << 1 ) );
      } else {
        Map5_Wram_Reg[ 6 ] = Map5_Prg_Reg[ 6 ] & 0x07;
        ROMBANK2 = Map5_ROMPAGE( Map5_Wram_Reg[ 6 ] );
      }

      ROMBANK3 = ROMPAGE( (Map5_Prg_Reg[7] & 0x7f) % ( NesHeader.byRomSize << 1 ) );
      break;
  }
}
