/*===================================================================*/
/*                                                                   */
/*                 Mapper 183 : Gimmick (Bootleg)                    */
/*                                                                   */
/*===================================================================*/

BYTE	Map183_Reg[8];
BYTE	Map183_IRQ_Enable;
int	Map183_IRQ_Counter;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 183                                            */
/*-------------------------------------------------------------------*/
void Map183_Init()
{
  /* Initialize Mapper */
  MapperInit = Map183_Init;

  /* Write to Mapper */
  MapperWrite = Map183_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map183_HSync;

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
  if ( NesHeader.byVRomSize > 0 ) {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Initialize Registers */
  for( int i = 0; i < 8; i++ ) {
    Map183_Reg[i] = i;
  }
  Map183_IRQ_Enable = 0;
  Map183_IRQ_Counter = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 183 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map183_Write( WORD wAddr, BYTE byData )
{
  switch( wAddr ) {
  case	0x8800:
    ROMBANK0 = ROMPAGE( byData % (NesHeader.byRomSize << 1) );
    break;
  case	0xA800:
    ROMBANK1 = ROMPAGE( byData % (NesHeader.byRomSize << 1) );
    break;
  case	0xA000:
    ROMBANK2 = ROMPAGE( byData % (NesHeader.byRomSize << 1) );
    break;
    
  case	0xB000:
    Map183_Reg[0] = (Map183_Reg[0]&0xF0)|(byData&0x0F);
    PPUBANK[ 0 ] = VROMPAGE( Map183_Reg[0] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xB004:
    Map183_Reg[0] = (Map183_Reg[0]&0x0F)|((byData&0x0F)<<4);
    PPUBANK[ 0 ] = VROMPAGE( Map183_Reg[0] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xB008:
    Map183_Reg[1] = (Map183_Reg[1]&0xF0)|(byData&0x0F);
    PPUBANK[ 1 ] = VROMPAGE( Map183_Reg[1] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xB00C:
    Map183_Reg[1] = (Map183_Reg[1]&0x0F)|((byData&0x0F)<<4);
    PPUBANK[ 1 ] = VROMPAGE( Map183_Reg[1] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
    
  case	0xC000:
    Map183_Reg[2] = (Map183_Reg[2]&0xF0)|(byData&0x0F);
    PPUBANK[ 2 ] = VROMPAGE( Map183_Reg[2] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xC004:
    Map183_Reg[2] = (Map183_Reg[2]&0x0F)|((byData&0x0F)<<4);
    PPUBANK[ 2 ] = VROMPAGE( Map183_Reg[2] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xC008:
    Map183_Reg[3] = (Map183_Reg[3]&0xF0)|(byData&0x0F);
    PPUBANK[ 3 ] = VROMPAGE( Map183_Reg[3] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xC00C:
    Map183_Reg[3] = (Map183_Reg[3]&0x0F)|((byData&0x0F)<<4);
    PPUBANK[ 3 ] = VROMPAGE( Map183_Reg[3] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
    
  case	0xD000:
    Map183_Reg[4] = (Map183_Reg[4]&0xF0)|(byData&0x0F);
    PPUBANK[ 4 ] = VROMPAGE( Map183_Reg[4] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xD004:
    Map183_Reg[4] = (Map183_Reg[4]&0x0F)|((byData&0x0F)<<4);
    PPUBANK[ 4 ] = VROMPAGE( Map183_Reg[4] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xD008:
    Map183_Reg[5] = (Map183_Reg[5]&0xF0)|(byData&0x0F);
    PPUBANK[ 5 ] = VROMPAGE( Map183_Reg[5] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xD00C:
    Map183_Reg[5] = (Map183_Reg[5]&0x0F)|((byData&0x0F)<<4);
    PPUBANK[ 5 ] = VROMPAGE( Map183_Reg[5] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
    
  case	0xE000:
    Map183_Reg[6] = (Map183_Reg[6]&0xF0)|(byData&0x0F);
    PPUBANK[ 6 ] = VROMPAGE( Map183_Reg[6] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xE004:
    Map183_Reg[6] = (Map183_Reg[6]&0x0F)|((byData&0x0F)<<4);
    PPUBANK[ 6 ] = VROMPAGE( Map183_Reg[6] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xE008:
    Map183_Reg[7] = (Map183_Reg[3]&0xF0)|(byData&0x0F);
    PPUBANK[ 7 ] = VROMPAGE( Map183_Reg[7] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
  case	0xE00C:
    Map183_Reg[7] = (Map183_Reg[3]&0x0F)|((byData&0x0F)<<4);
    PPUBANK[ 7 ] = VROMPAGE( Map183_Reg[7] % (NesHeader.byVRomSize << 3) );
    InfoNES_SetupChr();
    break;
    
  case	0x9008:
    if( byData == 1 ) {
      for( int i = 0; i < 8; i++ ) {
	Map183_Reg[i] = i;
      }
      /* Set ROM Banks */
      ROMBANK0 = ROMPAGE( 0 );
      ROMBANK1 = ROMPAGE( 1 );
      ROMBANK2 = ROMLASTPAGE( 1 );
      ROMBANK3 = ROMLASTPAGE( 0 );

      /* Set PPU Banks */
      if ( NesHeader.byVRomSize > 0 ) {
	PPUBANK[ 0 ] = VROMPAGE( 0 );
	PPUBANK[ 1 ] = VROMPAGE( 1 );
	PPUBANK[ 2 ] = VROMPAGE( 2 );
	PPUBANK[ 3 ] = VROMPAGE( 3 );
	PPUBANK[ 4 ] = VROMPAGE( 4 );
	PPUBANK[ 5 ] = VROMPAGE( 5 );
	PPUBANK[ 6 ] = VROMPAGE( 6 );
	PPUBANK[ 7 ] = VROMPAGE( 7 );
	InfoNES_SetupChr();
      }
    }
    break;
    
  case	0x9800:
    if( byData == 0 )      InfoNES_Mirroring( 1 );
    else if( byData == 1 ) InfoNES_Mirroring( 0 );
    else if( byData == 2 ) InfoNES_Mirroring( 2 );
    else if( byData == 3 ) InfoNES_Mirroring( 3 );
    break;
    
  case	0xF000:
    Map183_IRQ_Counter = (Map183_IRQ_Counter&0xFF00)|byData;
    break;
  case	0xF004:
    Map183_IRQ_Counter = (Map183_IRQ_Counter&0x00FF)|(byData<<8);
    break;
  case	0xF008:
    Map183_IRQ_Enable = byData;
    break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 183 H-Sync Function                                       */
/*-------------------------------------------------------------------*/
void Map183_HSync()
{
  if( Map183_IRQ_Enable & 0x02 ) {
    if( Map183_IRQ_Counter <= 113 ) {
      Map183_IRQ_Counter = 0;
      IRQ_REQ;
    } else {
      Map183_IRQ_Counter -= 113;
    }
  }
}
