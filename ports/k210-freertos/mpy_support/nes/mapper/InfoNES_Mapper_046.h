/*===================================================================*/
/*                                                                   */
/*                   Mapper 46 (Color Dreams)                        */
/*                                                                   */
/*===================================================================*/

BYTE Map46_Regs[ 4 ];

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 46                                             */
/*-------------------------------------------------------------------*/
void Map46_Init()
{
  /* Initialize Mapper */
  MapperInit = Map46_Init;

  /* Write to Mapper */
  MapperWrite = Map46_Write;

  /* Write to SRAM */
  MapperSram = Map46_Sram;

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
  Map46_Regs[ 0 ] = Map46_Regs[ 1 ] = Map46_Regs[ 2 ] = Map46_Regs[ 3 ] = 0;
  Map46_Set_ROM_Banks();

  /* Name Table Mirroring */
  InfoNES_Mirroring( 1 );
  
  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 46 Write to SRAM Function                                 */
/*-------------------------------------------------------------------*/
void Map46_Sram( WORD wAddr, BYTE byData )
{
  /* Set ROM Banks */
  Map46_Regs[ 0 ] = byData & 0x0f;
  Map46_Regs[ 1 ] = ( byData & 0xf0 ) >> 4;
  Map46_Set_ROM_Banks();
}

/*-------------------------------------------------------------------*/
/*  Mapper 46 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map46_Write( WORD wAddr, BYTE byData )
{
  /* Set ROM Banks */
  Map46_Regs[ 2 ] = byData & 0x01;
  Map46_Regs[ 3 ] = ( byData & 0x70 ) >> 4;
  Map46_Set_ROM_Banks();
}

/*-------------------------------------------------------------------*/
/*  Mapper 46 Setup ROM Banks Function                               */
/*-------------------------------------------------------------------*/
void Map46_Set_ROM_Banks()
{
  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( ( ( Map46_Regs[ 0 ] << 3 ) + ( Map46_Regs[ 2 ] << 2 ) + 0 ) % ( NesHeader.byRomSize << 1 ) );  
  ROMBANK1 = ROMPAGE( ( ( Map46_Regs[ 0 ] << 3 ) + ( Map46_Regs[ 2 ] << 2 ) + 1 ) % ( NesHeader.byRomSize << 1 ) );
  ROMBANK2 = ROMPAGE( ( ( Map46_Regs[ 0 ] << 3 ) + ( Map46_Regs[ 2 ] << 2 ) + 2 ) % ( NesHeader.byRomSize << 1 ) );
  ROMBANK3 = ROMPAGE( ( ( Map46_Regs[ 0 ] << 3 ) + ( Map46_Regs[ 2 ] << 2 ) + 3 ) % ( NesHeader.byRomSize << 1 ) ); 

  /* Set PPU Banks */
  PPUBANK[ 0 ] = VROMPAGE( ( ( Map46_Regs[ 1 ] << 6 ) + ( Map46_Regs[ 3 ] << 3 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) ); 
  PPUBANK[ 1 ] = VROMPAGE( ( ( Map46_Regs[ 1 ] << 6 ) + ( Map46_Regs[ 3 ] << 3 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) ); 
  PPUBANK[ 2 ] = VROMPAGE( ( ( Map46_Regs[ 1 ] << 6 ) + ( Map46_Regs[ 3 ] << 3 ) + 2 ) % ( NesHeader.byVRomSize << 3 ) ); 
  PPUBANK[ 3 ] = VROMPAGE( ( ( Map46_Regs[ 1 ] << 6 ) + ( Map46_Regs[ 3 ] << 3 ) + 3 ) % ( NesHeader.byVRomSize << 3 ) ); 
  PPUBANK[ 4 ] = VROMPAGE( ( ( Map46_Regs[ 1 ] << 6 ) + ( Map46_Regs[ 3 ] << 3 ) + 4 ) % ( NesHeader.byVRomSize << 3 ) ); 
  PPUBANK[ 5 ] = VROMPAGE( ( ( Map46_Regs[ 1 ] << 6 ) + ( Map46_Regs[ 3 ] << 3 ) + 5 ) % ( NesHeader.byVRomSize << 3 ) ); 
  PPUBANK[ 6 ] = VROMPAGE( ( ( Map46_Regs[ 1 ] << 6 ) + ( Map46_Regs[ 3 ] << 3 ) + 6 ) % ( NesHeader.byVRomSize << 3 ) ); 
  PPUBANK[ 7 ] = VROMPAGE( ( ( Map46_Regs[ 1 ] << 6 ) + ( Map46_Regs[ 3 ] << 3 ) + 7 ) % ( NesHeader.byVRomSize << 3 ) ); 
  InfoNES_SetupChr();
}
