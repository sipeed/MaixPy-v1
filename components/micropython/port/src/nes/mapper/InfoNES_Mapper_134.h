/*===================================================================*/
/*                                                                   */
/*                          Mapper 134                               */
/*                                                                   */
/*===================================================================*/

BYTE    Map134_Cmd, Map134_Prg, Map134_Chr;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 134                                            */
/*-------------------------------------------------------------------*/
void Map134_Init()
{
  /* Initialize Mapper */
  MapperInit = Map134_Init;

  /* Write to Mapper */
  MapperWrite = Map0_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map134_Apu;

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
  ROMBANK2 = ROMPAGE( 2 );
  ROMBANK3 = ROMPAGE( 3 );

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 ) {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 134 Write to APU Function                                 */
/*-------------------------------------------------------------------*/
void Map134_Apu( WORD wAddr, BYTE byData )
{
  switch( wAddr & 0x4101 ) {
  case	0x4100:
    Map134_Cmd = byData & 0x07;
    break;
  case	0x4101:
    switch( Map134_Cmd ) {
    case 0:	
      Map134_Prg = 0;
      Map134_Chr = 3;
      break;
    case 4:
      Map134_Chr &= 0x3;
      Map134_Chr |= (byData & 0x07) << 2;
      break;
    case 5:
      Map134_Prg = byData & 0x07;
      break;
    case 6:
      Map134_Chr &= 0x1C;
      Map134_Chr |= byData & 0x3;
      break;
    case 7:
      if( byData & 0x01 ) InfoNES_Mirroring( 0 );
      else		  InfoNES_Mirroring( 1 );
      break;
    }
    break;
  }

  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( ((Map134_Prg<<2) + 0 ) % (NesHeader.byRomSize << 1) );
  ROMBANK1 = ROMPAGE( ((Map134_Prg<<2) + 1 ) % (NesHeader.byRomSize << 1) );
  ROMBANK2 = ROMPAGE( ((Map134_Prg<<2) + 2 ) % (NesHeader.byRomSize << 1) );
  ROMBANK3 = ROMPAGE( ((Map134_Prg<<2) + 3 ) % (NesHeader.byRomSize << 1) );
  
  /* Set PPU Banks */
  PPUBANK[ 0 ] = VROMPAGE( ((Map134_Chr<<3) + 0) % (NesHeader.byVRomSize << 3) );
  PPUBANK[ 1 ] = VROMPAGE( ((Map134_Chr<<3) + 1) % (NesHeader.byVRomSize << 3) );
  PPUBANK[ 2 ] = VROMPAGE( ((Map134_Chr<<3) + 2) % (NesHeader.byVRomSize << 3) );
  PPUBANK[ 3 ] = VROMPAGE( ((Map134_Chr<<3) + 3) % (NesHeader.byVRomSize << 3) );
  PPUBANK[ 4 ] = VROMPAGE( ((Map134_Chr<<3) + 4) % (NesHeader.byVRomSize << 3) );
  PPUBANK[ 5 ] = VROMPAGE( ((Map134_Chr<<3) + 5) % (NesHeader.byVRomSize << 3) );
  PPUBANK[ 6 ] = VROMPAGE( ((Map134_Chr<<3) + 6) % (NesHeader.byVRomSize << 3) );
  PPUBANK[ 7 ] = VROMPAGE( ((Map134_Chr<<3) + 7) % (NesHeader.byVRomSize << 3) );
  InfoNES_SetupChr();

  //Map134_Wram[ wAddr & 0x1fff ] = byData;
}
