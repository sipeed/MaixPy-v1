/*===================================================================*/
/*                                                                   */
/*                       Mapper 45 (Pirates)                         */
/*                                                                   */
/*===================================================================*/

BYTE  Map45_Regs[7];
NES_DWORD Map45_P[4],Map45_Prg0,Map45_Prg1,Map45_Prg2,Map45_Prg3;
NES_DWORD Map45_C[8],Map45_Chr0, Map45_Chr1,Map45_Chr2, Map45_Chr3;
NES_DWORD Map45_Chr4, Map45_Chr5, Map45_Chr6, Map45_Chr7;

BYTE Map45_IRQ_Enable;
BYTE Map45_IRQ_Cnt;
BYTE Map45_IRQ_Latch;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 45                                             */
/*-------------------------------------------------------------------*/
void Map45_Init()
{
  /* Initialize Mapper */
  MapperInit = Map45_Init;

  /* Write to Mapper */
  MapperWrite = Map45_Write;

  /* Write to SRAM */
  MapperSram = Map45_Sram;

  /* Write to APU */
  MapperApu = Map0_Apu;

  /* Read from APU */
  MapperReadApu = Map0_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map45_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = SRAM;

  /* Set ROM Banks */
  Map45_Prg0 = 0;
  Map45_Prg1 = 1;
  Map45_Prg2 = NesHeader.byRomSize * 2 - 2;
  Map45_Prg3 = NesHeader.byRomSize * 2 - 1;

  ROMBANK0 = ROMPAGE( Map45_Prg0 );  Map45_P[0] = Map45_Prg0;
  ROMBANK1 = ROMPAGE( Map45_Prg1 );  Map45_P[1] = Map45_Prg1;
  ROMBANK2 = ROMPAGE( Map45_Prg2 );  Map45_P[2] = Map45_Prg2;
  ROMBANK3 = ROMPAGE( Map45_Prg3 );  Map45_P[3] = Map45_Prg3;
  
  /* Set PPU Banks */
  Map45_Chr0 = 0;  Map45_C[0] = Map45_Chr0;
  Map45_Chr1 = 1;  Map45_C[1] = Map45_Chr1;
  Map45_Chr2 = 2;  Map45_C[2] = Map45_Chr2;
  Map45_Chr3 = 3;  Map45_C[3] = Map45_Chr3;
  Map45_Chr4 = 4;  Map45_C[4] = Map45_Chr4;
  Map45_Chr5 = 5;  Map45_C[5] = Map45_Chr5;
  Map45_Chr6 = 6;  Map45_C[6] = Map45_Chr6;
  Map45_Chr7 = 7;  Map45_C[7] = Map45_Chr7;

  for ( int nPage = 0; nPage < 8; ++nPage ) {
    PPUBANK[ nPage ] = VROMPAGE( nPage );
  }
  InfoNES_SetupChr();
  
  /* Initialize IRQ Registers */
  Map45_IRQ_Enable = 0;
  Map45_IRQ_Cnt = 0;
  Map45_IRQ_Latch = 0;

  Map45_Regs[0] = Map45_Regs[1] = Map45_Regs[2] = Map45_Regs[3] = 0;
  Map45_Regs[4] = Map45_Regs[5] = Map45_Regs[6] = 0;

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 45 Write to Sram Function                                 */
/*-------------------------------------------------------------------*/
void Map45_Sram( WORD wAddr, BYTE byData )
{
	if(wAddr == 0x6000)
	{
		Map45_Regs[Map45_Regs[5]] = byData;
		Map45_Regs[5]= (Map45_Regs[5]+1) & 0x03;
		Map45_Set_CPU_Bank4((BYTE)Map45_Prg0);
		Map45_Set_CPU_Bank5((BYTE)Map45_Prg1);
		Map45_Set_CPU_Bank6((BYTE)Map45_Prg2);
		Map45_Set_CPU_Bank7((BYTE)Map45_Prg3);
		Map45_Set_PPU_Banks();
  }
}

/*-------------------------------------------------------------------*/
/*  Mapper 45 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map45_Write( WORD wAddr, BYTE byData )
{
  NES_DWORD swap;

	switch(wAddr & 0xE001) 
  {
    case 0x8000:
			if((byData & 0x40) != (Map45_Regs[6] & 0x40))
			{
				swap = Map45_Prg0; Map45_Prg0 = Map45_Prg2; Map45_Prg2 = swap;
				swap = Map45_P[0]; Map45_P[0] = Map45_P[2]; Map45_P[2] = swap;
        ROMBANK0 = ROMPAGE( Map45_P[0] % ( NesHeader.byRomSize << 1) );
        ROMBANK2 = ROMPAGE( Map45_P[2] % ( NesHeader.byRomSize << 1) );
			}
			if (NesHeader.byRomSize > 0)
			{
				if((byData & 0x80) != (Map45_Regs[6] & 0x80))
				{
					swap = Map45_Chr4; Map45_Chr4 = Map45_Chr0; Map45_Chr0 = swap;
					swap = Map45_Chr5; Map45_Chr5 = Map45_Chr1; Map45_Chr1 = swap;
					swap = Map45_Chr6; Map45_Chr6 = Map45_Chr2; Map45_Chr2 = swap;
					swap = Map45_Chr7; Map45_Chr7 = Map45_Chr3; Map45_Chr3 = swap;
					swap = Map45_C[4]; Map45_C[4] = Map45_C[0]; Map45_C[0] = swap;
					swap = Map45_C[5]; Map45_C[5] = Map45_C[1]; Map45_C[1] = swap;
					swap = Map45_C[6]; Map45_C[6] = Map45_C[2]; Map45_C[2] = swap;
					swap = Map45_C[7]; Map45_C[7] = Map45_C[3]; Map45_C[3] = swap;
					
          PPUBANK[ 0 ] = VROMPAGE( Map45_C[0] % ( NesHeader.byVRomSize << 3 ) );
          PPUBANK[ 1 ] = VROMPAGE( Map45_C[1] % ( NesHeader.byVRomSize << 3 ) );
          PPUBANK[ 2 ] = VROMPAGE( Map45_C[2] % ( NesHeader.byVRomSize << 3 ) );
          PPUBANK[ 3 ] = VROMPAGE( Map45_C[3] % ( NesHeader.byVRomSize << 3 ) );
          PPUBANK[ 4 ] = VROMPAGE( Map45_C[4] % ( NesHeader.byVRomSize << 3 ) );
          PPUBANK[ 5 ] = VROMPAGE( Map45_C[5] % ( NesHeader.byVRomSize << 3 ) );
          PPUBANK[ 6 ] = VROMPAGE( Map45_C[6] % ( NesHeader.byVRomSize << 3 ) );
          PPUBANK[ 7 ] = VROMPAGE( Map45_C[7] % ( NesHeader.byVRomSize << 3 ) );
          InfoNES_SetupChr();
				}
			}
			Map45_Regs[6] = byData;
		  break;

	case 0x8001:
	  switch(Map45_Regs[6] & 0x07)
		{
			case 0x00:
				Map45_Chr0 = (byData & 0xFE)+0;
				Map45_Chr1 = (byData & 0xFE)+1;
				Map45_Set_PPU_Banks();
				break;

			case 0x01:
			  Map45_Chr2 = (byData & 0xFE)+0;
				Map45_Chr3 = (byData & 0xFE)+1;
				Map45_Set_PPU_Banks();
				break;

			case 0x02:
				Map45_Chr4 = byData;
				Map45_Set_PPU_Banks();
				break;

			case 0x03:
				Map45_Chr5 = byData;
				Map45_Set_PPU_Banks();
				break;

			case 0x04:
				Map45_Chr6 = byData;
				Map45_Set_PPU_Banks();
				break;

			case 0x05:
				Map45_Chr7 = byData;
				Map45_Set_PPU_Banks();
				break;

      case 0x06:
				if(Map45_Regs[6] & 0x40)
				{
				  Map45_Prg2 = byData & 0x3F;
					Map45_Set_CPU_Bank6(byData);
				}
				else
				{
					Map45_Prg0 = byData & 0x3F;
					Map45_Set_CPU_Bank4(byData);
				}
				break;

			case 0x07:
				Map45_Prg1 = byData & 0x3F;
				Map45_Set_CPU_Bank5(byData);
				break;
		}
		break;

	case 0xA000:
     if ( byData & 0x01 )
     {
       InfoNES_Mirroring( 0 );
     } else {
       InfoNES_Mirroring( 1 );
     }
		break;

	case 0xC000:
	  Map45_IRQ_Cnt = byData;
		break;

	case 0xC001:
		Map45_IRQ_Latch = byData;
		break;

	case 0xE000:
		Map45_IRQ_Enable = 0;
		break;

	case 0xE001:
		Map45_IRQ_Enable = 1;
		break;
	}
}

/*-------------------------------------------------------------------*/
/*  Mapper 45 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map45_HSync()
{
/*
 *  Callback at HSync
 *
 */
  if(Map45_IRQ_Enable)
  {
    if ( /* 0 <= PPU_Scanline && */ PPU_Scanline <= 239 )
    {
      if ( PPU_R1 & R1_SHOW_SCR || PPU_R1 & R1_SHOW_SP )
			{
				if(!(Map45_IRQ_Cnt--))
				{
					Map45_IRQ_Cnt = Map45_IRQ_Latch;
          IRQ_REQ;
				}
			}
		}
	}
}

