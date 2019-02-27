/*===================================================================*/
/*                                                                   */
/*                            Mapper 61                              */
/*                                                                   */
/*===================================================================*/

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 61                                             */
/*-------------------------------------------------------------------*/
void Map61_Init()
{
  /* Initialize Mapper */
  MapperInit = Map61_Init;

  /* Write to Mapper */
  MapperWrite = Map61_Write;

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
  ROMBANK2 = ROMLASTPAGE( 1 );
  ROMBANK3 = ROMLASTPAGE( 0 );

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 61 Write Function                                         */
/*-------------------------------------------------------------------*/
void Map61_Write( WORD wAddr, BYTE byData )
{
	BYTE byBank;

	switch( wAddr & 0x30 ) {
		case	0x00:
		case	0x30:
			ROMBANK0 = ROMPAGE((((wAddr&0x0F)<<2)+0) % (NesHeader.byRomSize<<1));
			ROMBANK1 = ROMPAGE((((wAddr&0x0F)<<2)+1) % (NesHeader.byRomSize<<1));
			ROMBANK2 = ROMPAGE((((wAddr&0x0F)<<2)+2) % (NesHeader.byRomSize<<1));
			ROMBANK3 = ROMPAGE((((wAddr&0x0F)<<2)+3) % (NesHeader.byRomSize<<1));
			break;
		case	0x10:
		case	0x20:
			byBank = ((wAddr & 0x0F)<<1)|((wAddr&0x20)>>4);

			ROMBANK0 = ROMPAGE(((byBank<<1)+0) % (NesHeader.byRomSize<<1));
			ROMBANK1 = ROMPAGE(((byBank<<1)+1) % (NesHeader.byRomSize<<1));
			ROMBANK2 = ROMPAGE(((byBank<<1)+0) % (NesHeader.byRomSize<<1));
			ROMBANK3 = ROMPAGE(((byBank<<1)+1) % (NesHeader.byRomSize<<1));
			break;
	}

	if( wAddr & 0x80 ) InfoNES_Mirroring( 0 );
	else		   InfoNES_Mirroring( 1 );
}
