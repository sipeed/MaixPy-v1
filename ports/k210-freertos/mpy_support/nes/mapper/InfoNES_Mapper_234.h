/*===================================================================*/
/*                                                                   */
/*                     Mapper 234 : Maxi-15                          */
/*                                                                   */
/*===================================================================*/

BYTE	Map234_Reg[2];

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 234                                            */
/*-------------------------------------------------------------------*/
void Map234_Init()
{
  /* Initialize Mapper */
  MapperInit = Map234_Init;

  /* Write to Mapper */
  MapperWrite = Map234_Write;

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
  ROMBANK2 = ROMPAGE( 2 );
  ROMBANK3 = ROMPAGE( 3 );

  /* Set Registers */
  Map234_Reg[0] = 0;
  Map234_Reg[1] = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 234 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map234_Write( WORD wAddr, BYTE byData )
{
  if( wAddr >= 0xFF80 && wAddr <= 0xFF9F ) {
    if( !Map234_Reg[0] ) {
      Map234_Reg[0] = byData;
      Map234_Set_Banks();
    }
  }
  
  if( wAddr >= 0xFFE8 && wAddr <= 0xFFF7 ) {
    Map234_Reg[1] = byData;
    Map234_Set_Banks();
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 234 Set Banks Function                                    */
/*-------------------------------------------------------------------*/
void Map234_Set_Banks()
{
  BYTE byPrg, byChr;

  if( Map234_Reg[0] & 0x80 ) {
    InfoNES_Mirroring( 0 );
  } else {
    InfoNES_Mirroring( 1 );
  }
  if( Map234_Reg[0] & 0x40 ) {
    byPrg = (Map234_Reg[0]&0x0E)|(Map234_Reg[1]&0x01);
    byChr = ((Map234_Reg[0]&0x0E)<<2)|((Map234_Reg[1]>>4)&0x07);
  } else {
    byPrg = Map234_Reg[0]&0x0F;
    byChr = ((Map234_Reg[0]&0x0F)<<2)|((Map234_Reg[1]>>4)&0x03);
  }
  
  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE(((byPrg<<2)+0) % (NesHeader.byRomSize<<1));
  ROMBANK1 = ROMPAGE(((byPrg<<2)+1) % (NesHeader.byRomSize<<1));
  ROMBANK2 = ROMPAGE(((byPrg<<2)+2) % (NesHeader.byRomSize<<1));
  ROMBANK3 = ROMPAGE(((byPrg<<2)+3) % (NesHeader.byRomSize<<1));

  /* Set PPU Banks */
  PPUBANK[ 0 ] = VROMPAGE(((byChr<<3)+0) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 1 ] = VROMPAGE(((byChr<<3)+1) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 2 ] = VROMPAGE(((byChr<<3)+2) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 3 ] = VROMPAGE(((byChr<<3)+3) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 4 ] = VROMPAGE(((byChr<<3)+4) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 5 ] = VROMPAGE(((byChr<<3)+5) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 6 ] = VROMPAGE(((byChr<<3)+6) % (NesHeader.byVRomSize<<3));
  PPUBANK[ 7 ] = VROMPAGE(((byChr<<3)+7) % (NesHeader.byVRomSize<<3));
  InfoNES_SetupChr();
}


