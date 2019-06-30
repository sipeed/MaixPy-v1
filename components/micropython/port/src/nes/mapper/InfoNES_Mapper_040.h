/*===================================================================*/
/*                                                                   */
/*                       Mapper 40 (SMB2J)                           */
/*                                                                   */
/*===================================================================*/

BYTE  Map40_IRQ_Enable;
NES_DWORD Map40_Line_To_IRQ;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 40                                             */
/*-------------------------------------------------------------------*/
void Map40_Init()
{
  /* Initialize Mapper */
  MapperInit = Map40_Init;

  /* Write to Mapper */
  MapperWrite = Map40_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map40_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = ROMPAGE( 6 );

  /* Initialize IRQ Registers */
  Map40_IRQ_Enable = 0;
  Map40_Line_To_IRQ = 0;

  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( 4 );
  ROMBANK1 = ROMPAGE( 5 );
  ROMBANK2 = ROMPAGE( 0 );
  ROMBANK3 = ROMPAGE( 7 );

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
/*  Mapper 40 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map40_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr & 0xe000 )
  {
    case 0x8000:
      Map40_IRQ_Enable = 0;
      break;

    case 0xa000:
      Map40_IRQ_Enable = 1;
      Map40_Line_To_IRQ = 37;
      break;

    case 0xc000:
      break;

    case 0xe000:
      /* Set ROM Banks */
      ROMBANK2 = ROMPAGE ( ( byData & 0x07 ) % ( NesHeader.byRomSize << 1 ) );
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 40 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map40_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( Map40_IRQ_Enable )
  {
    if ( ( --Map40_Line_To_IRQ ) <= 0 )
    {
      IRQ_REQ;
    }
  }
}

/* End of InfoNES_Mapper_40.cpp */
