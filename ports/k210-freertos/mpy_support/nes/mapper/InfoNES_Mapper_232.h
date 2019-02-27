/*===================================================================*/
/*                                                                   */
/*                   Mapper 232 : Quattro Games                      */
/*                                                                   */
/*===================================================================*/

BYTE Map232_Regs[2];

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 232                                            */
/*-------------------------------------------------------------------*/
void Map232_Init()
{
  /* Initialize Mapper */
  MapperInit = Map232_Init;

  /* Write to Mapper */
  MapperWrite = Map232_Write;

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

  /* Initialize Registers */
  Map232_Regs[0] = 0x0C;
  Map232_Regs[1] = 0x00;
  
  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 232 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map232_Write( WORD wAddr, BYTE byData )
{
  if ( wAddr == 0x9000 ) {
    Map232_Regs[0] = (byData & 0x18)>>1;
  } else if ( 0xA000 <= wAddr /* && wAddr <= 0xFFFF */ ) {
    Map232_Regs[1] = byData & 0x03;
  }
  
  ROMBANK0= ROMPAGE((((Map232_Regs[0]|Map232_Regs[1])<<1)+0) % (NesHeader.byRomSize<<1));
  ROMBANK1= ROMPAGE((((Map232_Regs[0]|Map232_Regs[1])<<1)+1) % (NesHeader.byRomSize<<1));
  ROMBANK2= ROMPAGE((((Map232_Regs[0]|0x03)<<1)+0) % (NesHeader.byRomSize<<1));
  ROMBANK3= ROMPAGE((((Map232_Regs[0]|0x03)<<1)+1) % (NesHeader.byRomSize<<1));
}
