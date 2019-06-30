/*===================================================================*/
/*                                                                   */
/*               Mapper 245 : Yong Zhe Dou E Long                    */
/*                                                                   */
/*===================================================================*/

BYTE	Map245_Reg[8];
BYTE	Map245_Prg0, Map245_Prg1;
BYTE	Map245_Chr01, Map245_Chr23, Map245_Chr4, Map245_Chr5, Map245_Chr6, Map245_Chr7;
BYTE	Map245_WeSram;

BYTE	Map245_IRQ_Enable;
BYTE	Map245_IRQ_Counter;
BYTE	Map245_IRQ_Latch;
BYTE	Map245_IRQ_Request;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 245                                            */
/*-------------------------------------------------------------------*/
void Map245_Init()
{
  /* Initialize Mapper */
  MapperInit = Map245_Init;

  /* Write to Mapper */
  MapperWrite = Map245_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map245_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Set Registers */
  for( int i = 0; i < 8; i++ ) {
    Map245_Reg[i] = 0x00;
  }

  Map245_Prg0 = 0;
  Map245_Prg1 = 1;

  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( 0 );
  ROMBANK1 = ROMPAGE( 1 );
  ROMBANK2 = ROMLASTPAGE( 1 );
  ROMBANK3 = ROMLASTPAGE( 0 );

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 ) {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  Map245_WeSram  = 0;		// Disable
  Map245_IRQ_Enable = 0;	// Disable
  Map245_IRQ_Counter = 0;
  Map245_IRQ_Latch = 0;
  Map245_IRQ_Request = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 245 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map245_Write( WORD wAddr, BYTE byData )
{
  switch( wAddr&0xF7FF ) {
  case	0x8000:
    Map245_Reg[0] = byData;
    break;
  case	0x8001:
    Map245_Reg[1] = byData;
    switch( Map245_Reg[0] ) {
    case	0x00:
      Map245_Reg[3]=(byData & 2 )<<5;
      ROMBANK2 = ROMPAGE((0x3E|Map245_Reg[3]) % (NesHeader.byRomSize<<1));
      ROMBANK3 = ROMPAGE((0x3F|Map245_Reg[3]) % (NesHeader.byRomSize<<1));
      break;
    case	0x06:
      Map245_Prg0=byData;
      break;
    case	0x07:
      Map245_Prg1=byData;
      break;
    }
    ROMBANK0 = ROMPAGE((Map245_Prg0|Map245_Reg[3]) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE((Map245_Prg1|Map245_Reg[3]) % (NesHeader.byRomSize<<1));
    break;
  case	0xA000:
    Map245_Reg[2] = byData;
    if( !ROM_FourScr ) {
      if( byData & 0x01 ) InfoNES_Mirroring( 0 );
      else		  InfoNES_Mirroring( 1 );
    }
    break;
  case	0xA001:
    
    break;
  case	0xC000:
    Map245_Reg[4] = byData;
    Map245_IRQ_Counter = byData;
    Map245_IRQ_Request = 0;
    break;
  case	0xC001:
    Map245_Reg[5] = byData;
    Map245_IRQ_Latch = byData;
    Map245_IRQ_Request = 0;
    break;
  case	0xE000:
    Map245_Reg[6] = byData;
    Map245_IRQ_Enable = 0;
    Map245_IRQ_Request = 0;
    break;
  case	0xE001:
    Map245_Reg[7] = byData;
    Map245_IRQ_Enable = 1;
    Map245_IRQ_Request = 0;
    break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 245 H-Sync Function                                       */
/*-------------------------------------------------------------------*/
void Map245_HSync()
{
  if( ( /* PPU_Scanline >= 0 && */ PPU_Scanline <= 239) ) {
    if( PPU_R1 & R1_SHOW_SCR || PPU_R1 & R1_SHOW_SP ) {
      if( Map245_IRQ_Enable && !Map245_IRQ_Request ) {
	if( PPU_Scanline == 0 ) {
	  if( Map245_IRQ_Counter ) {
	    Map245_IRQ_Counter--;
	  }
	}
	if( !(Map245_IRQ_Counter--) ) {
	  Map245_IRQ_Request = 0xFF;
	  Map245_IRQ_Counter = Map245_IRQ_Latch;
	}
      }
    }
  }
  if( Map245_IRQ_Request ) {
    IRQ_REQ;
  }
}

#if 0
/*-------------------------------------------------------------------*/
/*  Mapper 245 Set CPU Banks Function                                */
/*-------------------------------------------------------------------*/
void SetBank_CPU()
{
  ROMBANK0 = ROMPAGE( Map245_Prg0 % ( NesHeader.byRomSize << 1 ) );
  ROMBANK1 = ROMPAGE( Map245_Prg1 % ( NesHeader.byRomSize << 1 ) );
  ROMBANK2 = ROMLASTPAGE( 1 );
  ROMBANK3 = ROMLASTPAGE( 0 );
}

/*-------------------------------------------------------------------*/
/*  Mapper 245 Set PPU Banks Function                                */
/*-------------------------------------------------------------------*/
void SetBank_PPU()
{
  if( NesHeader.byVRomSize > 0 ) {
    if( Map245_Reg[0] & 0x80 ) {
      PPUBANK[ 0 ] = VROMPAGE( Map245_Chr4 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( Map245_Chr5 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( Map245_Chr6 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( Map245_Chr7 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( ( Map245_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( ( Map245_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( ( Map245_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( ( Map245_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    } else {
      PPUBANK[ 0 ] = VROMPAGE( ( Map245_Chr01 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( Map245_Chr01 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 2 ] = VROMPAGE( ( Map245_Chr23 + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( Map245_Chr23 + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 4 ] = VROMPAGE( Map245_Chr4 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 5 ] = VROMPAGE( Map245_Chr5 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 6 ] = VROMPAGE( Map245_Chr6 % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 7 ] = VROMPAGE( Map245_Chr7 % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
    }
  } else {
    if( Map245_Reg[0] & 0x80 ) {
      PPUBANK[ 4 ] = CRAMPAGE( (Map245_Chr01+0)&0x07 );
      PPUBANK[ 5 ] = CRAMPAGE( (Map245_Chr01+1)&0x07 );
      PPUBANK[ 6 ] = CRAMPAGE( (Map245_Chr23+0)&0x07 );
      PPUBANK[ 7 ] = CRAMPAGE( (Map245_Chr23+1)&0x07 );
      PPUBANK[ 0 ] = CRAMPAGE( Map245_Chr4&0x07 );
      PPUBANK[ 1 ] = CRAMPAGE( Map245_Chr5&0x07 );
      PPUBANK[ 2 ] = CRAMPAGE( Map245_Chr6&0x07 );
      PPUBANK[ 3 ] = CRAMPAGE( Map245_Chr7&0x07 );
      InfoNES_SetupChr();
    } else {
      PPUBANK[ 0 ] = CRAMPAGE( (Map245_Chr01+0)&0x07 );
      PPUBANK[ 1 ] = CRAMPAGE( (Map245_Chr01+1)&0x07 );
      PPUBANK[ 2 ] = CRAMPAGE( (Map245_Chr23+0)&0x07 );
      PPUBANK[ 3 ] = CRAMPAGE( (Map245_Chr23+1)&0x07 );
      PPUBANK[ 4 ] = CRAMPAGE( Map245_Chr4&0x07 );
      PPUBANK[ 5 ] = CRAMPAGE( Map245_Chr5&0x07 );
      PPUBANK[ 6 ] = CRAMPAGE( Map245_Chr6&0x07 );
      PPUBANK[ 7 ] = CRAMPAGE( Map245_Chr7&0x07 );
      InfoNES_SetupChr();
    }
  }
}
#endif
