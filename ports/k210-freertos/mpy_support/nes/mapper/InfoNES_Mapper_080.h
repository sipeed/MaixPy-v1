/*===================================================================*/
/*                                                                   */
/*                        Mapper 80 (X1-005)                         */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 80                                             */
/*-------------------------------------------------------------------*/
void Map80_Init()
{
  /* Initialize Mapper */
  MapperInit = Map80_Init;

  /* Write to Mapper */
  MapperWrite = Map0_Write;

  /* Write to SRAM */
  MapperSram = Map80_Sram;

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
/*  Mapper 80 Write to SRAM Function                                 */
/*-------------------------------------------------------------------*/
void Map80_Sram( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    /* Set PPU Banks */
    case 0x7ef0:
      byData &= 0x7f;
      byData %= ( NesHeader.byVRomSize << 3 );
      
      PPUBANK[ 0 ] = VROMPAGE( byData );
      PPUBANK[ 1 ] = VROMPAGE( byData + 1 );
      InfoNES_SetupChr();
      break;

    case 0x7ef1:
      byData &= 0x7f;
      byData %= ( NesHeader.byVRomSize << 3 );
      
      PPUBANK[ 2 ] = VROMPAGE( byData );
      PPUBANK[ 3 ] = VROMPAGE( byData + 1 );
      InfoNES_SetupChr();
      break;
  
    case 0x7ef2:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 4 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;      

    case 0x7ef3:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 5 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;  

    case 0x7ef4:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 6 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break; 

    case 0x7ef5:
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ 7 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break; 

    /* Name Table Mirroring */
    case 0x7ef6:
      if ( byData & 0x01 )
      {
        InfoNES_Mirroring( 1 );
      } else {
        InfoNES_Mirroring( 0 );
      }

    /* Set ROM Banks */
    case 0x7efa:
    case 0x7efb:
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK0 = ROMPAGE( byData );
      break;

    case 0x7efc:
    case 0x7efd:
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK1 = ROMPAGE( byData );
      break;

    case 0x7efe:
    case 0x7eff:
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK2 = ROMPAGE( byData );
      break;
  }
}
