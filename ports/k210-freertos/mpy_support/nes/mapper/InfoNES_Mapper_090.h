/*===================================================================*/
/*                                                                   */
/*                        Mapper 90 (PC-JY-??)                       */
/*                                                                   */
/*===================================================================*/

BYTE Map90_Prg_Reg[ 4 ];
BYTE Map90_Chr_Low_Reg[ 8 ];
BYTE Map90_Chr_High_Reg[ 8 ];
BYTE Map90_Nam_Low_Reg[ 4 ];
BYTE Map90_Nam_High_Reg[ 4 ];

BYTE Map90_Prg_Bank_Size;
BYTE Map90_Prg_Bank_6000;
BYTE Map90_Prg_Bank_E000;
BYTE Map90_Chr_Bank_Size;
BYTE Map90_Mirror_Mode;
BYTE Map90_Mirror_Type;

NES_DWORD Map90_Value1;
NES_DWORD Map90_Value2;

BYTE Map90_IRQ_Enable;
BYTE Map90_IRQ_Cnt;
BYTE Map90_IRQ_Latch;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 90                                             */
/*-------------------------------------------------------------------*/
void Map90_Init()
{
  /* Initialize Mapper */
  MapperInit = Map90_Init;

  /* Write to Mapper */
  MapperWrite = Map90_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map90_Apu;

  /* Read from APU */
  MapperReadApu = Map90_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map90_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Set ROM Banks */
  ROMBANK0 = ROMLASTPAGE( 3 );
  ROMBANK1 = ROMLASTPAGE( 2 );
  ROMBANK2 = ROMLASTPAGE( 1 );
  ROMBANK3 = ROMLASTPAGE( 0 );

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 )
  {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Initialize IRQ Registers */
  Map90_IRQ_Cnt = 0;
  Map90_IRQ_Latch = 0;
  Map90_IRQ_Enable = 0;

  for ( BYTE byPage = 0; byPage < 4; byPage++ )
  {
    Map90_Prg_Reg[ byPage ] = ( NesHeader.byRomSize << 1 ) - 4 + byPage;
    Map90_Nam_Low_Reg[ byPage ] = 0;
    Map90_Nam_High_Reg[ byPage ] = 0;
    Map90_Chr_Low_Reg[ byPage ] = byPage;
    Map90_Chr_High_Reg[ byPage ] = 0;
    Map90_Chr_Low_Reg[ byPage + 4 ] = byPage + 4;
    Map90_Chr_High_Reg[ byPage + 4 ] = 0;
  }

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 90 Write to APU Function                                  */
/*-------------------------------------------------------------------*/
void Map90_Apu( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x5000:
      Map90_Value1 = byData;
      break;

    case 0x5001:
      Map90_Value2 = byData;
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 90 Read from APU Function                                 */
/*-------------------------------------------------------------------*/
BYTE Map90_ReadApu( WORD wAddr )
{
  if ( wAddr == 0x5000 )
  {
    return ( BYTE )( ( Map90_Value1 * Map90_Value2 ) & 0x00ff );
  } else {
    return ( BYTE )( wAddr >> 8 );
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 90 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map90_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    /* Set ROM Banks */
    case 0x8000:
    case 0x8001:
    case 0x8002:
    case 0x8003:
      Map90_Prg_Reg[ wAddr & 0x03 ] = byData;
      Map90_Sync_Prg_Banks();
      break;
    
    case 0x9000:
    case 0x9001:
    case 0x9002:
    case 0x9003:
    case 0x9004:
    case 0x9005:
    case 0x9006:
    case 0x9007:
      Map90_Chr_Low_Reg[ wAddr & 0x07 ] = byData;
      Map90_Sync_Chr_Banks();
      break;

    case 0xa000:
    case 0xa001:
    case 0xa002:
    case 0xa003:
    case 0xa004:
    case 0xa005:
    case 0xa006:
    case 0xa007:
      Map90_Chr_High_Reg[ wAddr & 0x07 ] = byData;
      Map90_Sync_Chr_Banks();
      break;

    case 0xb000:
    case 0xb001:
    case 0xb002:
    case 0xb003:
      Map90_Nam_Low_Reg[ wAddr & 0x03 ] = byData;
      Map90_Sync_Mirror();
      break;

    case 0xb004:
    case 0xb005:
    case 0xb006:
    case 0xb007:
      Map90_Nam_High_Reg[ wAddr & 0x03 ] = byData;
      Map90_Sync_Mirror();
      break;

    case 0xc002:
      Map90_IRQ_Enable = 0;
      break;

    case 0xc003:
    case 0xc004:
      if ( Map90_IRQ_Enable == 0 )
      {
        Map90_IRQ_Enable = 1;
        Map90_IRQ_Cnt = Map90_IRQ_Latch;
      }
      break;

    case 0xc005:
      Map90_IRQ_Cnt = byData;
      Map90_IRQ_Latch = byData;
      break;

    case 0xd000:
      Map90_Prg_Bank_6000 = byData & 0x80;
      Map90_Prg_Bank_E000 = byData & 0x04;
      Map90_Prg_Bank_Size = byData & 0x03;
      Map90_Chr_Bank_Size = ( byData & 0x18 ) >> 3;
      Map90_Mirror_Mode = byData & 0x20;
      
      Map90_Sync_Prg_Banks();
      Map90_Sync_Chr_Banks();
      Map90_Sync_Mirror();
      break;

    case 0xd001:
      Map90_Mirror_Type = byData & 0x03;
      Map90_Sync_Mirror();
      break;

    case 0xd003:
      /* Bank Page*/
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 90 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map90_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( /* 0 <= PPU_Scanline && */ PPU_Scanline <= 239 )
  {
    if ( PPU_R1 & R1_SHOW_SCR || PPU_R1 & R1_SHOW_SP )
    {
      if ( Map90_IRQ_Cnt == 0 )
      {
        if ( Map90_IRQ_Enable )
        {
          IRQ_REQ;
        }
        Map90_IRQ_Latch = 0;
        Map90_IRQ_Enable = 0;
      } else {
        Map90_IRQ_Cnt--;
      }
    }
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 90 Sync Mirror Function                                   */
/*-------------------------------------------------------------------*/
void Map90_Sync_Mirror( void )
{
  BYTE byPage;
  NES_DWORD dwNamBank[ 4 ];

  for ( byPage = 0; byPage < 4; byPage++ )
  {
    dwNamBank[ byPage ] = 
      ( (NES_DWORD)Map90_Nam_High_Reg[ byPage ] << 8 ) | (NES_DWORD)Map90_Nam_Low_Reg[ byPage ];
  }

  if ( Map90_Mirror_Mode )
  {
    for ( byPage = 0; byPage < 4; byPage++ )
    {
      if ( !Map90_Nam_High_Reg[ byPage ] && ( Map90_Nam_Low_Reg[ byPage ] == byPage ) )
      {
        Map90_Mirror_Mode = 0;
      }
    }

    if ( Map90_Mirror_Mode )
    {
        PPUBANK[ NAME_TABLE0 ] = VROMPAGE( dwNamBank[ 0 ] % ( NesHeader.byVRomSize << 3 ) );
        PPUBANK[ NAME_TABLE1 ] = VROMPAGE( dwNamBank[ 1 ] % ( NesHeader.byVRomSize << 3 ) );
        PPUBANK[ NAME_TABLE2 ] = VROMPAGE( dwNamBank[ 2 ] % ( NesHeader.byVRomSize << 3 ) );
        PPUBANK[ NAME_TABLE3 ] = VROMPAGE( dwNamBank[ 3 ] % ( NesHeader.byVRomSize << 3 ) );
    }
  } else {
    switch ( Map90_Mirror_Type )
    {
      case 0x00:
        InfoNES_Mirroring( 1 );
        break;

      case 0x01:
        InfoNES_Mirroring( 0 );
        break;

      default:
        InfoNES_Mirroring( 3 );
        break;
    }
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 90 Sync Char Banks Function                               */
/*-------------------------------------------------------------------*/
void Map90_Sync_Chr_Banks( void )
{
  BYTE byPage;
  NES_DWORD dwChrBank[ 8 ];

  for ( byPage = 0; byPage < 8; byPage++ )
  {
    dwChrBank[ byPage ] =
      ( (NES_DWORD)Map90_Chr_High_Reg[ byPage ] << 8 ) | (NES_DWORD)Map90_Chr_Low_Reg[ byPage ];
  }

  switch ( Map90_Chr_Bank_Size )
  {
    case 0:
      PPUBANK[ 0 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 3 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 3 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 3 ) + 2 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 3 ) + 3 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 3 ) + 4 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 3 ) + 5 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 3 ) + 6 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 3 ) + 7 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 1:
      PPUBANK[ 0 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 2 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 2 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 2 ) + 2 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 2 ) + 3 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( ( ( dwChrBank[ 4 ] << 2 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( ( ( dwChrBank[ 4 ] << 2 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( ( ( dwChrBank[ 4 ] << 2 ) + 2 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( ( ( dwChrBank[ 4 ] << 2 ) + 3 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 2:
      PPUBANK[ 0 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( ( dwChrBank[ 0 ] << 1 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( ( ( dwChrBank[ 2 ] << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( ( dwChrBank[ 2 ] << 1 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( ( ( dwChrBank[ 4 ] << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( ( ( dwChrBank[ 4 ] << 1 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( ( ( dwChrBank[ 6 ] << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( ( ( dwChrBank[ 6 ] << 1 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    default:
      PPUBANK[ 0 ] = VROMPAGE( dwChrBank[ 0 ] % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( dwChrBank[ 1 ] % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( dwChrBank[ 2 ] % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( dwChrBank[ 3 ] % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( dwChrBank[ 4 ] % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( dwChrBank[ 5 ] % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( dwChrBank[ 6 ] % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( dwChrBank[ 7 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 90 Sync Program Banks Function                            */
/*-------------------------------------------------------------------*/
void Map90_Sync_Prg_Banks( void )
{
  switch ( Map90_Prg_Bank_Size )
  {
    case 0:
      ROMBANK0 = ROMLASTPAGE( 3 );
      ROMBANK1 = ROMLASTPAGE( 2 );
      ROMBANK2 = ROMLASTPAGE( 1 );
      ROMBANK3 = ROMLASTPAGE( 0 );      
      break;

    case 1:
      ROMBANK0 = ROMPAGE( ( ( Map90_Prg_Reg[ 1 ] << 1 ) + 0 ) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK1 = ROMPAGE( ( ( Map90_Prg_Reg[ 1 ] << 1 ) + 1 ) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK2 = ROMLASTPAGE( 1 );
      ROMBANK3 = ROMLASTPAGE( 0 );      
      break;

    case 2:
      if ( Map90_Prg_Bank_E000 )
      {
        ROMBANK0 = ROMPAGE( Map90_Prg_Reg[ 0 ] % ( NesHeader.byRomSize << 1 ) );
        ROMBANK1 = ROMPAGE( Map90_Prg_Reg[ 1 ] % ( NesHeader.byRomSize << 1 ) );
        ROMBANK2 = ROMPAGE( Map90_Prg_Reg[ 2 ] % ( NesHeader.byRomSize << 1 ) );
        ROMBANK3 = ROMPAGE( Map90_Prg_Reg[ 3 ] % ( NesHeader.byRomSize << 1 ) );   
      } else {
        if ( Map90_Prg_Bank_6000 )
        {
          SRAMBANK = ROMPAGE( Map90_Prg_Reg[ 3 ] % ( NesHeader.byRomSize << 1 ) );
        }
        ROMBANK0 = ROMPAGE( Map90_Prg_Reg[ 0 ] % ( NesHeader.byRomSize << 1 ) );
        ROMBANK1 = ROMPAGE( Map90_Prg_Reg[ 1 ] % ( NesHeader.byRomSize << 1 ) );
        ROMBANK2 = ROMPAGE( Map90_Prg_Reg[ 2 ] % ( NesHeader.byRomSize << 1 ) );
        ROMBANK3 = ROMLASTPAGE( 0 );  
      }
      break;

    default:
      /* 8k in reverse mode? */
      ROMBANK0 = ROMPAGE( Map90_Prg_Reg[ 3 ] % ( NesHeader.byRomSize << 1 ) );
      ROMBANK1 = ROMPAGE( Map90_Prg_Reg[ 2 ] % ( NesHeader.byRomSize << 1 ) );
      ROMBANK2 = ROMPAGE( Map90_Prg_Reg[ 1 ] % ( NesHeader.byRomSize << 1 ) );
      ROMBANK3 = ROMPAGE( Map90_Prg_Reg[ 0 ] % ( NesHeader.byRomSize << 1 ) ); 
      break;
  }
}
