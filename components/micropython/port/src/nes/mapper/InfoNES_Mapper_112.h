/*===================================================================*/
/*                                                                   */
/*                        Mapper 112 (Pirates)                       */
/*                                                                   */
/*===================================================================*/

BYTE  Map112_Regs[8];
NES_DWORD Map112_Prg0,Map112_Prg1;
NES_DWORD Map112_Chr01,Map112_Chr23,Map112_Chr4,Map112_Chr5,Map112_Chr6,Map112_Chr7;

BYTE  Map112_IRQ_Enable;  /* IRQs enabled */
BYTE  Map112_IRQ_Cnt;     /* IRQ scanline counter, decreasing */
BYTE  Map112_IRQ_Latch;   /* IRQ scanline counter latch */

#define Map112_Chr_Swap()  ( Map112_Regs[0] & 0x80 )
#define Map112_Prg_Swap()  ( Map112_Regs[0] & 0x40 )

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 112                                            */
/*-------------------------------------------------------------------*/
void Map112_Init()
{
  /* Initialize Mapper */
  MapperInit = Map112_Init;

  /* Write to Mapper */
  MapperWrite = Map112_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map112_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* clear registers FIRST!!! */
  for(int i = 0; i < 8; i++) 
  {
    Map112_Regs[i] = 0x00;
  }

  /* set CPU bank pointers */
  Map112_Prg0 = 0;
  Map112_Prg1 = 1;
  Map112_Set_CPU_Banks();

  /* set VROM banks */
  if ( NesHeader.byVRomSize > 0 )
  {
    Map112_Chr01 = 0;
    Map112_Chr23 = 2;
    Map112_Chr4  = 4;
    Map112_Chr5  = 5;
    Map112_Chr6  = 6;
    Map112_Chr7  = 7;
    Map112_Set_PPU_Banks();
  }
  else
  {
    Map112_Chr01 = Map112_Chr23 = Map112_Chr4 = Map112_Chr5 = Map112_Chr6 = Map112_Chr7 = 0;
  }

  Map112_IRQ_Enable = 0;
  Map112_IRQ_Cnt = 0;
  Map112_IRQ_Latch = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 112 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map112_Write( WORD wAddr, BYTE byData )
{
  NES_DWORD dwBankNum;

  /* Set ROM Banks */
  switch( wAddr & 0xE001 )
  {
    case 0x8000:
      Map112_Regs[0] = byData;
      Map112_Set_PPU_Banks();
      Map112_Set_CPU_Banks();
      break;

    case 0xA000:
      Map112_Regs[1] = byData;
      dwBankNum = Map112_Regs[1];
      switch ( Map112_Regs[0] & 0x07 )
      {
        case 0x02:
	  if ( NesHeader.byVRomSize > 0 )
	  {
	    dwBankNum &= 0xfe;
	    Map112_Chr01 = dwBankNum;
	    Map112_Set_PPU_Banks();
	  }
	  break;

        case 0x03:
	  if ( NesHeader.byVRomSize > 0 )
	  {
	    dwBankNum &= 0xfe;
	    Map112_Chr23 = dwBankNum;
	    Map112_Set_PPU_Banks();
	  }
	  break;

        case 0x04:
	  if ( NesHeader.byVRomSize > 0 )
          {
	    Map112_Chr4 = dwBankNum;
	    Map112_Set_PPU_Banks();
	  }
	  break;

        case 0x05:
	  if ( NesHeader.byVRomSize > 0 )
          {
	    Map112_Chr5 = dwBankNum;
	    Map112_Set_PPU_Banks();
	  }
	  break;

        case 0x06:
	  if ( NesHeader.byVRomSize > 0 )
          {
	    Map112_Chr6 = dwBankNum;
	    Map112_Set_PPU_Banks();
          }
	  break;

        case 0x07:
	  if ( NesHeader.byVRomSize > 0 )
          {
	    Map112_Chr7 = dwBankNum;
	    Map112_Set_PPU_Banks();
	  }
	  break;

        case 0x00:
	  Map112_Prg0 = dwBankNum;
	  Map112_Set_CPU_Banks();
	  break;

        case 0x01:
	  Map112_Prg1 = dwBankNum;
	  Map112_Set_CPU_Banks();
	  break;
      }
      break;

    case 0x8001:
      Map112_Regs[2] = byData;
      if ( !ROM_FourScr )
      {
	if ( byData & 0x01 )
        {
	  InfoNES_Mirroring( 1 );   /* Vertical */
	}
	else
        {
	  InfoNES_Mirroring( 0 );   /* Horizontal */
	}
      }
      break;

    case 0xA001:
      Map112_Regs[3] = byData;
      break;

    case 0xC000:
      Map112_Regs[4] = byData;
      Map112_IRQ_Cnt = Map112_Regs[4];
      break;

    case 0xC001:
      Map112_Regs[5] = byData;
      Map112_IRQ_Latch = Map112_Regs[5];
      break;

    case 0xE000:
      Map112_Regs[6] = byData;
      Map112_IRQ_Enable = 0;

      if ( byData )
      {
	InfoNES_Mirroring( 0 );   /* Horizontal */
      }
      else
      {
	InfoNES_Mirroring( 1 );   /* Vertical */
      }
      break;

    case 0xE001:
      Map112_Regs[7] = byData;
      Map112_IRQ_Enable = 1;
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 112 H-Sync Function                                       */
/*-------------------------------------------------------------------*/
void Map112_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( Map112_IRQ_Enable )
  {
    if ( /* 0 <= PPU_Scanline && */ PPU_Scanline <= 239 )
    {
      if ( PPU_R1 & R1_SHOW_SCR || PPU_R1 & R1_SHOW_SP )
      {
        if ( !( Map112_IRQ_Cnt-- ) )
        {
          Map112_IRQ_Cnt = Map112_IRQ_Latch;
          IRQ_REQ;
        }
      }
    }
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 112 Set CPU Banks Function                                */
/*-------------------------------------------------------------------*/
void Map112_Set_CPU_Banks()
{
  /* Set ROM Banks */
  if ( Map112_Prg_Swap() )
  {
    ROMBANK0 = ROMLASTPAGE( 1 );
    ROMBANK1 = ROMPAGE( Map112_Prg1 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMPAGE( Map112_Prg0 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK3 = ROMLASTPAGE( 0 );
  }
  else
  {
    ROMBANK0 = ROMPAGE( Map112_Prg0 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK1 = ROMPAGE( Map112_Prg1 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMLASTPAGE( 1 );
    ROMBANK3 = ROMLASTPAGE( 0 );
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 112 Set PPU Banks Function                                */
/*-------------------------------------------------------------------*/
void Map112_Set_PPU_Banks()
{
  if ( NesHeader.byVRomSize > 0 )
  {
    if ( Map112_Chr_Swap() )
    {
      PPUBANK[ 0 ] = VROMPAGE( Map112_Chr4 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( Map112_Chr5 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( Map112_Chr6 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( Map112_Chr7 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( ( Map112_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( ( Map112_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( ( Map112_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( ( Map112_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    } 
    else 
    {
      PPUBANK[ 0 ] = VROMPAGE( ( Map112_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( Map112_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( ( Map112_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( Map112_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( Map112_Chr4 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( Map112_Chr5 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( Map112_Chr6 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( Map112_Chr7 % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    }
  }
}
