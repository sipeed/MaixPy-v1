/*===================================================================*/
/*                                                                   */
/*                  Mapper 22 (Konami VRC2 type A)                   */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 22                                             */
/*-------------------------------------------------------------------*/
void Map22_Init()
{
  /* Initialize Mapper */
  MapperInit = Map22_Init;

  /* Write to Mapper */
  MapperWrite = Map22_Write;

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

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 22 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map22_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x8000:
      /* Set ROM Banks */
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK0 = ROMPAGE( byData );
      break;

    case 0x9000:
      /* Name Table Mirroring */
      switch ( byData & 0x03 )
      {
        case 0:
          InfoNES_Mirroring( 1 );   /* Vertical */
          break;
        case 1:
          InfoNES_Mirroring( 0 );   /* Horizontal */
          break;
        case 2:
          InfoNES_Mirroring( 2 );   /* One Screen 0x2000 */
          break;
        case 3:
          InfoNES_Mirroring( 3 );   /* One Screen 0x2400 */
          break;
      }
      break;

    case 0xa000:
      /* Set ROM Banks */
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK1 = ROMPAGE( byData );
      break;
    
    case 0xb000:
      /* Set PPU Banks */
      byData >>= 1;
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 0 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xb001:
      /* Set PPU Banks */
      byData >>= 1;
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 1 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;
    
    case 0xc000:
      /* Set PPU Banks */
      byData >>= 1;
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 2 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xc001:
      /* Set PPU Banks */
      byData >>= 1;
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 3 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;
          
    case 0xd000:
      /* Set PPU Banks */
      byData >>= 1;
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 4 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xd001:
      /* Set PPU Banks */
      byData >>= 1;
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 5 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;
          
    case 0xe000:
      /* Set PPU Banks */
      byData >>= 1;
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 6 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    case 0xe001:
      /* Set PPU Banks */
      byData >>= 1;
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 7 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break; 
  }
}
