/*===================================================================*/
/*                                                                   */
/*                     Mapper 255 : 110-in-1                         */
/*                                                                   */
/*===================================================================*/

BYTE    Map255_Reg[4];

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 255                                            */
/*-------------------------------------------------------------------*/
void Map255_Init()
{
  /* Initialize Mapper */
  MapperInit = Map255_Init;

  /* Write to Mapper */
  MapperWrite = Map255_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map255_Apu;

  /* Read from APU */
  MapperReadApu = Map255_ReadApu;

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

  /* Set Registers */
  InfoNES_Mirroring( 1 );

  Map255_Reg[0] = 0;
  Map255_Reg[1] = 0;
  Map255_Reg[2] = 0;
  Map255_Reg[3] = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 255 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map255_Write( WORD wAddr, BYTE byData )
{
  BYTE	byPrg = (wAddr & 0x0F80)>>7;
  int	nChr = (wAddr & 0x003F);
  int	nBank = (wAddr & 0x4000)>>14;

  if( wAddr & 0x2000 ) {
    InfoNES_Mirroring( 0 );
  } else {
    InfoNES_Mirroring( 1 );
  }

  if( wAddr & 0x1000 ) {
    if( wAddr & 0x0040 ) {
      ROMBANK0 = ROMPAGE((0x80*nBank+byPrg*4+2) % (NesHeader.byRomSize<<1));
      ROMBANK1 = ROMPAGE((0x80*nBank+byPrg*4+3) % (NesHeader.byRomSize<<1));
      ROMBANK2 = ROMPAGE((0x80*nBank+byPrg*4+2) % (NesHeader.byRomSize<<1));
      ROMBANK3 = ROMPAGE((0x80*nBank+byPrg*4+3) % (NesHeader.byRomSize<<1));
    } else {
      ROMBANK0 = ROMPAGE((0x80*nBank+byPrg*4+0) % (NesHeader.byRomSize<<1));
      ROMBANK1 = ROMPAGE((0x80*nBank+byPrg*4+1) % (NesHeader.byRomSize<<1));
      ROMBANK2 = ROMPAGE((0x80*nBank+byPrg*4+0) % (NesHeader.byRomSize<<1));
      ROMBANK3 = ROMPAGE((0x80*nBank+byPrg*4+1) % (NesHeader.byRomSize<<1));
    }
  } else {
    ROMBANK0 = ROMPAGE((0x80*nBank+byPrg*4+0) % (NesHeader.byRomSize<<1));
    ROMBANK1 = ROMPAGE((0x80*nBank+byPrg*4+1) % (NesHeader.byRomSize<<1));
    ROMBANK2 = ROMPAGE((0x80*nBank+byPrg*4+2) % (NesHeader.byRomSize<<1));
    ROMBANK3 = ROMPAGE((0x80*nBank+byPrg*4+3) % (NesHeader.byRomSize<<1));
  }
  
  PPUBANK[ 0 ] = VROMPAGE((0x200*nBank+nChr*8+0) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 1 ] = VROMPAGE((0x200*nBank+nChr*8+1) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 2 ] = VROMPAGE((0x200*nBank+nChr*8+2) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 3 ] = VROMPAGE((0x200*nBank+nChr*8+3) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 4 ] = VROMPAGE((0x200*nBank+nChr*8+4) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 5 ] = VROMPAGE((0x200*nBank+nChr*8+5) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 6 ] = VROMPAGE((0x200*nBank+nChr*8+6) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 7 ] = VROMPAGE((0x200*nBank+nChr*8+7) % (NesHeader.byVRomSize<<3));
}

/*-------------------------------------------------------------------*/
/*  Mapper 255 Write to APU Function                                 */
/*-------------------------------------------------------------------*/
void Map255_Apu( WORD wAddr, BYTE byData )
{
  if( wAddr >= 0x5800 ) {
    Map255_Reg[wAddr&0x0003] = byData & 0x0F;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 255 Read from APU Function                                */
/*-------------------------------------------------------------------*/
BYTE Map255_ReadApu( WORD wAddr )
{
  if( wAddr >= 0x5800 ) {
    return	Map255_Reg[wAddr&0x0003] & 0x0F;
  } else {
    return	wAddr>>8;
  }
}
