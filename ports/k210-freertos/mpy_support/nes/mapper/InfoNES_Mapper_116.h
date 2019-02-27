/*===================================================================*/
/*                                                                   */
/*   Mapper 116 : CartSaint : Yuu Yuu AV Kyoretsuden                 */
/*                                                                   */
/*===================================================================*/

BYTE	Map116_Reg[8];
BYTE	Map116_Prg0, Map116_Prg1, Map116_Prg2, Map116_Prg3;
BYTE	Map116_Prg0L, Map116_Prg1L;
BYTE	Map116_Chr0, Map116_Chr1, Map116_Chr2, Map116_Chr3;
BYTE    Map116_Chr4, Map116_Chr5, Map116_Chr6, Map116_Chr7;

BYTE	Map116_IRQ_Enable;
BYTE	Map116_IRQ_Counter;
BYTE	Map116_IRQ_Latch;

BYTE	Map116_ExPrgSwitch;
BYTE	Map116_ExChrSwitch;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 116                                            */
/*-------------------------------------------------------------------*/
void Map116_Init()
{
  /* Initialize Mapper */
  MapperInit = Map116_Init;

  /* Write to Mapper */
  MapperWrite = Map116_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map116_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Initialize Registers */
  for( int i = 0; i < 8; i++ ) {
    Map116_Reg[i] = 0x00;
  }

  Map116_Prg0 = Map116_Prg0L = 0;
  Map116_Prg1 = Map116_Prg1L = 1;
  Map116_Prg2 = ( NesHeader.byRomSize << 1 ) - 2;
  Map116_Prg3 = ( NesHeader.byRomSize << 1 ) - 1;

  Map116_ExPrgSwitch = 0;
  Map116_ExChrSwitch = 0;

  /* Set ROM Banks */
  Map116_Set_CPU_Banks();

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 ) {
    Map116_Chr0 = 0;
    Map116_Chr1 = 1;
    Map116_Chr2 = 2;
    Map116_Chr3 = 3;
    Map116_Chr4 = 4;
    Map116_Chr5 = 5;
    Map116_Chr6 = 6;
    Map116_Chr7 = 7;
    Map116_Set_PPU_Banks();
  } else {
    Map116_Chr0 = Map116_Chr2 = Map116_Chr4 = Map116_Chr5 = Map116_Chr6 = Map116_Chr7 = 0;
    Map116_Chr1 = Map116_Chr3 = 1;
  }

  Map116_IRQ_Enable = 0;	/* Disable */
  Map116_IRQ_Counter = 0;
  Map116_IRQ_Latch = 0;
  
  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 116 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map116_Write( WORD wAddr, BYTE byData )
{
  switch( wAddr & 0xE001 ) {
  case	0x8000:
    Map116_Reg[0] = byData;
    Map116_Set_CPU_Banks();
    Map116_Set_PPU_Banks();
    break;
  case	0x8001:
    Map116_Reg[1] = byData;
    switch( Map116_Reg[0] & 0x07 ) {
    case	0x00:
      Map116_Chr0 = byData & 0xFE;
      Map116_Chr1 = Map116_Chr0+1;
      Map116_Set_PPU_Banks();
      break;
    case	0x01:
      Map116_Chr2 = byData & 0xFE;
      Map116_Chr3 = Map116_Chr2+1;
      Map116_Set_PPU_Banks();
      break;
    case	0x02:
      Map116_Chr4 = byData;
      Map116_Set_PPU_Banks();
      break;
    case	0x03:
      Map116_Chr5 = byData;
      Map116_Set_PPU_Banks();
      break;
    case	0x04:
      Map116_Chr6 = byData;
      Map116_Set_PPU_Banks();
      break;
    case	0x05:
      Map116_Chr7 = byData;
      Map116_Set_PPU_Banks();
      break;
    case	0x06:
      Map116_Prg0 = byData;
      Map116_Set_CPU_Banks();
      break;
    case	0x07:
      Map116_Prg1 = byData;
      Map116_Set_CPU_Banks();
      break;
    }
    break;
  case	0xA000:
    Map116_Reg[2] = byData;
    if ( !ROM_FourScr ) {
      if( byData & 0x01 ) InfoNES_Mirroring( 0 );
      else		  InfoNES_Mirroring( 1 );
    }
    break;
  case	0xA001:
    Map116_Reg[3] = byData;
    break;
  case	0xC000:
    Map116_Reg[4] = byData;
    Map116_IRQ_Counter = byData;
    Map116_IRQ_Enable = 0xFF;
    break;
  case	0xC001:
    Map116_Reg[5] = byData;
    Map116_IRQ_Latch = byData;
    break;
  case	0xE000:
    Map116_Reg[6] = byData;
    Map116_IRQ_Enable = 0;
    break;
  case	0xE001:
    Map116_Reg[7] = byData;
    Map116_IRQ_Enable = 0xFF;
    break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 116 H-Sync Function                                       */
/*-------------------------------------------------------------------*/
void Map116_HSync()
{
  if( ( /* PPU_Scanline >= 0 && */ PPU_Scanline <= 239) ) {
    if( PPU_R1 & R1_SHOW_SCR || PPU_R1 & R1_SHOW_SP ) {
      if( Map116_IRQ_Enable ) {
	if( !(Map116_IRQ_Counter--) ) {
	  Map116_IRQ_Counter = Map116_IRQ_Latch;
	  IRQ_REQ;
	}
      }
    }
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 116 Set CPU Banks Function                                */
/*-------------------------------------------------------------------*/
void Map116_Set_CPU_Banks()
{
  if( Map116_Reg[0] & 0x40 ) {
    ROMBANK0 = ROMPAGE( ( NesHeader.byRomSize << 1 ) - 2 );
    ROMBANK1 = ROMPAGE( Map116_Prg1 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMPAGE( Map116_Prg0 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK3 = ROMPAGE( ( NesHeader.byRomSize << 1 ) - 1 );
  } else {
    ROMBANK0 = ROMPAGE( Map116_Prg0 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK1 = ROMPAGE( Map116_Prg1 % ( NesHeader.byRomSize << 1 ) );
    ROMBANK2 = ROMPAGE( ( NesHeader.byRomSize << 1 ) - 2 );
    ROMBANK3 = ROMPAGE( ( NesHeader.byRomSize << 1 ) - 1 );
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 116 Set PPU Banks Function                                */
/*-------------------------------------------------------------------*/
void Map116_Set_PPU_Banks()
{
  if ( NesHeader.byVRomSize > 0 ) {
    if( Map116_Reg[0] & 0x80 ) {
      PPUBANK[ 0 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr4) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 1 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr5) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 2 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr6) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 3 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr7) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 4 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr0) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 5 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr1) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 6 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr2) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 7 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr3) % (NesHeader.byVRomSize<<3));
      InfoNES_SetupChr();
    } else {
      PPUBANK[ 0 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr0) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 1 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr1) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 2 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr2) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 3 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr3) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 4 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr4) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 5 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr5) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 6 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr6) % (NesHeader.byVRomSize<<3));
      PPUBANK[ 7 ] = VROMPAGE( ((Map116_ExChrSwitch<<8)+Map116_Chr7) % (NesHeader.byVRomSize<<3));
      InfoNES_SetupChr();
    }
  }
}

