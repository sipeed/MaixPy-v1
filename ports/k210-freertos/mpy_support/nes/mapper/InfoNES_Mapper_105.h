/*===================================================================*/
/*                                                                   */
/*        Mapper 105 : Nintendo World Championship                   */
/*                                                                   */
/*===================================================================*/

BYTE	Map105_Init_State;
BYTE	Map105_Write_Count;
BYTE	Map105_Bits;
BYTE	Map105_Reg[4];

BYTE	Map105_IRQ_Enable;
int	Map105_IRQ_Counter;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 105                                            */
/*-------------------------------------------------------------------*/
void Map105_Init()
{
  /* Initialize Mapper */
  MapperInit = Map105_Init;

  /* Write to Mapper */
  MapperWrite = Map105_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map105_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( 0 );
  ROMBANK1 = ROMPAGE( 1 );
  ROMBANK2 = ROMPAGE( 2 );
  ROMBANK3 = ROMPAGE( 3 );

  /* Set PPU Banks */
  Map105_Reg[0] = 0x0C;
  Map105_Reg[1] = 0x00;
  Map105_Reg[2] = 0x00;
  Map105_Reg[3] = 0x10;
  
  Map105_Bits = 0;
  Map105_Write_Count = 0;
  
  Map105_IRQ_Counter = 0;
  Map105_IRQ_Enable = 0;
  Map105_Init_State = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 105 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map105_Write( WORD wAddr, BYTE byData )
{
  WORD reg_num = (wAddr & 0x7FFF) >> 13;

  if( byData & 0x80 ) {
    Map105_Bits = Map105_Write_Count = 0;
    if( reg_num == 0 ) {
      Map105_Reg[reg_num] |= 0x0C;
    }
  } else {
    Map105_Bits |= (byData & 1) << Map105_Write_Count++;
    if( Map105_Write_Count == 5) {
      Map105_Reg[reg_num] = Map105_Bits & 0x1F;
      Map105_Bits = Map105_Write_Count = 0;
    }
  }

  if( Map105_Reg[0] & 0x02 ) {
    if( Map105_Reg[0] & 0x01 ) {
      InfoNES_Mirroring( 0 );
    } else {
      InfoNES_Mirroring( 1 );
    }
  } else {
    if( Map105_Reg[0] & 0x01 ) {
      InfoNES_Mirroring( 3 );
    } else {
      InfoNES_Mirroring( 2 );
    }
  }
  
  switch( Map105_Init_State ) {
  case 0:
  case 1:
    Map105_Init_State++;
    break;

  case 2:
    if(Map105_Reg[1] & 0x08) {
      if (Map105_Reg[0] & 0x08) {
	if (Map105_Reg[0] & 0x04) {
	  ROMBANK0 = ROMPAGE( ((Map105_Reg[3] & 0x07) * 2 + 16) % ( NesHeader.byRomSize << 1 ) );
	  ROMBANK1 = ROMPAGE( ((Map105_Reg[3] & 0x07) * 2 + 17) % ( NesHeader.byRomSize << 1 ) );
	  ROMBANK2 = ROMPAGE( 30 % ( NesHeader.byRomSize << 1 ) );
	  ROMBANK3 = ROMPAGE( 31 % ( NesHeader.byRomSize << 1 ) );
	} else {
	  ROMBANK0 = ROMPAGE( 16 % ( NesHeader.byRomSize << 1 ) );
	  ROMBANK1 = ROMPAGE( 17 % ( NesHeader.byRomSize << 1 ) );
	  ROMBANK2 = ROMPAGE( ((Map105_Reg[3] & 0x07) * 2 + 16) % ( NesHeader.byRomSize << 1 ) );
	  ROMBANK3 = ROMPAGE( ((Map105_Reg[3] & 0x07) * 2 + 17) % ( NesHeader.byRomSize << 1 ) );
	}
      } else {
	ROMBANK0 = ROMPAGE( ((Map105_Reg[3] & 0x06) * 2 + 16) % ( NesHeader.byRomSize << 1 ) );
	ROMBANK1 = ROMPAGE( ((Map105_Reg[3] & 0x06) * 2 + 17) % ( NesHeader.byRomSize << 1 ) );
	ROMBANK2 = ROMPAGE( ((Map105_Reg[3] & 0x06) * 2 + 18) % ( NesHeader.byRomSize << 1 ) );
	ROMBANK3 = ROMPAGE( ((Map105_Reg[3] & 0x06) * 2 + 19) % ( NesHeader.byRomSize << 1 ) );
      }
    } else {
      ROMBANK0 = ROMPAGE( ((Map105_Reg[1] & 0x06) * 2 + 0) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK1 = ROMPAGE( ((Map105_Reg[1] & 0x06) * 2 + 1) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK2 = ROMPAGE( ((Map105_Reg[1] & 0x06) * 2 + 2) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK3 = ROMPAGE( ((Map105_Reg[1] & 0x06) * 2 + 3) % ( NesHeader.byRomSize << 1 ) );
    }
    
    if( Map105_Reg[1] & 0x10 ) {
      Map105_IRQ_Counter = 0;
      Map105_IRQ_Enable = 0;
    } else {
      Map105_IRQ_Enable = 1;
    }
    break;

  default:
    break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 105 H-Sync Function                                       */
/*-------------------------------------------------------------------*/
void Map105_HSync()
{
  if( !PPU_Scanline ) {
    if( Map105_IRQ_Enable ) {
      Map105_IRQ_Counter += 29781;
    }
    if( ((Map105_IRQ_Counter | 0x21FFFFFF) & 0x3E000000) == 0x3E000000 ) {
      IRQ_REQ;
    }
  }
}

