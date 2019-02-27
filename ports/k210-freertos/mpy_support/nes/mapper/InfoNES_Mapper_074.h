/*===================================================================*/
/*                                                                   */
/*         Mapper 74 : Metal Max (Zhong Zhuang Ji Bing               */
/*                                                                   */
/*===================================================================*/

BYTE  Map74_Regs[ 8 ];
NES_DWORD Map74_Rom_Bank;
NES_DWORD Map74_Prg0, Map74_Prg1;
NES_DWORD Map74_Chr01, Map74_Chr23;
NES_DWORD Map74_Chr4, Map74_Chr5, Map74_Chr6, Map74_Chr7;

#define Map74_Chr_Swap()    ( Map74_Regs[ 0 ] & 0x80 )
#define Map74_Prg_Swap()    ( Map74_Regs[ 0 ] & 0x40 )

BYTE Map74_IRQ_Enable;
BYTE Map74_IRQ_Cnt;
BYTE Map74_IRQ_Latch;
BYTE Map74_IRQ_Request;
BYTE Map74_IRQ_Present;
BYTE Map74_IRQ_Present_Vbl;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 74                                             */
/*-------------------------------------------------------------------*/
void Map74_Init()
{
  /* Initialize Mapper */
  MapperInit = Map74_Init;

  /* Write to Mapper */
  MapperWrite = Map74_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map74_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Initialize State Registers */
  for ( int nPage = 0; nPage < 8; nPage++ )
  {
    Map74_Regs[ nPage ] = 0x00;
  }

  /* Set ROM Banks */
  Map74_Prg0 = 0;
  Map74_Prg1 = 1;
  Map74_Set_CPU_Banks();

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 )
  {
    Map74_Chr01 = 0;
    Map74_Chr23 = 2;
    Map74_Chr4  = 4;
    Map74_Chr5  = 5;
    Map74_Chr6  = 6;
    Map74_Chr7  = 7;
    Map74_Set_PPU_Banks();
  } else {
    Map74_Chr01 = Map74_Chr23 = 0;
    Map74_Chr4 = Map74_Chr5 = Map74_Chr6 = Map74_Chr7 = 0;
  }

  /* Initialize IRQ Registers */
  Map74_IRQ_Enable = 0;
  Map74_IRQ_Cnt = 0;
  Map74_IRQ_Latch = 0;
  Map74_IRQ_Request = 0;
  Map74_IRQ_Present = 0;
  Map74_IRQ_Present_Vbl = 0;

  /* VRAM Write Enabled */
  byVramWriteEnable = 1;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 74 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map74_Write( WORD wAddr, BYTE byData )
{
  NES_DWORD dwBankNum;

  switch ( wAddr & 0xe001 )
  {
    case 0x8000:
      Map74_Regs[ 0 ] = byData;
      Map74_Set_PPU_Banks();
      Map74_Set_CPU_Banks();
      break;

    case 0x8001:
      Map74_Regs[ 1 ] = byData;
      dwBankNum = Map74_Regs[ 1 ];

      switch ( Map74_Regs[ 0 ] & 0x07 )
      {
        /* Set PPU Banks */
        case 0x00:
          if ( NesHeader.byVRomSize > 0 )
          {
            dwBankNum &= 0xfe;
            Map74_Chr01 = dwBankNum;
            Map74_Set_PPU_Banks();
          }
          break;

        case 0x01:
          if ( NesHeader.byVRomSize > 0 )
          {
            dwBankNum &= 0xfe;
            Map74_Chr23 = dwBankNum;
            Map74_Set_PPU_Banks();
          }
          break;

        case 0x02:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map74_Chr4 = dwBankNum;
            Map74_Set_PPU_Banks();
          }
          break;

        case 0x03:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map74_Chr5 = dwBankNum;
            Map74_Set_PPU_Banks();
          }
          break;

        case 0x04:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map74_Chr6 = dwBankNum;
            Map74_Set_PPU_Banks();
          }
          break;

        case 0x05:
          if ( NesHeader.byVRomSize > 0 )
          {
            Map74_Chr7 = dwBankNum;
            Map74_Set_PPU_Banks();
          }
          break;

        /* Set ROM Banks */
        case 0x06:
          Map74_Prg0 = dwBankNum;
          Map74_Set_CPU_Banks();
          break;

        case 0x07:
          Map74_Prg1 = dwBankNum;
          Map74_Set_CPU_Banks();
          break;
      }
      break;

    case 0xa000:
      Map74_Regs[ 2 ] = byData;

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
      Map74_Regs[ 3 ] = byData;
      break;

    case 0xc000:
      Map74_Regs[ 4 ] = byData;
      Map74_IRQ_Latch = byData;
      break;

    case 0xc001:
      Map74_Regs[ 5 ] = byData;
      if ( PPU_Scanline < 240 )
      {
          Map74_IRQ_Cnt |= 0x80;
          Map74_IRQ_Present = 0xff;
      } else {
          Map74_IRQ_Cnt |= 0x80;
          Map74_IRQ_Present_Vbl = 0xff;
          Map74_IRQ_Present = 0;
      }
      break;

    case 0xe000:
      Map74_Regs[ 6 ] = byData;
      Map74_IRQ_Enable = 0;
			Map74_IRQ_Request = 0;
      break;

    case 0xe001:
      Map74_Regs[ 7 ] = byData;
      Map74_IRQ_Enable = 1;
			Map74_IRQ_Request = 0;
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 74 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map74_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( ( /* 0 <= PPU_Scanline && */ PPU_Scanline <= 239 ) && 
       ( PPU_R1 & R1_SHOW_SCR || PPU_R1 & R1_SHOW_SP ) )
  {
		if( Map74_IRQ_Present_Vbl ) {
			Map74_IRQ_Cnt = Map74_IRQ_Latch;
			Map74_IRQ_Present_Vbl = 0;
		}
		if( Map74_IRQ_Present ) {
			Map74_IRQ_Cnt = Map74_IRQ_Latch;
			Map74_IRQ_Present = 0;
		} else if( Map74_IRQ_Cnt > 0 ) {
			Map74_IRQ_Cnt--;
		}

		if( Map74_IRQ_Cnt == 0 ) {
			if( Map74_IRQ_Enable ) {
				Map74_IRQ_Request = 0xFF;
			}
			Map74_IRQ_Present = 0xFF;
		}
	}
	if( Map74_IRQ_Request  ) {
		IRQ_REQ;
	}
}

