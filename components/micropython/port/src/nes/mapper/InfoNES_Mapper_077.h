/*===================================================================*/
/*                                                                   */
/*                Mapper 77  (Irem Early Mapper #0)                  */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 77                                             */
/*-------------------------------------------------------------------*/
void Map77_Init()
{
  /* Initialize Mapper */
  MapperInit = Map77_Init;

  /* Write to Mapper */
  MapperWrite = Map77_Write;

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

  /* VRAM Write Enabled */
  byVramWriteEnable = 1;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 77 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map77_Write( WORD wAddr, BYTE byData )
{
  BYTE byRomBank = byData & 0x07;
  BYTE byChrBank = ( byData & 0xf0 ) >> 4;

  /* Set ROM Banks */
  byRomBank <<= 2;
  byRomBank %= ( NesHeader.byRomSize << 1 );

  ROMBANK0 = ROMPAGE( byRomBank );
  ROMBANK1 = ROMPAGE( byRomBank + 1 );
  ROMBANK2 = ROMPAGE( byRomBank + 2 );
  ROMBANK3 = ROMPAGE( byRomBank + 3 );

  /* Set PPU Banks */
  byChrBank <<= 1;
  byChrBank %= ( NesHeader.byVRomSize << 3 );

  PPUBANK[ 0 ] = VROMPAGE( byChrBank );
  PPUBANK[ 1 ] = VROMPAGE( byChrBank + 1 );
  InfoNES_SetupChr();
}
