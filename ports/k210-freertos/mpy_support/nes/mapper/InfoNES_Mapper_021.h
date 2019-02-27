/*===================================================================*/
/*                                                                   */
/*                   Mapper 21 (Konami VRC4 2A)                      */
/*                                                                   */
/*===================================================================*/

BYTE Map21_Regs[ 10 ];
BYTE Map21_IRQ_Enable;
BYTE Map21_IRQ_Cnt;
BYTE Map21_IRQ_Latch;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 21                                             */
/*-------------------------------------------------------------------*/
void Map21_Init()
{
  /* Initialize Mapper */
  MapperInit = Map21_Init;

  /* Write to Mapper */
  MapperWrite = Map21_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map21_HSync;

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

  /* Initialize State Registers */
  for ( int nPage = 0; nPage < 8; nPage++ )
  {
    Map21_Regs[ nPage ] = nPage;
  }
  Map21_Regs[ 8 ] = 0;

  Map21_IRQ_Enable = 0;
  Map21_IRQ_Cnt = 0;
  Map21_IRQ_Latch = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 21 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map21_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr & 0xf00f )
  {
    /* Set ROM Banks */
    case 0x8000:
      if ( Map21_Regs[ 8 ] & 0x02 )
      {
        byData %= ( NesHeader.byRomSize << 1 );
        ROMBANK2 = ROMPAGE( byData );
      } else {
        byData %= ( NesHeader.byRomSize << 1 );
        ROMBANK0 = ROMPAGE( byData );
      }
      break;

    case 0xa000:
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK1 = ROMPAGE( byData );
      break;

    /* Name Table Mirroring */
    case 0x9000:
      switch ( byData & 0x03 )
      {
        case 0:
          InfoNES_Mirroring( 1 );
          break;

        case 1:
          InfoNES_Mirroring( 0 );
          break;
       
        case 2:
          InfoNES_Mirroring( 3 );
          break;

        case 3:
          InfoNES_Mirroring( 2 );
          break; 
      }
      break;

    case 0x9002:
      Map21_Regs[ 8 ] = byData;
      break;

    case 0xb000:
      Map21_Regs[ 0 ] = ( Map21_Regs[ 0 ] & 0xf0 ) | ( byData & 0x0f );
      PPUBANK[ 0 ] = VROMPAGE( Map21_Regs[ 0 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xb002:
      Map21_Regs[ 0 ] = ( Map21_Regs[ 0 ] & 0x0f ) | ( ( byData & 0x0f ) << 4 );
      PPUBANK[ 0 ] = VROMPAGE( Map21_Regs[ 0 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xb001:
    case 0xb004:
      Map21_Regs[ 1 ] = ( Map21_Regs[ 1 ] & 0xf0 ) | ( byData & 0x0f );
      PPUBANK[ 1 ] = VROMPAGE( Map21_Regs[ 1 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xb003:
    case 0xb006:
      Map21_Regs[ 1 ] = ( Map21_Regs[ 1 ] & 0x0f ) | ( ( byData & 0x0f ) << 4 );
      PPUBANK[ 1 ] = VROMPAGE( Map21_Regs[ 1 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xc000:
      Map21_Regs[ 2 ] = ( Map21_Regs[ 2 ] & 0xf0 ) | ( byData & 0x0f );
      PPUBANK[ 2 ] = VROMPAGE( Map21_Regs[ 2 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xc002:
      Map21_Regs[ 2 ] = ( Map21_Regs[ 2 ] & 0x0f ) | ( ( byData & 0x0f ) << 4 );
      PPUBANK[ 2 ] = VROMPAGE( Map21_Regs[ 2 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xc001:
    case 0xc004:
      Map21_Regs[ 3 ] = ( Map21_Regs[ 3 ] & 0xf0 ) | ( byData & 0x0f );
      PPUBANK[ 3 ] = VROMPAGE( Map21_Regs[ 3 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xc003:
    case 0xc006:
      Map21_Regs[ 3 ] = ( Map21_Regs[ 3 ] & 0x0f ) | ( ( byData & 0x0f ) << 4 );
      PPUBANK[ 3 ] = VROMPAGE( Map21_Regs[ 3 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xd000:
      Map21_Regs[ 4 ] = ( Map21_Regs[ 4 ] & 0xf0 ) | ( byData & 0x0f );
      PPUBANK[ 4 ] = VROMPAGE( Map21_Regs[ 4 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xd002:
      Map21_Regs[ 4 ] = ( Map21_Regs[ 4 ] & 0x0f ) | ( ( byData & 0x0f ) << 4 );
      PPUBANK[ 4 ] = VROMPAGE( Map21_Regs[ 4 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xd001:
    case 0xd004:
      Map21_Regs[ 5 ] = ( Map21_Regs[ 5 ] & 0xf0 ) | ( byData & 0x0f );
      PPUBANK[ 5 ] = VROMPAGE( Map21_Regs[ 5 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xd003:
    case 0xd006:
      Map21_Regs[ 5 ] = ( Map21_Regs[ 5 ] & 0x0f ) | ( ( byData & 0x0f ) << 4 );
      PPUBANK[ 5 ] = VROMPAGE( Map21_Regs[ 5 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xe000:
      Map21_Regs[ 6 ] = ( Map21_Regs[ 6 ] & 0xf0 ) | ( byData & 0x0f );
      PPUBANK[ 6 ] = VROMPAGE( Map21_Regs[ 6 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xe002:
      Map21_Regs[ 6 ] = ( Map21_Regs[ 6 ] & 0x0f ) | ( ( byData & 0x0f ) << 4 );
      PPUBANK[ 6 ] = VROMPAGE( Map21_Regs[ 6 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xe001:
    case 0xe004:
      Map21_Regs[ 7 ] = ( Map21_Regs[ 7 ] & 0xf0 ) | ( byData & 0x0f );
      PPUBANK[ 7 ] = VROMPAGE( Map21_Regs[ 7 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xe003:
    case 0xe006:
      Map21_Regs[ 7 ] = ( Map21_Regs[ 7 ] & 0x0f ) | ( ( byData & 0x0f ) << 4 );
      PPUBANK[ 7 ] = VROMPAGE( Map21_Regs[ 7 ] % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xf000:
      Map21_IRQ_Latch = ( Map21_IRQ_Latch & 0xf0 ) | ( byData & 0x0f );
      break;

    case 0xf002:
      Map21_IRQ_Latch = ( Map21_IRQ_Latch & 0x0f ) | ( ( byData & 0x0f ) << 4 );
      break;

    case 0xf003:
      if ( Map21_IRQ_Enable & 0x01 )
      {
        Map21_IRQ_Enable |= 0x02;
      } else {
        Map21_IRQ_Enable &= 0x01;
      }
      break;

    case 0xf004:
      Map21_IRQ_Enable = byData & 0x03;
      if ( Map21_IRQ_Enable & 0x02 )
      {
        Map21_IRQ_Cnt = Map21_IRQ_Latch;
      }
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 21 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map21_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( Map21_IRQ_Enable & 0x02 )
  {
    if ( Map21_IRQ_Cnt == 0xff )
    {
      Map21_IRQ_Cnt = Map21_IRQ_Latch;
      
      if ( Map21_IRQ_Enable & 0x01 )
      {
        Map21_IRQ_Enable |= 0x02;
      } else {
        Map21_IRQ_Enable &= 0x01;
      }
      IRQ_REQ;
    } else {
      Map21_IRQ_Cnt++;
    }
  }
}
