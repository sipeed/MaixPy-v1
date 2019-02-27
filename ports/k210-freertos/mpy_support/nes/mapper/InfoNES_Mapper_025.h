/*===================================================================*/
/*                                                                   */
/*                  Mapper 25 (Konami VRC4 type B)                   */
/*                                                                   */
/*===================================================================*/

BYTE Map25_Bank_Selector;
BYTE Map25_VBank[16];

BYTE Map25_IRQ_Count;
BYTE Map25_IRQ_State;
BYTE Map25_IRQ_Latch;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 25                                             */
/*-------------------------------------------------------------------*/
void Map25_Init()
{
  int nPage;

  /* Initialize Mapper */
  MapperInit = Map25_Init;

  /* Write to Mapper */
  MapperWrite = Map25_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map25_HSync;

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

  /* Reset VBank Registers */
  for (nPage = 0; nPage < 16; nPage++)
    Map25_VBank[ nPage ] = 0x00;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 25 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map25_Write( WORD wAddr, BYTE byData )
{
  int nBank;

  switch ( wAddr )
  {
    case 0x8000:
      /* Set ROM Banks */
      byData %= ( NesHeader.byRomSize << 1 );
      if ( Map25_Bank_Selector ) 
      {
        ROMBANK2 = ROMPAGE( byData );
      } else {
        ROMBANK0 = ROMPAGE( byData );
      }
      break;

    case 0x9000:
      /* Name Table Mirroring */
      switch (byData & 0x03)
      {
        case 0:
          InfoNES_Mirroring( 1 );   /* Vertical */
          break;
        case 1:
          InfoNES_Mirroring( 0 );   /* Horizontal */
          break;
        case 2:
          InfoNES_Mirroring( 2 );   /* One Screen 0x2000 */
          break;
        case 3:
          InfoNES_Mirroring( 3 );   /* One Screen 0x2400 */
          break;
      }
      break;

    case 0x9002:
      /* TODO: SaveRAM Toggle */
      /* $8000 Switching Mode */
      Map25_Bank_Selector = byData & 0x02;
      break;

    case 0xa000:
      /* Set ROM Banks */
      byData %= ( NesHeader.byRomSize << 1 );
      ROMBANK1 = ROMPAGE( byData );
      break;
    
    default:
      /* Set PPU Banks */
      switch ( wAddr & 0xfffc)
      {
        case 0xb000:
          Map25_VBank[ 0 + ( wAddr & 0x0003 ) ] = byData & 0x0f;
          nBank = 0 + ( wAddr & 0x0001 );
          Map25_Sync_Vrom( nBank );
          break;
  
        case 0xc000:
          Map25_VBank[ 4 + ( wAddr & 0x0003 ) ] = byData & 0x0f;
          nBank = 2 + ( wAddr & 0x0001 );
          Map25_Sync_Vrom( nBank );
          break;

        case 0xd000:
          Map25_VBank[ 8 + ( wAddr & 0x0003 ) ] = byData & 0x0f;
          nBank = 4 + ( wAddr & 0x0001 );
          Map25_Sync_Vrom( nBank );
          break;

        case 0xe000:
          Map25_VBank[ 12 + ( wAddr & 0x0003 ) ] = byData & 0x0f;
          nBank = 6 + ( wAddr & 0x0001 );
          Map25_Sync_Vrom( nBank );
          break;

        case 0xf000:
          switch ( wAddr & 0x0003 )
          {
            case 0:
            case 1:
              Map25_IRQ_Latch = byData;
              break;
            case 2:
              Map25_IRQ_State = ( byData & 0x01 ) ? Map25_IRQ_State : 0x00;
              Map25_IRQ_State = ( byData & 0x02 ) ? 0x01 : Map25_IRQ_State;
              Map25_IRQ_Count = Map25_IRQ_Latch;
              break;
            case 3:
              Map25_IRQ_State = ( Map25_IRQ_State << 1 ) | ( Map25_IRQ_State & 1 );
              break;
          }
          break;
      }
      break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 25 Sync VROM Function                                     */
/*-------------------------------------------------------------------*/
void Map25_Sync_Vrom( int nBank )
{
  BYTE byValue;
  
  byValue  = Map25_VBank[ ( nBank << 1 ) - ( nBank & 0x01 ) ];
  byValue |= Map25_VBank[ ( nBank << 1 ) - ( nBank & 0x01 ) + 2] << 4;
  byValue %= ( NesHeader.byVRomSize << 3 );
  PPUBANK[ nBank ] = VROMPAGE( byValue );
  InfoNES_SetupChr();
}

/*-------------------------------------------------------------------*/
/*  Mapper 25 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map25_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if ( ( Map25_IRQ_State & 0x02 ) && ( ++Map25_IRQ_Count == 0 ) )
  {
    IRQ_REQ;
    Map25_IRQ_Count = Map25_IRQ_Latch;
  }
}
