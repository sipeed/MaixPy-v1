/*===================================================================*/
/*                                                                   */
/*                       Mapper 4 (MMC3)                             */
/*                                                                   */
/*===================================================================*/

BYTE  Map4_Regs[ 8 ];
NES_DWORD Map4_Rom_Bank;
NES_DWORD Map4_Prg0, Map4_Prg1;
NES_DWORD Map4_Chr01, Map4_Chr23;
NES_DWORD Map4_Chr4, Map4_Chr5, Map4_Chr6, Map4_Chr7;

#define Map4_Chr_Swap()    ( Map4_Regs[ 0 ] & 0x80 )
#define Map4_Prg_Swap()    ( Map4_Regs[ 0 ] & 0x40 )

BYTE Map4_IRQ_Enable;
BYTE Map4_IRQ_Cnt;
BYTE Map4_IRQ_Latch;
BYTE Map4_IRQ_Request;
BYTE Map4_IRQ_Present;
BYTE Map4_IRQ_Present_Vbl;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 4                                              */
/*-------------------------------------------------------------------*/
void Map4_Init()
{
  /* Initialize Mapper */
  MapperInit = Map4_Init;

  /* Write to Mapper */
  MapperWrite = Map4_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map4_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Initialize State Registers */
  for ( int nPage = 0; nPage < 8; nPage++ )
  {
    Map4_Regs[ nPage ] = 0x00;
  }

  /* Set ROM Banks */
  Map4_Prg0 = 0;
  Map4_Prg1 = 1;
  Map4_Set_CPU_Banks();

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 )
  {
    Map4_Chr01 = 0;
    Map4_Chr23 = 2;
    Map4_Chr4  = 4;
    Map4_Chr5  = 5;
    Map4_Chr6  = 6;
    Map4_Chr7  = 7;
    Map4_Set_PPU_Banks();
  } else {
    Map4_Chr01 = Map4_Chr23 = 0;
    Map4_Chr4 = Map4_Chr5 = Map4_Chr6 = Map4_Chr7 = 0;
  }

  /* Initialize IRQ Registers */
  Map4_IRQ_Enable = 0;
  Map4_IRQ_Cnt = 0;
  Map4_IRQ_Latch = 0;
  Map4_IRQ_Request = 0;
  Map4_IRQ_Present = 0;
  Map4_IRQ_Present_Vbl = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 4 Write Function                                          */
/*-------------------------------------------------------------------*/
void Map4_Write( WORD wAddr, BYTE byData )
{
  NES_DWORD dwBankNum;

  switch ( wAddr & 0xe001 )
  {
    case 0x8000:
      Map4_Regs[ 0 ] = byData;
      Map4_Set_PPU_Banks();
      Map4_Set_CPU_Banks();
      break;

    case 0x8001:
      Map4_Regs[ 1 ] = byData;
      dwBankNum = Map4_Regs[ 1 ];

      switch ( Map4_Regs[ 0 ] & 0x07 )
      {
        /* Set PPU Banks */
        case 0x00:
          if ( NesHeader.byVRomSize > 0 )
          {
            dwBankNum &= 0xfe;
            Map4_Chr01 = dwBankNum;
            Map4_Set_PPU_Banks();
          }
          break;

        case 0x01:
          if ( NesHeader.byVRomSize > 0 )
          {
            dwBankNum &= 0xfe;
            Map4_Chr23 = dwBankNum;
            Map4_Set_PPU_Banks();
          }
          break;

        case 0x02:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map4_Chr4 = dwBankNum;
            Map4_Set_PPU_Banks();
          }
          break;

        case 0x03:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map4_Chr5 = dwBankNum;
            Map4_Set_PPU_Banks();
          }
          break;

        case 0x04:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map4_Chr6 = dwBankNum;
            Map4_Set_PPU_Banks();
          }
          break;

        case 0x05:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map4_Chr7 = dwBankNum;
            Map4_Set_PPU_Banks();
          }
          break;

        /* Set ROM Banks */
        case 0x06:
          Map4_Prg0 = dwBankNum;
          Map4_Set_CPU_Banks();
          break;

        case 0x07:
          Map4_Prg1 = dwBankNum;
          Map4_Set_CPU_Banks();
          break;
      }
      break;

    case 0xa000:
      Map4_Regs[ 2 ] = byData;

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
      Map4_Regs[ 3 ] = byData;
      break;

    case 0xc000:
      Map4_Regs[ 4 ] = byData;
      Map4_IRQ_Latch = byData;
      break;

    case 0xc001:
      Map4_Regs[ 5 ] = byData;
      if ( PPU_Scanline < 240 )
      {
          Map4_IRQ_Cnt |= 0x80;
          Map4_IRQ_Present = 0xff;
      } else {
          Map4_IRQ_Cnt |= 0x80;
          Map4_IRQ_Present_Vbl = 0xff;
          Map4_IRQ_Present = 0;
      }
      break;

    case 0xe000:
      Map4_Regs[ 6 ] = byData;
      Map4_IRQ_Enable = 0;
			Map4_IRQ_Request = 0;
      break;

    case 0xe001:
      Map4_Regs[ 7 ] = byData;
      Map4_IRQ_Enable = 1;
			Map4_IRQ_Request = 0;
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 4 H-Sync Function                                         */
/*-------------------------------------------------------------------*/
void Map4_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( ( /* 0 <= PPU_Scanline && */ PPU_Scanline <= 239 ) && 
       ( PPU_R1 & R1_SHOW_SCR || PPU_R1 & R1_SHOW_SP ) )
  {
		if( Map4_IRQ_Present_Vbl ) {
			Map4_IRQ_Cnt = Map4_IRQ_Latch;
			Map4_IRQ_Present_Vbl = 0;
		}
		if( Map4_IRQ_Present ) {
			Map4_IRQ_Cnt = Map4_IRQ_Latch;
			Map4_IRQ_Present = 0;
		} else if( Map4_IRQ_Cnt > 0 ) {
			Map4_IRQ_Cnt--;
		}

		if( Map4_IRQ_Cnt == 0 ) {
			if( Map4_IRQ_Enable ) {
				Map4_IRQ_Request = 0xFF;
			}
			Map4_IRQ_Present = 0xFF;
		}
	}
	if( Map4_IRQ_Request  ) {
		IRQ_REQ;
	}
}

