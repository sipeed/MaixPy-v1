/*===================================================================*/
/*                                                                   */
/*                    Mapper 64 (Tengen RAMBO-1)                     */
/*                                                                   */
/*===================================================================*/

BYTE Map64_Cmd;
BYTE Map64_Prg;
BYTE Map64_Chr;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 64                                             */
/*-------------------------------------------------------------------*/
void Map64_Init()
{
  /* Initialize Mapper */
  MapperInit = Map64_Init;

  /* Write to Mapper */
  MapperWrite = Map64_Write;

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
  ROMBANK0 = ROMLASTPAGE( 0 );
  ROMBANK1 = ROMLASTPAGE( 0 );
  ROMBANK2 = ROMLASTPAGE( 0 );
  ROMBANK3 = ROMLASTPAGE( 0 );

  /* Set PPU Banks */
  if ( NesHeader.byVRomSize > 0 )
  {
    for ( int nPage = 0; nPage < 8; ++nPage )
      PPUBANK[ nPage ] = VROMPAGE( nPage );
    InfoNES_SetupChr();
  }

  /* Initialize state flag */
  Map64_Cmd = 0x00;
  Map64_Prg = 0x00;
  Map64_Chr = 0x00;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 64 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map64_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x8000:
      /* Set state flag */
      Map64_Cmd = byData & 0x0f;
      Map64_Prg = ( byData & 0x40 ) >> 6;
      Map64_Chr = ( byData & 0x80 ) >> 7;
      break;

    case 0x8001:
      switch ( Map64_Cmd )
      {
        case 0x00:
          /* Set PPU Banks */
          byData %= ( NesHeader.byVRomSize << 3 );
          if ( Map64_Chr )
          {
            PPUBANK[ 4 ] = VROMPAGE( byData );
            PPUBANK[ 5 ] = VROMPAGE( byData + 1 );      
          } else {
            PPUBANK[ 0 ] = VROMPAGE( byData );
            PPUBANK[ 1 ] = VROMPAGE( byData + 1 );  
          } 
          InfoNES_SetupChr();
          break;

        case 0x01:
          /* Set PPU Banks */
          byData %= ( NesHeader.byVRomSize << 3 );
          if ( Map64_Chr )
          {
            PPUBANK[ 6 ] = VROMPAGE( byData );
            PPUBANK[ 7 ] = VROMPAGE( byData + 1 );      
          } else {
            PPUBANK[ 2 ] = VROMPAGE( byData );
            PPUBANK[ 3 ] = VROMPAGE( byData + 1 );  
          } 
          InfoNES_SetupChr();
          break;

        case 0x02:
          /* Set PPU Banks */
          byData %= ( NesHeader.byVRomSize << 3 );
          if ( Map64_Chr )
          {
            PPUBANK[ 0 ] = VROMPAGE( byData );
          } else {
            PPUBANK[ 4 ] = VROMPAGE( byData );
          } 
          InfoNES_SetupChr();
          break;

        case 0x03:
          /* Set PPU Banks */
          byData %= ( NesHeader.byVRomSize << 3 );
          if ( Map64_Chr )
          {
            PPUBANK[ 1 ] = VROMPAGE( byData );
          } else {
            PPUBANK[ 5 ] = VROMPAGE( byData );
          } 
          InfoNES_SetupChr();
          break;

        case 0x04:
          /* Set PPU Banks */
          byData %= ( NesHeader.byVRomSize << 3 );
          if ( Map64_Chr )
          {
            PPUBANK[ 2 ] = VROMPAGE( byData );
          } else {
            PPUBANK[ 6 ] = VROMPAGE( byData );
          } 
          InfoNES_SetupChr();
          break;

        case 0x05:
          /* Set PPU Banks */
          byData %= ( NesHeader.byVRomSize << 3 );
          if ( Map64_Chr )
          {
            PPUBANK[ 3 ] = VROMPAGE( byData );
          } else {
            PPUBANK[ 7 ] = VROMPAGE( byData );
          } 
          InfoNES_SetupChr();
          break;

        case 0x06:
          /* Set ROM Banks */
          byData %= ( NesHeader.byRomSize << 1 );
          if ( Map64_Prg )
          {
            ROMBANK1 = ROMPAGE( byData );
          } else {
            ROMBANK0 = ROMPAGE( byData );
          } 
          break;

        case 0x07:
          /* Set ROM Banks */
          byData %= ( NesHeader.byRomSize << 1 );
          if ( Map64_Prg )
          {
            ROMBANK2 = ROMPAGE( byData );
          } else {
            ROMBANK1 = ROMPAGE( byData );
          } 
          break;

        case 0x08:
          /* Set PPU Banks */
          byData %= ( NesHeader.byVRomSize << 3 );
          PPUBANK[ 1 ] = VROMPAGE( byData );
          InfoNES_SetupChr();
          break;

        case 0x09:
          /* Set PPU Banks */
          byData %= ( NesHeader.byVRomSize << 3 );
          PPUBANK[ 3 ] = VROMPAGE( byData );
          InfoNES_SetupChr();
          break;

        case 0x0f:
          /* Set ROM Banks */
          byData %= ( NesHeader.byRomSize << 1 );
          if ( Map64_Prg )
          {
            ROMBANK0 = ROMPAGE( byData );
          } else {
            ROMBANK2 = ROMPAGE( byData );
          } 
          break;
      }
      break;

    default:
      switch ( wAddr & 0xf000 )
      {
        case 0xa000:
          /* Name Table Mirroring */
          InfoNES_Mirroring( byData & 0x01 );
          break;

        default:
          break;
      }
      break;
  }
}
