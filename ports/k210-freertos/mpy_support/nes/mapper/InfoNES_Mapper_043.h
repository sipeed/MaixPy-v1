/*===================================================================*/
/*                                                                   */
/*                         Mapper 43 (SMB2J)                         */
/*                                                                   */
/*===================================================================*/

NES_DWORD Map43_IRQ_Cnt;
BYTE Map43_IRQ_Enable;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 43                                             */
/*-------------------------------------------------------------------*/
void Map43_Init()
{
  /* Initialize Mapper */
  MapperInit = Map43_Init;

  /* Write to Mapper */
  MapperWrite = Map43_Write;

  /* Write to SRAM */
  MapperSram = Map0_Sram;

  /* Write to APU */
  MapperApu = Map43_Apu;

  /* Read from APU */
  MapperReadApu = Map43_ReadApu;

  /* Callback at VSync */
  MapperVSync = Map0_VSync;

  /* Callback at HSync */
  MapperHSync = Map43_HSync;

  /* Callback at PPU */
  MapperPPU = Map0_PPU;

  /* Callback at Rendering Screen ( 1:BG, 0:Sprite ) */
  MapperRenderScreen = Map0_RenderScreen;

  /* Set SRAM Banks */
  SRAMBANK = ROMPAGE( 2 );

  /* Set ROM Banks */
  ROMBANK0 = ROMPAGE( 1 );
  ROMBANK1 = ROMPAGE( 0 );
  ROMBANK2 = ROMPAGE( 4 );
  ROMBANK3 = ROMPAGE( 9 );

  /* Initialize State Registers */
	Map43_IRQ_Enable = 1;
	Map43_IRQ_Cnt = 0;

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
/*  Mapper 43 Read from APU Function                                 */
/*-------------------------------------------------------------------*/
BYTE Map43_ReadApu( WORD wAddr )
{
  if ( 0x5000 <= wAddr && wAddr < 0x6000 ) 
  {
    return ROM[ 0x2000*8 + 0x1000 + (wAddr - 0x5000) ];
  }
  return (BYTE)(wAddr >> 8);
}

/*-------------------------------------------------------------------*/
/*  Mapper 43 Write to APU Function                                  */
/*-------------------------------------------------------------------*/
void Map43_Apu( WORD wAddr, BYTE byData )
{
	if( (wAddr&0xF0FF) == 0x4022 ) 
  {
		switch( byData&0x07 ) 
    {
			case	0x00:
			case	0x02:
			case	0x03:
			case	0x04:
        ROMBANK2 = ROMPAGE( 4 );
				break;
			case	0x01:
        ROMBANK2 = ROMPAGE( 3 );
				break;
			case	0x05:
        ROMBANK2 = ROMPAGE( 7 );
				break;
			case	0x06:
        ROMBANK2 = ROMPAGE( 5 );
				break;
			case	0x07:
        ROMBANK2 = ROMPAGE( 6 );
				break;
		}
	}
}

/*-------------------------------------------------------------------*/
/*  Mapper 43 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map43_Write( WORD wAddr, BYTE byData )
{
	if( wAddr == 0x8122 ) {
		if( byData&0x03 ) {
			Map43_IRQ_Enable = 1;
		} else {
			Map43_IRQ_Cnt = 0;
			Map43_IRQ_Enable = 0;
		}
	}
}

/*-------------------------------------------------------------------*/
/*  Mapper 43 H-Sync Function                                        */
/*-------------------------------------------------------------------*/
void Map43_HSync()
{
	if( Map43_IRQ_Enable ) 
  {
		Map43_IRQ_Cnt += 114;
		if( Map43_IRQ_Cnt >= 4096 ) {
			Map43_IRQ_Cnt -= 4096;
			IRQ_REQ;
		}
	}
}