/*-------------------------------------------------------------------*/
/*  Mapper 4 Set CPU Banks Function                                  */
/*-------------------------------------------------------------------*/
void Map4_Set_CPU_Banks()
{
  if ( Map4_Prg_Swap() )
  {
    ROMBANK0 = ROMLASTPAGE( 1 );
    ROMBANK1 = ROMPAGE( Map4_Prg1 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMPAGE( Map4_Prg0 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK3 = ROMLASTPAGE( 0 );
  } else {
    ROMBANK0 = ROMPAGE( Map4_Prg0 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK1 = ROMPAGE( Map4_Prg1 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMLASTPAGE( 1 );
    ROMBANK3 = ROMLASTPAGE( 0 );
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 4 Set PPU Banks Function                                  */
/*-------------------------------------------------------------------*/
void Map4_Set_PPU_Banks()
{
  if ( NesHeader.byVRomSize > 0 )
  {
    if ( Map4_Chr_Swap() )
    { 
      PPUBANK[ 0 ] = VROMPAGE( Map4_Chr4 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( Map4_Chr5 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( Map4_Chr6 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( Map4_Chr7 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( ( Map4_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( ( Map4_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( ( Map4_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( ( Map4_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    } else {
      PPUBANK[ 0 ] = VROMPAGE( ( Map4_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( Map4_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( ( Map4_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( Map4_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( Map4_Chr4 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( Map4_Chr5 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( Map4_Chr6 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( Map4_Chr7 % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    }
  }
  else
  {
    if ( Map4_Chr_Swap() )
    { 
#if 0
      PPUBANK[ 0 ] = VRAMPAGE0( 0 );
      PPUBANK[ 1 ] = VRAMPAGE0( 1 );
      PPUBANK[ 2 ] = VRAMPAGE0( 2 );
      PPUBANK[ 3 ] = VRAMPAGE0( 3 );
      PPUBANK[ 4 ] = VRAMPAGE1( 0 );
      PPUBANK[ 5 ] = VRAMPAGE1( 1 );
      PPUBANK[ 6 ] = VRAMPAGE1( 2 );
      PPUBANK[ 7 ] = VRAMPAGE1( 3 );
#else
      PPUBANK[ 0 ] = CRAMPAGE( 0 );
      PPUBANK[ 1 ] = CRAMPAGE( 1 );
      PPUBANK[ 2 ] = CRAMPAGE( 2 );
      PPUBANK[ 3 ] = CRAMPAGE( 3 );
      PPUBANK[ 4 ] = CRAMPAGE( 4 );
      PPUBANK[ 5 ] = CRAMPAGE( 5 );
      PPUBANK[ 6 ] = CRAMPAGE( 6 );
      PPUBANK[ 7 ] = CRAMPAGE( 7 );
#endif
      InfoNES_SetupChr();
    } else {
#if 0
      PPUBANK[ 0 ] = VRAMPAGE1( 0 );
      PPUBANK[ 1 ] = VRAMPAGE1( 1 );
      PPUBANK[ 2 ] = VRAMPAGE1( 2 );
      PPUBANK[ 3 ] = VRAMPAGE1( 3 );
      PPUBANK[ 4 ] = VRAMPAGE0( 0 );
      PPUBANK[ 5 ] = VRAMPAGE0( 1 );
      PPUBANK[ 6 ] = VRAMPAGE0( 2 );
      PPUBANK[ 7 ] = VRAMPAGE0( 3 );
#else
      PPUBANK[ 0 ] = CRAMPAGE( 0 );
      PPUBANK[ 1 ] = CRAMPAGE( 1 );
      PPUBANK[ 2 ] = CRAMPAGE( 2 );
      PPUBANK[ 3 ] = CRAMPAGE( 3 );
      PPUBANK[ 4 ] = CRAMPAGE( 4 );
      PPUBANK[ 5 ] = CRAMPAGE( 5 );
      PPUBANK[ 6 ] = CRAMPAGE( 6 );
      PPUBANK[ 7 ] = CRAMPAGE( 7 );
#endif
      InfoNES_SetupChr();
    }
  }    
}
