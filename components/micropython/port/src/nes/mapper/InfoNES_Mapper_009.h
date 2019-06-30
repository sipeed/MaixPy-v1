/*===================================================================*/
/*                                                                   */
/*                     Mapper 9 (MMC2)                               */
/*                                                                   */
/*===================================================================*/

struct Map9_Latch 
{
  BYTE lo_bank;
  BYTE hi_bank;
  BYTE state;
};

struct Map9_Latch latch1;
struct Map9_Latch latch2;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 9                                              */
/*-------------------------------------------------------------------*/
void Map9_Init()
{
  int nPage;

  /* Initialize Mapper */
  MapperInit = Map9_Init;

  /* Write to Mapper */
  MapperWrite = Map9_Write;

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
  MapperPPU = Map9_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( 0 );
  ROMBANK1 = ROMLASTPAGE( 2 );
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
  latch1.state = 0xfe;
  latch1.lo_bank = 0;
  latch1.hi_bank = 0;
  latch2.state = 0xfe;
  latch2.lo_bank = 0;
  latch2.hi_bank = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 );
}

/*-------------------------------------------------------------------*/
/*  Mapper 9 Write Function                                          */
/*-------------------------------------------------------------------*/
void Map9_Write( WORD wAddr, BYTE byData )
{
  WORD wMapAddr;

  wMapAddr = wAddr & 0xf000;
  switch ( wMapAddr )
  {
    case 0xa000:
      /* Set ROM Banks */
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK0 = ROMPAGE( byData );
      break;

    case 0xb000:
      /* Number of 4K Banks to Number of 1K Banks */
      byData %= ( NesHeader.byVRomSize << 1 );
      byData <<= 2;

      /* Latch Control */
      latch1.lo_bank = byData;

      if (0xfd == latch1.state)
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
      latch1.hi_bank = byData;

      if (0xfe == latch1.state)
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
      latch2.lo_bank = byData;

      if (0xfd == latch2.state)
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
      latch2.hi_bank = byData;

      if (0xfe == latch2.state)
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
/*  Mapper 9 PPU Function                                            */
/*-------------------------------------------------------------------*/
void Map9_PPU( WORD wAddr )
{
  /* Control Latch Selector */ 
  switch ( wAddr & 0x3ff0 )
  {
    case 0x0fd0:
      /* Latch Control */
      latch1.state = 0xfd;
      /* Set PPU Banks */
      PPUBANK[ 0 ] = VROMPAGE( latch1.lo_bank );
      PPUBANK[ 1 ] = VROMPAGE( latch1.lo_bank + 1 );
      PPUBANK[ 2 ] = VROMPAGE( latch1.lo_bank + 2 );
      PPUBANK[ 3 ] = VROMPAGE( latch1.lo_bank + 3 );     
      InfoNES_SetupChr();
      break;

    case 0x0fe0:
      /* Latch Control */
      latch1.state = 0xfe;
      /* Set PPU Banks */
      PPUBANK[ 0 ] = VROMPAGE( latch1.hi_bank );
      PPUBANK[ 1 ] = VROMPAGE( latch1.hi_bank + 1 );
      PPUBANK[ 2 ] = VROMPAGE( latch1.hi_bank + 2 );
      PPUBANK[ 3 ] = VROMPAGE( latch1.hi_bank + 3 );     
      InfoNES_SetupChr();      
      break;

    case 0x1fd0:
      /* Latch Control */
      latch2.state = 0xfd;
      /* Set PPU Banks */
      PPUBANK[ 4 ] = VROMPAGE( latch2.lo_bank );
      PPUBANK[ 5 ] = VROMPAGE( latch2.lo_bank + 1 );
      PPUBANK[ 6 ] = VROMPAGE( latch2.lo_bank + 2 );
      PPUBANK[ 7 ] = VROMPAGE( latch2.lo_bank + 3 );     
      InfoNES_SetupChr();
      break;      

    case 0x1fe0:
      /* Latch Control */
      latch2.state = 0xfe;
      /* Set PPU Banks */
      PPUBANK[ 4 ] = VROMPAGE( latch2.hi_bank );
      PPUBANK[ 5 ] = VROMPAGE( latch2.hi_bank + 1 );
      PPUBANK[ 6 ] = VROMPAGE( latch2.hi_bank + 2 );
      PPUBANK[ 7 ] = VROMPAGE( latch2.hi_bank + 3 );     
      InfoNES_SetupChr();            
      break;
  }
}
