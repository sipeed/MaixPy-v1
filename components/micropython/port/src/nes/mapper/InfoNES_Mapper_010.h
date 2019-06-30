/*===================================================================*/
/*                                                                   */
/*                     Mapper 10 (MMC4)                              */
/*                                                                   */
/*===================================================================*/

struct Map10_Latch 
{
  BYTE lo_bank;
  BYTE hi_bank;
  BYTE state;
};

struct Map10_Latch latch3;    // Latch Selector #1
struct Map10_Latch latch4;    // Latch Selector #2

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 10                                             */
/*-------------------------------------------------------------------*/
void Map10_Init()
{
  int nPage;

  /* Initialize Mapper */
  MapperInit = Map10_Init;

  /* Write to Mapper */
  MapperWrite = Map10_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map0_HSync;

  /* Callback at PPU */
  MapperPPU = Map10_PPU;

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
    for ( nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Init Latch Selector */
  latch3.state = 0xfe;
  latch3.lo_bank = 0;
  latch3.hi_bank = 0;
  latch4.state = 0xfe;
  latch4.lo_bank = 0;
  latch4.hi_bank = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 );
}

/*-------------------------------------------------------------------*/
/*  Mapper 10 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map10_Write( WORD wAddr, BYTE byData )
{
  WORD wMapAddr;

  wMapAddr = wAddr & 0xf000;
  switch ( wMapAddr )
  {
    case 0xa000:
      /* Set ROM Banks */
      byData %= NesHeader.byRomSize;
      byData <<= 1;
      ROMBANK0 = ROMPAGE( byData );
      ROMBANK1 = ROMPAGE( byData + 1 );
      break;

    case 0xb000:
      /* Number of 4K Banks to Number of 1K Banks */
      byData %= ( NesHeader.byVRomSize << 1 );
      byData <<= 2;

      /* Latch Control */
      latch3.lo_bank = byData;

      if (0xfd == latch3.state)
      {
        /* Set PPU Banks */
        PPUBANK[ 0 ] = VROMPAGE( byData );
        PPUBANK[ 1 ] = VROMPAGE( byData + 1 );
        PPUBANK[ 2 ] = VROMPAGE( byData + 2 );
        PPUBANK[ 3 ] = VROMPAGE( byData + 3 );     
        InfoNES_SetupChr();
      }
      break;

    case 0xc000:
      /* Number of 4K Banks to Number of 1K Banks */
      byData %= ( NesHeader.byVRomSize << 1 );
      byData <<= 2;

      /* Latch Control */
      latch3.hi_bank = byData;

      if (0xfe == latch3.state)
      {
        /* Set PPU Banks */
        PPUBANK[ 0 ] = VROMPAGE( byData );
        PPUBANK[ 1 ] = VROMPAGE( byData + 1 );
        PPUBANK[ 2 ] = VROMPAGE( byData + 2 );
        PPUBANK[ 3 ] = VROMPAGE( byData + 3 );     
        InfoNES_SetupChr();
      }
      break;

    case 0xd000:
      /* Number of 4K Banks to Number of 1K Banks */
      byData %= ( NesHeader.byVRomSize << 1 );
      byData <<= 2;

      /* Latch Control */
      latch4.lo_bank = byData;

      if (0xfd == latch4.state)
      {
        /* Set PPU Banks */
        PPUBANK[ 4 ] = VROMPAGE( byData );
        PPUBANK[ 5 ] = VROMPAGE( byData + 1 );
        PPUBANK[ 6 ] = VROMPAGE( byData + 2 );
        PPUBANK[ 7 ] = VROMPAGE( byData + 3 );    
        InfoNES_SetupChr();
      }
      break;

    case 0xe000:
      /* Number of 4K Banks to Number of 1K Banks */
      byData %= ( NesHeader.byVRomSize << 1 );
      byData <<= 2;

      /* Latch Control */
      latch4.hi_bank = byData;

      if (0xfe == latch4.state)
      {
        /* Set PPU Banks */
        PPUBANK[ 4 ] = VROMPAGE( byData );
        PPUBANK[ 5 ] = VROMPAGE( byData + 1 );
        PPUBANK[ 6 ] = VROMPAGE( byData + 2 );
        PPUBANK[ 7 ] = VROMPAGE( byData + 3 ); 
        InfoNES_SetupChr();
      }
      break;

    case 0xf000:
      /* Name Table Mirroring */
      InfoNES_Mirroring( byData & 0x01 ? 0 : 1);
      break;
  }  
}

/*-------------------------------------------------------------------*/
/*  Mapper 10 PPU Function                                           */
/*-------------------------------------------------------------------*/
void Map10_PPU( WORD wAddr )
{
  /* Control Latch Selector */ 
  switch ( wAddr & 0x3ff0 )
  {
    case 0x0fd0:
      /* Latch Control */
      latch3.state = 0xfd;
      /* Set PPU Banks */
      PPUBANK[ 0 ] = VROMPAGE( latch3.lo_bank );
      PPUBANK[ 1 ] = VROMPAGE( latch3.lo_bank + 1 );
      PPUBANK[ 2 ] = VROMPAGE( latch3.lo_bank + 2 );
      PPUBANK[ 3 ] = VROMPAGE( latch3.lo_bank + 3 );     
      InfoNES_SetupChr();
      break;

    case 0x0fe0:
      /* Latch Control */
      latch3.state = 0xfe;
      /* Set PPU Banks */
      PPUBANK[ 0 ] = VROMPAGE( latch3.hi_bank );
      PPUBANK[ 1 ] = VROMPAGE( latch3.hi_bank + 1 );
      PPUBANK[ 2 ] = VROMPAGE( latch3.hi_bank + 2 );
      PPUBANK[ 3 ] = VROMPAGE( latch3.hi_bank + 3 );     
      InfoNES_SetupChr();      
      break;

    case 0x1fd0:
      /* Latch Control */
      latch4.state = 0xfd;
      /* Set PPU Banks */
      PPUBANK[ 4 ] = VROMPAGE( latch4.lo_bank );
      PPUBANK[ 5 ] = VROMPAGE( latch4.lo_bank + 1 );
      PPUBANK[ 6 ] = VROMPAGE( latch4.lo_bank + 2 );
      PPUBANK[ 7 ] = VROMPAGE( latch4.lo_bank + 3 );     
      InfoNES_SetupChr();
      break;      

    case 0x1fe0:
      /* Latch Control */
      latch4.state = 0xfe;
      /* Set PPU Banks */
      PPUBANK[ 4 ] = VROMPAGE( latch4.hi_bank );
      PPUBANK[ 5 ] = VROMPAGE( latch4.hi_bank + 1 );
      PPUBANK[ 6 ] = VROMPAGE( latch4.hi_bank + 2 );
      PPUBANK[ 7 ] = VROMPAGE( latch4.hi_bank + 3 );     
      InfoNES_SetupChr();            
      break;
  }
}
