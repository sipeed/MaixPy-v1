/*===================================================================*/
/*                                                                   */
/*                  Mapper 26 (Konami VRC 6V)                        */
/*                                                                   */
/*===================================================================*/

BYTE Map26_IRQ_Enable;
BYTE Map26_IRQ_Cnt;
BYTE Map26_IRQ_Latch;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 26                                             */
/*-------------------------------------------------------------------*/
void Map26_Init()
{
  /* Initialize Mapper */
  MapperInit = Map26_Init;

  /* Write to Mapper */
  MapperWrite = Map26_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map26_HSync;

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
  if ( NesHeader.byVRomSize > 0 )
  {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Initialize IRQ Registers */
  Map26_IRQ_Enable = 0;
  Map26_IRQ_Cnt = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 26 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map26_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    /* Set ROM Banks */
    case 0x8000:
      byData <<= 1;      
      byData %= ( NesHeader.byRomSize << 1 );
      
      ROMBANK0 = ROMPAGE( byData + 0 );
      ROMBANK1 = ROMPAGE( byData + 1 );
      break;

    /* Name Table Mirroring */
    case 0xb003:  
      switch ( byData & 0x7f )
      {
        case 0x08:
        case 0x2c:
          InfoNES_Mirroring( 2 );   /* One Screen 0x2400 */
          break;
        case 0x20:
          InfoNES_Mirroring( 1 );   /* Vertical */
          break;
        case 0x24:
          InfoNES_Mirroring( 0 );   /* Horizontal */
          break;
        case 0x28:
          InfoNES_Mirroring( 3 );   /* One Screen 0x2000 */
          break;
      }
      break;

    /* Set ROM Banks */
    case 0xc000:
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK2 = ROMPAGE( byData );      
      InfoNES_SetupChr();
      break;

    /* Set PPU Bank */
    case 0xd000:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 0 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xd001:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 2 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xd002:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 1 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xd003:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 3 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xe000:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 4 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xe001:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 6 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xe002:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 5 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xe003:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 7 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    /* Set IRQ Registers */
    case 0xf000:
      Map26_IRQ_Latch = byData;
      break;

    case 0xf001:
      Map26_IRQ_Enable = byData & 0x01;
      break;

    case 0xf002:
      Map26_IRQ_Enable = byData & 0x03;

      if ( Map26_IRQ_Enable & 0x02 )
      {
        Map26_IRQ_Cnt = Map26_IRQ_Latch;
      }
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 26 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map26_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( Map26_IRQ_Enable & 0x03 )
  {
    if ( Map26_IRQ_Cnt >= 0xfe )
    {
      IRQ_REQ;
      Map26_IRQ_Cnt = Map26_IRQ_Latch;
      Map26_IRQ_Enable = 0;
    } else {
      Map26_IRQ_Cnt++;
    }
  }
}
