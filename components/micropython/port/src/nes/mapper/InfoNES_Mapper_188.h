/*===================================================================*/
/*                                                                   */
/*                        Mapper 188 (Bandai)                        */
/*                                                                   */
/*===================================================================*/

BYTE Map188_Dummy[ 0x2000 ];

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 188                                            */
/*-------------------------------------------------------------------*/
void Map188_Init()
{
  /* Initialize Mapper */
  MapperInit = Map188_Init;

  /* Write to Mapper */
  MapperWrite = Map188_Write;

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
  SRAMBANK = Map188_Dummy;

  /* Set ROM Banks */
  if ( ( NesHeader.byRomSize << 1 ) > 16 )
  {
    ROMBANK0 = ROMPAGE( 0 );
    ROMBANK1 = ROMPAGE( 1 );
    ROMBANK2 = ROMPAGE( 14 );
    ROMBANK3 = ROMPAGE( 15 );
  } else {
    ROMBANK0 = ROMPAGE( 0 );
    ROMBANK1 = ROMPAGE( 1 );
    ROMBANK2 = ROMLASTPAGE( 1 );
    ROMBANK3 = ROMLASTPAGE( 0 );
  }

  /* Magic Code */
  Map188_Dummy[ 0 ] = 0x03;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 188 Write Function                                        */
/*-------------------------------------------------------------------*/
void Map188_Write( WORD wAddr, BYTE byData )
{
  /* Set ROM Banks */
  if ( byData )
  {
    if ( byData & 0x10 )
    {
      byData = ( byData & 0x07 ) << 1;
      ROMBANK0 = ROMPAGE( ( byData + 0 ) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK1 = ROMPAGE( ( byData + 1 ) % ( NesHeader.byRomSize << 1 ) );
    } else {
      byData <<= 1;
      ROMBANK0 = ROMPAGE( ( byData + 16 ) % ( NesHeader.byRomSize << 1 ) );
      ROMBANK1 = ROMPAGE( ( byData + 17 ) % ( NesHeader.byRomSize << 1 ) );
    }
  } 
  else 
  {
    if ( ( NesHeader.byRomSize << 1 ) == 0x10 )
    {
      ROMBANK0 = ROMPAGE( 14 );
      ROMBANK1 = ROMPAGE( 15 );
    } else {
      ROMBANK0 = ROMPAGE( 16 );
      ROMBANK1 = ROMPAGE( 17 );
    }
  }
}
