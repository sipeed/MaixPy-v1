/*===================================================================*/
/*                                                                   */
/*                  Mapper 69 (Sunsoft FME-7)                        */
/*                                                                   */
/*===================================================================*/

BYTE  Map69_IRQ_Enable;
NES_DWORD Map69_IRQ_Cnt;
BYTE  Map69_Regs[ 1 ];

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 69                                             */
/*-------------------------------------------------------------------*/
void Map69_Init()
{
  /* Initialize Mapper */
  MapperInit = Map69_Init;

  /* Write to Mapper */
  MapperWrite = Map69_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map69_HSync;

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

  /* Initialize IRQ Reg */
  Map69_IRQ_Enable = 0;
  Map69_IRQ_Cnt    = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 69 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map69_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x8000:
      Map69_Regs[ 0 ] = byData & 0x0f;
      break;

    case 0xA000:
      switch ( Map69_Regs[ 0 ] )
      {
        /* Set PPU Banks */
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
          byData %= ( NesHeader.byVRomSize << 3 );
          PPUBANK[ Map69_Regs[ 0 ] ] = VROMPAGE( byData );
          InfoNES_SetupChr();
          break;

        /* Set ROM Banks */
#if 0
        case 0x08:
          if ( !( byData & 0x40 ) )
          {
            byData %= ( NesHeader.byRomSize << 1 );
            SRAMBANK = ROMPAGE( byData );
          }
          break;
#endif

        case 0x09:
          byData %= ( NesHeader.byRomSize << 1 );
          ROMBANK0 = ROMPAGE( byData );
          break;

        case 0x0a:
          byData %= ( NesHeader.byRomSize << 1 );
          ROMBANK1 = ROMPAGE( byData );
          break;

        case 0x0b:
          byData %= ( NesHeader.byRomSize << 1 );
          ROMBANK2 = ROMPAGE( byData );
          break;

        /* Name Table Mirroring */
        case 0x0c:  
          switch ( byData & 0x03 )
          {
            case 0:
              InfoNES_Mirroring( 1 );   /* Vertical */
              break;
            case 1:
              InfoNES_Mirroring( 0 );   /* Horizontal */
              break;
            case 2:
              InfoNES_Mirroring( 3 );   /* One Screen 0x2400 */
              break;
            case 3:
              InfoNES_Mirroring( 2 );   /* One Screen 0x2000 */
              break;
          }
          break;

        case 0x0d:
          Map69_IRQ_Enable = byData;
          break;

        case 0x0e:
          Map69_IRQ_Cnt = ( Map69_IRQ_Cnt & 0xff00) | (NES_DWORD)byData;
          break;

        case 0x0f:
          Map69_IRQ_Cnt = ( Map69_IRQ_Cnt & 0x00ff) | ( (NES_DWORD)byData << 8 );
          break;
      }
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 69 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map69_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( Map69_IRQ_Enable )
  {
    if ( Map69_IRQ_Cnt <= 113 )
    {
      IRQ_REQ;
      Map69_IRQ_Cnt = 0;
    } else {
      Map69_IRQ_Cnt -= 113;
    }
  }
}
