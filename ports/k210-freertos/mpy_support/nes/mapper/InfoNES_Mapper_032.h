/*===================================================================*/
/*                                                                   */
/*                       Mapper 32 (Irem G-101)                      */
/*                                                                   */
/*===================================================================*/

BYTE Map32_Saved;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 32                                             */
/*-------------------------------------------------------------------*/
void Map32_Init()
{
  /* Initialize Mapper */
  MapperInit = Map32_Init;

  /* Write to Mapper */
  MapperWrite = Map32_Write;

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

  /* Initialize state flag */
  Map32_Saved = 0x00;

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
/*  Mapper 32 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map32_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr & 0xf000 )
  {
    case 0x8000:
      /* Set ROM Banks */
      byData %= ( NesHeader.byRomSize << 1 );

      if ( Map32_Saved & 0x02 ) 
      {
        ROMBANK2 = ROMPAGE( byData );
      } else {
        ROMBANK0 = ROMPAGE( byData );
      }
      break;
      
    case 0x9000:
      Map32_Saved = byData;
      
      // Name Table Mirroring
      InfoNES_Mirroring( byData & 0x01 );
      break;

    case 0xa000:
      /* Set ROM Banks */
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK1 = ROMPAGE( byData );
      break;

    case 0xb000:
      /* Set PPU Banks */
      byData %= ( NesHeader.byVRomSize << 3 );
      PPUBANK[ wAddr & 0x0007 ] = VROMPAGE( byData );
      InfoNES_SetupChr();
      break;

    default:
      break;
  }
}
