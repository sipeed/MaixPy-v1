/*===================================================================*/
/*                                                                   */
/*                     Mapper 15 (100-in-1)                          */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 15                                             */
/*-------------------------------------------------------------------*/
void Map15_Init()
{
  /* Initialize Mapper */
  MapperInit = Map15_Init;

  /* Write to Mapper */
  MapperWrite = Map15_Write;

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

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 15 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map15_Write( WORD wAddr, BYTE byData )
{
  BYTE byBank;

  switch ( wAddr )
  {
    case 0x8000:
      /* Name Table Mirroring */
      InfoNES_Mirroring( byData & 0x20 ? 0 : 1);
      
      /* Set ROM Banks */
      byBank = byData & 0x1f;
      byBank %= ( NesHeader.byRomSize << 1 );
      byBank <<= 1;

      ROMBANK0 = ROMPAGE( byBank );
      ROMBANK1 = ROMPAGE( byBank + 1 );
      ROMBANK2 = ROMPAGE( byBank + 2 );
      ROMBANK3 = ROMPAGE( byBank + 3 );
      break;

    case 0x8001:
      /* Set ROM Banks */
      byData &= 0x3f;
      byData %= ( NesHeader.byRomSize << 1 );
      byData <<= 1;

      ROMBANK2 = ROMPAGE( byData );
      ROMBANK3 = ROMPAGE( byData + 1 );
      break;

    case 0x8002:
      /* Set ROM Banks */
      byBank = byData & 0x3f; 
      byBank %= ( NesHeader.byRomSize << 1 );
      byBank <<= 1;
      byBank += ( byData & 0x80 ? 1 : 0 );

      ROMBANK0 = ROMPAGE( byBank );
      ROMBANK1 = ROMPAGE( byBank );
      ROMBANK2 = ROMPAGE( byBank );
      ROMBANK3 = ROMPAGE( byBank );
      break;

    case 0x8003:
      /* Name Table Mirroring */
      InfoNES_Mirroring( byData & 0x20 ? 0 : 1);
      
      /* Set ROM Banks */
      byData &= 0x1f;
      byData %= ( NesHeader.byRomSize << 1 );
      byData <<= 1;

      ROMBANK2 = ROMPAGE( byData );
      ROMBANK3 = ROMPAGE( byData + 1 );
      break;
  }
}
