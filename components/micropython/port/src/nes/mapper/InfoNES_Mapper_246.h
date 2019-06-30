/*===================================================================*/
/*                                                                   */
/*                 Mapper 246 : Phone Serm Berm                      */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 246                                            */
/*-------------------------------------------------------------------*/
void Map246_Init()
{
  /* Initialize Mapper */
  MapperInit = Map246_Init;

  /* Write to Mapper */
  MapperWrite = Map0_Write;

  /* Write to SRAM */
  MapperSram = Map246_Sram;

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

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 246 Write to SRAM Function                                */
/*-------------------------------------------------------------------*/
void Map246_Sram( WORD wAddr, BYTE byData )
{
  if( wAddr>=0x6000 && wAddr<0x8000 ) {
    switch( wAddr ) {
    case 0x6000:
      ROMBANK0 = ROMPAGE(((byData<<2)+0) % (NesHeader.byRomSize<<1));
      break;
    case 0x6001:
      ROMBANK1 = ROMPAGE(((byData<<2)+1) % (NesHeader.byRomSize<<1));
      break;
    case 0x6002:
      ROMBANK2 = ROMPAGE(((byData<<2)+2) % (NesHeader.byRomSize<<1));
      break;
    case 0x6003: 
      ROMBANK3 = ROMPAGE(((byData<<2)+3) % (NesHeader.byRomSize<<1));
      break;
    case 0x6004:
      PPUBANK[ 0 ] = VROMPAGE(((byData<<1)+0) % (NesHeader.byVRomSize<<3)); 
      PPUBANK[ 1 ] = VROMPAGE(((byData<<1)+1) % (NesHeader.byVRomSize<<3)); 
      InfoNES_SetupChr();
      break;
    case 0x6005:
      PPUBANK[ 2 ] = VROMPAGE(((byData<<1)+0) % (NesHeader.byVRomSize<<3)); 
      PPUBANK[ 3 ] = VROMPAGE(((byData<<1)+1) % (NesHeader.byVRomSize<<3)); 
      InfoNES_SetupChr();
      break;
    case 0x6006:
      PPUBANK[ 4 ] = VROMPAGE(((byData<<1)+0) % (NesHeader.byVRomSize<<3)); 
      PPUBANK[ 5 ] = VROMPAGE(((byData<<1)+1) % (NesHeader.byVRomSize<<3)); 
      InfoNES_SetupChr();
      break;
    case 0x6007:
      PPUBANK[ 6 ] = VROMPAGE(((byData<<1)+0) % (NesHeader.byVRomSize<<3)); 
      PPUBANK[ 7 ] = VROMPAGE(((byData<<1)+1) % (NesHeader.byVRomSize<<3)); 
      InfoNES_SetupChr();
      break;
    default:
      SRAMBANK[wAddr&0x1FFF] = byData;
      break;
    }
  }
}