/*-------------------------------------------------------------------*/
/*  Mapper 45 Set CPU Banks Function                                 */
/*-------------------------------------------------------------------*/

void Map45_Set_CPU_Bank4(BYTE byData)
{
	byData &= (Map45_Regs[3] & 0x3F) ^ 0xFF;
	byData &= 0x3F;
	byData |= Map45_Regs[1];
  ROMBANK0 = ROMPAGE( byData % ( NesHeader.byRomSize << 1) );
	Map45_P[0] = byData;
}

void Map45_Set_CPU_Bank5(BYTE byData)
{
	byData &= (Map45_Regs[3] & 0x3F) ^ 0xFF;
	byData &= 0x3F;
	byData |= Map45_Regs[1];
  ROMBANK1 = ROMPAGE( byData % ( NesHeader.byRomSize << 1) );
	Map45_P[1] = byData;
}

void Map45_Set_CPU_Bank6(BYTE byData)
{
	byData &= (Map45_Regs[3] & 0x3F) ^ 0xFF;
	byData &= 0x3F;
	byData |= Map45_Regs[1];
  ROMBANK2 = ROMPAGE( byData % ( NesHeader.byRomSize << 1) );
	Map45_P[2] = byData;
}

void Map45_Set_CPU_Bank7(BYTE byData)
{
	byData &= (Map45_Regs[3] & 0x3F) ^ 0xFF;
	byData &= 0x3F;
	byData |= Map45_Regs[1];
  ROMBANK3 = ROMPAGE( byData % ( NesHeader.byRomSize << 1) );
	Map45_P[3] = byData;
}

