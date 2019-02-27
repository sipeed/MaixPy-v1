/*===================================================================*/
/*                                                                   */
/*                  Mapper 24 (Konami VRC6)                          */
/*                                                                   */
/*===================================================================*/

BYTE Map24_IRQ_Count;
BYTE Map24_IRQ_State;
BYTE Map24_IRQ_Latch;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 24                                             */
/*-------------------------------------------------------------------*/
void Map24_Init()
{
  /* Initialize Mapper */
  MapperInit = Map24_Init;

  /* Write to Mapper */
  MapperWrite = Map24_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map24_HSync;

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

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 24 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map24_Write( WORD wAddr, BYTE byData )
{
  switch ( wAddr )
  {
    case 0x8000:
      /* Set ROM Banks */
      ROMBANK0 = ROMPAGE( ( byData + 0 ) % ( NesHeader.byRomSize << 1) );
      ROMBANK1 = ROMPAGE( ( byData + 1 ) % ( NesHeader.byRomSize << 1) );
      break;

    case 0xb003:
      /* Name Table Mirroring */
      switch ( byData & 0x0c )
      {
        case 0x00:
          InfoNES_Mirroring( 1 );   /* Vertical */
          break;
        case 0x04:
          InfoNES_Mirroring( 0 );   /* Horizontal */
          break;
        case 0x08:
          InfoNES_Mirroring( 3 );   /* One Screen 0x2000 */
          break;
        case 0x0c:
          InfoNES_Mirroring( 2 );   /* One Screen 0x2400 */
          break;
      }
      break;

	  case 0xC000:
      ROMBANK2 = ROMPAGE( byData % ( NesHeader.byRomSize << 1) );
		  break;

	  case 0xD000:
      PPUBANK[ 0 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

	  case 0xD001:
      PPUBANK[ 1 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

	  case 0xD002:
      PPUBANK[ 2 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

	  case 0xD003:
      PPUBANK[ 3 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

	  case 0xE000:
      PPUBANK[ 4 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

	  case 0xE001:
      PPUBANK[ 5 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

	  case 0xE002:
      PPUBANK[ 6 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

	  case 0xE003:
      PPUBANK[ 7 ] = VROMPAGE( byData % ( NesHeader.byVRomSize << 3 ) );
      InfoNES_SetupChr();
      break;

	  case 0xF000:
			Map24_IRQ_Latch = byData;
	  	break;

	  case 0xF001:
			Map24_IRQ_State = byData & 0x03;
			if(Map24_IRQ_State & 0x02)
			{
				Map24_IRQ_Count = Map24_IRQ_Latch;
			}
		  break;

	  case 0xF002:
			if(Map24_IRQ_State & 0x01)
			{
				Map24_IRQ_State |= 0x02;
			}
			else
			{
				Map24_IRQ_State &= 0x01;
			}
		break;
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 24 H-Sync Function                                         */
/*-------------------------------------------------------------------*/
void Map24_HSync()
{
/*
 *  Callback at HSync
 *
 */
	if(Map24_IRQ_State & 0x02)
	{
	  if(Map24_IRQ_Count == 0xFF)
		{
			IRQ_REQ;
			Map24_IRQ_Count = Map24_IRQ_Latch;
		}
		else
		{
			Map24_IRQ_Count++;
		}
	}
}