/*-------------------------------------------------------------------*/
/*  Mapper 74 Set CPU Banks Function                                 */
/*-------------------------------------------------------------------*/
void Map74_Set_CPU_Banks()
{
  if ( Map74_Prg_Swap() )
  {
    ROMBANK0 = ROMLASTPAGE( 1 );
    ROMBANK1 = ROMPAGE( Map74_Prg1 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMPAGE( Map74_Prg0 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK3 = ROMLASTPAGE( 0 );
  } else {
    ROMBANK0 = ROMPAGE( Map74_Prg0 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK1 = ROMPAGE( Map74_Prg1 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMLASTPAGE( 1 );
    ROMBANK3 = ROMLASTPAGE( 0 );
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 74 Set PPU Banks Function                                 */
/*-------------------------------------------------------------------*/
void Map74_Set_PPU_Banks()
{
  if ( NesHeader.byVRomSize > 0 )
  {
    if ( Map74_Chr_Swap() )
    { 
      PPUBANK[ 0 ] = VROMPAGE( Map74_Chr4 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( Map74_Chr5 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( Map74_Chr6 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( Map74_Chr7 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( ( Map74_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( ( Map74_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( ( Map74_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( ( Map74_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    } else {
      PPUBANK[ 0 ] = VROMPAGE( ( Map74_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( Map74_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( ( Map74_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( Map74_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( Map74_Chr4 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( Map74_Chr5 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( Map74_Chr6 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( Map74_Chr7 % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    }
  }
  else
  {
    if ( Map74_Chr_Swap() )
    { 
      PPUBANK[ 0 ] = CRAMPAGE( 0 );
      PPUBANK[ 1 ] = CRAMPAGE( 1 );
      PPUBANK[ 2 ] = CRAMPAGE( 2 );
      PPUBANK[ 3 ] = CRAMPAGE( 3 );
      PPUBANK[ 4 ] = CRAMPAGE( 4 );
      PPUBANK[ 5 ] = CRAMPAGE( 5 );
      PPUBANK[ 6 ] = CRAMPAGE( 6 );
      PPUBANK[ 7 ] = CRAMPAGE( 7 );
      InfoNES_SetupChr();
    } else {
      PPUBANK[ 0 ] = CRAMPAGE( 0 );
      PPUBANK[ 1 ] = CRAMPAGE( 1 );
      PPUBANK[ 2 ] = CRAMPAGE( 2 );
      PPUBANK[ 3 ] = CRAMPAGE( 3 );
      PPUBANK[ 4 ] = CRAMPAGE( 4 );
      PPUBANK[ 5 ] = CRAMPAGE( 5 );
      PPUBANK[ 6 ] = CRAMPAGE( 6 );
      PPUBANK[ 7 ] = CRAMPAGE( 7 );
      InfoNES_SetupChr();
    }
  }    
}

