/*===================================================================*/
/*                                                                   */
/*                Mapper 68 (Sunsoft Mapper #4)                      */
/*                                                                   */
/*===================================================================*/

BYTE Map68_Regs[4];

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 68                                              */
/*-------------------------------------------------------------------*/
void Map68_Init()
{
  /* Initialize Mapper */
  MapperInit = Map68_Init;

  /* Write to Mapper */
  MapperWrite = Map68_Write;

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

  /* Initialize state flag */
  for ( int i = 0; i < 4; i++ )
    Map68_Regs[ i ] = 0x00;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 68 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map68_Write( WORD wAddr, BYTE byData )
{
  switch( wAddr )
  {
    case 0x8000:  
      /* Set PPU Banks */
      byData %= ( NesHeader.byVRomSize << 2 );
      byData <<= 1;
      PPUBANK[ 0 ] = VROMPAGE( byData );
      PPUBANK[ 1 ] = VROMPAGE( byData + 1);
      InfoNES_SetupChr();
      break;

    case 0x9000:  
      /* Set PPU Banks */
      byData %= ( NesHeader.byVRomSize << 2 );
      byData <<= 1;
      PPUBANK[ 2 ] = VROMPAGE( byData );
      PPUBANK[ 3 ] = VROMPAGE( byData + 1);
      InfoNES_SetupChr();
      break;

    case 0xA000:  
      /* Set PPU Banks */
      byData %= ( NesHeader.byVRomSize << 2 );
      byData <<= 1;
      PPUBANK[ 4 ] = VROMPAGE( byData );
      PPUBANK[ 5 ] = VROMPAGE( byData + 1);
      InfoNES_SetupChr();
      break;

    case 0xB000:  
      /* Set PPU Banks */
      byData %= ( NesHeader.byVRomSize << 2 );
      byData <<= 1;
      PPUBANK[ 6 ] = VROMPAGE( byData );
      PPUBANK[ 7 ] = VROMPAGE( byData + 1);
      InfoNES_SetupChr();
      break;

    case 0xC000:  
      Map68_Regs[ 2 ] = byData;
      Map68_SyncMirror();
      break;

    case 0xD000:  
      Map68_Regs[ 3 ] = byData;
      Map68_SyncMirror();
      break;

    case 0xE000:  
      Map68_Regs[ 0 ] = ( byData & 0x10 ) >> 4;
      Map68_Regs[ 1 ] = byData & 0x03;
      Map68_SyncMirror();
      break;

    case 0xF000:  
      /* Set ROM Banks */
      byData %= NesHeader.byRomSize;
      byData <<= 1;
      ROMBANK0 = ROMPAGE( byData );
      ROMBANK1 = ROMPAGE( byData + 1 );
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 68 Sub Function                                           */
/*-------------------------------------------------------------------*/
void Map68_SyncMirror( void )
{
  if ( Map68_Regs[ 0 ] )
  {
    switch( Map68_Regs[ 1 ] )
    {
      case 0x00:
        PPUBANK[  8 ] = VROMPAGE( Map68_Regs[ 2 ] + 0x80 );
        PPUBANK[  9 ] = VROMPAGE( Map68_Regs[ 3 ] + 0x80 );
        PPUBANK[ 10 ] = VROMPAGE( Map68_Regs[ 2 ] + 0x80 );
        PPUBANK[ 11 ] = VROMPAGE( Map68_Regs[ 3 ] + 0x80 );
        break;

      case 0x01:
        PPUBANK[  8 ] = VROMPAGE( Map68_Regs[ 2 ] + 0x80 );
        PPUBANK[  9 ] = VROMPAGE( Map68_Regs[ 2 ] + 0x80 );
        PPUBANK[ 10 ] = VROMPAGE( Map68_Regs[ 3 ] + 0x80 );
        PPUBANK[ 11 ] = VROMPAGE( Map68_Regs[ 3 ] + 0x80 );
        break;

      case 0x02:
        PPUBANK[  8 ] = VROMPAGE( Map68_Regs[ 2 ] + 0x80 );
        PPUBANK[  9 ] = VROMPAGE( Map68_Regs[ 2 ] + 0x80 );
        PPUBANK[ 10 ] = VROMPAGE( Map68_Regs[ 2 ] + 0x80 );
        PPUBANK[ 11 ] = VROMPAGE( Map68_Regs[ 2 ] + 0x80 );
        break;

      case 0x03:
        PPUBANK[  8 ] = VROMPAGE( Map68_Regs[ 3 ] + 0x80 );
        PPUBANK[  9 ] = VROMPAGE( Map68_Regs[ 3 ] + 0x80 );
        PPUBANK[ 10 ] = VROMPAGE( Map68_Regs[ 3 ] + 0x80 );
        PPUBANK[ 11 ] = VROMPAGE( Map68_Regs[ 3 ] + 0x80 );
        break;
    }
    InfoNES_SetupChr();
  } 
  else 
  {
    InfoNES_Mirroring( Map68_Regs[ 1 ] );
  }
}
