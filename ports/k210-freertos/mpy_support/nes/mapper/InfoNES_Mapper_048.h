/*===================================================================*/
/*                                                                   */
/*                   Mapper 48 (Taito TC0190V)                       */
/*                                                                   */
/*===================================================================*/

BYTE Map48_Regs[ 1 ];
BYTE Map48_IRQ_Enable;
BYTE Map48_IRQ_Cnt;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 48                                             */
/*-------------------------------------------------------------------*/
void Map48_Init()
{
  /* Initialize Mapper */
  MapperInit = Map48_Init;

  /* Write to Mapper */
  MapperWrite = Map48_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map48_HSync;

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

  /* Initialize IRQ Registers */
  Map48_Regs[ 0 ] = 0;
  Map48_IRQ_Enable = 0;
  Map48_IRQ_Cnt = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 48 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map48_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x8000:
      /* Name Table Mirroring */ 
      if ( !Map48_Regs[ 0 ] )
      {
        if ( byData & 0x40 )
        {
          InfoNES_Mirroring( 0 );
        } else {
          InfoNES_Mirroring( 1 );
        }
      }
      /* Set ROM Banks */
      ROMBANK0 = ROMPAGE( byData % ( NesHeader.byRomSize << 1 ) );
      break;

    case 0x8001:
      /* Set ROM Banks */
      ROMBANK1 = ROMPAGE( byData % ( NesHeader.byRomSize << 1 ) );
      break;  
 
    /* Set PPU Banks */
    case 0x8002:
      PPUBANK[ 0 ] = VROMPAGE( ( ( byData << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 1 ] = VROMPAGE( ( ( byData << 1 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0x8003:
      PPUBANK[ 2 ] = VROMPAGE( ( ( byData << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      PPUBANK[ 3 ] = VROMPAGE( ( ( byData << 1 ) + 1 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xa000:
      PPUBANK[ 4 ] = VROMPAGE( ( ( byData << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xa001:
      PPUBANK[ 5 ] = VROMPAGE( ( ( byData << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xa002:
      PPUBANK[ 6 ] = VROMPAGE( ( ( byData << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xa003:
      PPUBANK[ 7 ] = VROMPAGE( ( ( byData << 1 ) + 0 ) % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

    case 0xc000:
      Map48_IRQ_Cnt = byData;
      break;

    case 0xc001:
      Map48_IRQ_Enable = byData & 0x01;
      break;

    case 0xe000:
      /* Name Table Mirroring */ 
      if ( byData & 0x40 )
      {
        InfoNES_Mirroring( 0 );
      } else {
        InfoNES_Mirroring( 1 );
      }
      Map48_Regs[ 0 ] = 1;
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 48 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map48_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( Map48_IRQ_Enable )
  {
    if ( /* 0 <= PPU_Scanline && */ PPU_Scanline <= 239 )
    {
      if ( PPU_R1 & R1_SHOW_SCR || PPU_R1 & R1_SHOW_SP )
      {
        if ( Map48_IRQ_Cnt == 0xff )
        {
          IRQ_REQ;
          Map48_IRQ_Enable = 0;
        } else {
          Map48_IRQ_Cnt++;
        }
      }
    }
  }
}
