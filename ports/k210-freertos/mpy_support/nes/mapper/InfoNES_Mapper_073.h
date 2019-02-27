/*===================================================================*/
/*                                                                   */
/*                     Mapper 73 (Konami VRC 3)                      */
/*                                                                   */
/*===================================================================*/

BYTE  Map73_IRQ_Enable;
NES_DWORD Map73_IRQ_Cnt;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 73                                             */
/*-------------------------------------------------------------------*/
void Map73_Init()
{
  /* Initialize Mapper */
  MapperInit = Map73_Init;

  /* Write to Mapper */
  MapperWrite = Map73_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map73_HSync;

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

  /* Initialize IRQ Registers */
  Map73_IRQ_Enable = 0;
  Map73_IRQ_Cnt = 0;  

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 73 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map73_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x8000:
      Map73_IRQ_Cnt = ( Map73_IRQ_Cnt & 0xfff0 ) | ( byData & 0x0f );
      break;

    case 0x9000:
      Map73_IRQ_Cnt = ( Map73_IRQ_Cnt & 0xff0f ) | ( ( byData & 0x0f ) << 4 );
      break;

    case 0xa000:
      Map73_IRQ_Cnt = ( Map73_IRQ_Cnt & 0xf0ff ) | ( ( byData & 0x0f ) << 8 );
      break;

    case 0xb000:
      Map73_IRQ_Cnt = ( Map73_IRQ_Cnt & 0x0fff ) | ( ( byData & 0x0f ) << 12 );
      break;

    case 0xc000:
      Map73_IRQ_Enable = byData;
      break;

    /* Set ROM Banks */
    case 0xf000:
      byData <<= 1;
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK0 = ROMPAGE( byData );
      ROMBANK1 = ROMPAGE( byData + 1 );
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 73 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map73_HSync()
{
/*
 *  Callback at HSync
 *
 */
#if 1
  if ( Map73_IRQ_Enable & 0x02 )
  {
    if ( ( Map73_IRQ_Cnt += STEP_PER_SCANLINE ) > 0xffff )
    {
      Map73_IRQ_Cnt &= 0xffff;
      IRQ_REQ;
      Map73_IRQ_Enable = 0;
    }
  }
#else
  if ( Map73_IRQ_Enable )
  {
    if ( Map73_IRQ_Cnt > 0xffff - 114 )
    {
      IRQ_REQ;
      Map73_IRQ_Enable = 0;
    } else {
      Map73_IRQ_Cnt += 114;
    }
  }
#endif
}