/*-------------------------------------------------------------------*/
/*  Mapper 45 Set PPU Banks Function                                 */
/*-------------------------------------------------------------------*/
void Map45_Set_PPU_Banks()
{
	BYTE table[16] =
	    {
	        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	        0x01,0x03,0x07,0x0F,0x1F,0x3F,0x7F,0xFF
	    };
	Map45_C[0] = Map45_Chr0;
	Map45_C[1] = Map45_Chr1;
	Map45_C[2] = Map45_Chr2;
	Map45_C[3] = Map45_Chr3;
	Map45_C[4] = Map45_Chr4;
	Map45_C[5] = Map45_Chr5;
	Map45_C[6] = Map45_Chr6;
	Map45_C[7] = Map45_Chr7;
	for(BYTE i = 0; i < 8; i++)
	{
		Map45_C[i] &= table[Map45_Regs[2] & 0x0F];
		Map45_C[i] |= Map45_Regs[0] & 0xff;
		Map45_C[i] += (BYTE)(Map45_Regs[2] & 0x10) << 4;
	}
	if(Map45_Regs[6] & 0x80)
	{
    PPUBANK[ 0 ] = VROMPAGE( Map45_C[4] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 1 ] = VROMPAGE( Map45_C[5] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 2 ] = VROMPAGE( Map45_C[6] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 3 ] = VROMPAGE( Map45_C[7] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 4 ] = VROMPAGE( Map45_C[0] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 5 ] = VROMPAGE( Map45_C[1] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 6 ] = VROMPAGE( Map45_C[2] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 7 ] = VROMPAGE( Map45_C[3] % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
	}
	else
	{
    PPUBANK[ 0 ] = VROMPAGE( Map45_C[0] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 1 ] = VROMPAGE( Map45_C[1] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 2 ] = VROMPAGE( Map45_C[2] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 3 ] = VROMPAGE( Map45_C[3] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 4 ] = VROMPAGE( Map45_C[4] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 5 ] = VROMPAGE( Map45_C[5] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 6 ] = VROMPAGE( Map45_C[6] % ( NesHeader.byVRomSize << 3 ) );
    PPUBANK[ 7 ] = VROMPAGE( Map45_C[7] % ( NesHeader.byVRomSize << 3 ) );
    InfoNES_SetupChr();
	}
}


