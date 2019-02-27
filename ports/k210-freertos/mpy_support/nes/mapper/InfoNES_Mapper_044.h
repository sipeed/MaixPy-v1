/*===================================================================*/
/*                                                                   */
/*                       Mapper 44 (Nin1)                            */
/*                                                                   */
/*===================================================================*/

BYTE  Map44_Regs[ 8 ];
NES_DWORD Map44_Rom_Bank;
NES_DWORD Map44_Prg0, Map44_Prg1;
NES_DWORD Map44_Chr01, Map44_Chr23;
NES_DWORD Map44_Chr4, Map44_Chr5, Map44_Chr6, Map44_Chr7;

#define Map44_Chr_Swap()    ( Map44_Regs[ 0 ] & 0x80 )
#define Map44_Prg_Swap()    ( Map44_Regs[ 0 ] & 0x40 )

BYTE Map44_IRQ_Enable;
BYTE Map44_IRQ_Cnt;
BYTE Map44_IRQ_Latch;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 44                                             */
/*-------------------------------------------------------------------*/
void Map44_Init()
{
  /* Initialize Mapper */
  MapperInit = Map44_Init;

  /* Write to Mapper */
  MapperWrite = Map44_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map44_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Initialize State Registers */
  for ( int nPage = 0; nPage < 8; nPage++ )
  {
    Map44_Regs[ nPage ] = 0x00;
  }

  /* Set ROM Banks */
  Map44_Rom_Bank = 0;
  Map44_Prg0 = 0;
  Map44_Prg1 = 1;
  Map44_Set_CPU_Banks();

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 )
  {
    Map44_Chr01 = 0;
    Map44_Chr23 = 2;
    Map44_Chr4  = 4;
    Map44_Chr5  = 5;
    Map44_Chr6  = 6;
    Map44_Chr7  = 7;
    Map44_Set_PPU_Banks();
  } else {
    Map44_Chr01 = Map44_Chr23 = 0;
    Map44_Chr4 = Map44_Chr5 = Map44_Chr6 = Map44_Chr7 = 0;
  }

  /* Initialize IRQ Registers */
  Map44_IRQ_Enable = 0;
  Map44_IRQ_Cnt = 0;
  Map44_IRQ_Latch = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 44 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map44_Write( WORD wAddr, BYTE byData )
{
  NES_DWORD dwBankNum;

  switch ( wAddr & 0xe001 )
  {
    case 0x8000:
      Map44_Regs[ 0 ] = byData;
      Map44_Set_PPU_Banks();
      Map44_Set_CPU_Banks();
      break;

    case 0x8001:
      Map44_Regs[ 1 ] = byData;
      dwBankNum = Map44_Regs[ 1 ];

      switch ( Map44_Regs[ 0 ] & 0x07 )
      {
        /* Set PPU Banks */
        case 0x00:
          if ( NesHeader.byVRomSize > 0 )
          {
            dwBankNum &= 0xfe;
            Map44_Chr01 = dwBankNum;
            Map44_Set_PPU_Banks();
          }
          break;

        case 0x01:
          if ( NesHeader.byVRomSize > 0 )
          {
            dwBankNum &= 0xfe;
            Map44_Chr23 = dwBankNum;
            Map44_Set_PPU_Banks();
          }
          break;

        case 0x02:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map44_Chr4 = dwBankNum;
            Map44_Set_PPU_Banks();
          }
          break;

        case 0x03:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map44_Chr5 = dwBankNum;
            Map44_Set_PPU_Banks();
          }
          break;

        case 0x04:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map44_Chr6 = dwBankNum;
            Map44_Set_PPU_Banks();
          }
          break;

        case 0x05:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map44_Chr7 = dwBankNum;
            Map44_Set_PPU_Banks();
          }
          break;

        /* Set ROM Banks */
        case 0x06:
          Map44_Prg0 = dwBankNum;
          Map44_Set_CPU_Banks();
          break;

        case 0x07:
          Map44_Prg1 = dwBankNum;
          Map44_Set_CPU_Banks();
          break;
      }
      break;

    case 0xa000:
      Map44_Regs[ 2 ] = byData;

      if ( !ROM_FourScr )
      {
        if ( byData & 0x01 )
        {
          InfoNES_Mirroring( 0 );
        } else {
          InfoNES_Mirroring( 1 );
        }
      }
      break;

    case 0xa001:
      Map44_Rom_Bank = byData & 0x07;
      if ( Map44_Rom_Bank == 7 )
      {
        Map44_Rom_Bank = 6;
      }
      Map44_Set_CPU_Banks();
      Map44_Set_PPU_Banks();
      break;

    case 0xc000:
      Map44_Regs[ 4 ] = byData;
      Map44_IRQ_Cnt = Map44_Regs[ 4 ];
      break;

    case 0xc001:
      Map44_Regs[ 5 ] = byData;
      Map44_IRQ_Latch = Map44_Regs[ 5 ];
      break;

    case 0xe000:
      Map44_Regs[ 6 ] = byData;
      Map44_IRQ_Enable = 0;
      break;

    case 0xe001:
      Map44_Regs[ 7 ] = byData;
      Map44_IRQ_Enable = 1;
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 44 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map44_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( Map44_IRQ_Enable )
  {
    if ( /* 0 <= PPU_Scanline && */ PPU_Scanline <= 239 )
    {
      if ( PPU_R1 & R1_SHOW_SCR || PPU_R1 & R1_SHOW_SP )
      {
        if ( !( Map44_IRQ_Cnt-- ) )
        {
          Map44_IRQ_Cnt = Map44_IRQ_Latch;
          IRQ_REQ;
        }
      }
    }
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 44 Set CPU Banks Function                                 */
/*-------------------------------------------------------------------*/
void Map44_Set_CPU_Banks()
{
  if ( Map44_Prg_Swap() )
  {
    ROMBANK0 = ROMPAGE( ( ( Map44_Rom_Bank << 4 ) + 14 ) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK1 = ROMPAGE( ( ( Map44_Rom_Bank << 4 ) + Map44_Prg1 ) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMPAGE( ( ( Map44_Rom_Bank << 4 ) + Map44_Prg0 ) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK3 = ROMPAGE( ( ( Map44_Rom_Bank << 4 ) + 15 ) % ( NesHeader.byRomSize << 1 ) );
  } else {
    ROMBANK0 = ROMPAGE( ( ( Map44_Rom_Bank << 4 ) + Map44_Prg0 ) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK1 = ROMPAGE( ( ( Map44_Rom_Bank << 4 ) + Map44_Prg1 ) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMPAGE( ( ( Map44_Rom_Bank << 4 ) + 14 ) % ( NesHeader.byRomSize << 1 ) );
    ROMBANK3 = ROMPAGE( ( ( Map44_Rom_Bank << 4 ) + 15 ) % ( NesHeader.byRomSize << 1 ) );
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 44 Set PPU Banks Function                                 */
/*-------------------------------------------------------------------*/
void Map44_Set_PPU_Banks()
{
  if ( NesHeader.byVRomSize > 0 )
  {
    if ( Map44_Chr_Swap() )
    { 
      PPUBANK[ 0 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr4 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr5 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr6 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr7 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    } else {
      PPUBANK[ 0 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr4 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr5 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr6 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( ( ( Map44_Rom_Bank << 7 ) + Map44_Chr7 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    }
  }
}
