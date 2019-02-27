/*===================================================================*/
/*                                                                   */
/*                            Mapper 1                               */
/*                                                                   */
/*===================================================================*/

BYTE  Map1_Regs[ 4 ];
NES_DWORD Map1_Cnt;
BYTE  Map1_Latch;
WORD  Map1_Last_Write_Addr;

enum Map1_Size_t
{
  Map1_SMALL,
  Map1_512K,
  Map1_1024K
};

enum Map1_Size_t Map1_Size;
NES_DWORD Map1_256K_base;
NES_DWORD Map1_swap;

// these are the 4 ROM banks currently selected
NES_DWORD Map1_bank1;
NES_DWORD Map1_bank2;
NES_DWORD Map1_bank3;
NES_DWORD Map1_bank4;

NES_DWORD Map1_HI1;
NES_DWORD Map1_HI2;

/*-------------------------------------------------------------------*/
/*  Initialize Mapper 1                                              */
/*-------------------------------------------------------------------*/
void Map1_Init()
{
  NES_DWORD size_in_K;

  /* Initialize Mapper */
  MapperInit = Map1_Init;

  /* Write to Mapper */
  MapperWrite = Map1_Write;

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

  /* Initialize State Registers */
  Map1_Cnt = 0;
  Map1_Latch = 0x00;

  Map1_Regs[ 0 ] = 0x0c;
  Map1_Regs[ 1 ] = 0x00;
  Map1_Regs[ 2 ] = 0x00;
  Map1_Regs[ 3 ] = 0x00;

  size_in_K = ( NesHeader.byRomSize << 1 ) * 8;

  if ( size_in_K == 1024 )
  {
    Map1_Size = Map1_1024K;
  } 
  else if(size_in_K == 512)
  {
    Map1_Size = Map1_512K;
  }
  else
  {
    Map1_Size = Map1_SMALL;
  }

  Map1_256K_base = 0; // use first 256K
  Map1_swap = 0;

  if( Map1_Size == Map1_SMALL )
  {
    // set two high pages to last two banks
    Map1_HI1 = ( NesHeader.byRomSize << 1 ) - 2;
    Map1_HI2 = ( NesHeader.byRomSize << 1 ) - 1;
  }
  else
  {
    // set two high pages to last two banks of current 256K region
    Map1_HI1 = ( 256 / 8 ) - 2;
    Map1_HI2 = ( 256 / 8 ) - 1;
  }

  // set CPU bank pointers
  Map1_bank1 = 0;
  Map1_bank2 = 1;
  Map1_bank3 = Map1_HI1;
  Map1_bank4 = Map1_HI2;

  /* Set ROM Banks */
  Map1_set_ROM_banks();

  /* Set up wiring of the interrupt pin */
  K6502_Set_Int_Wiring( 1, 1 );
}

void Map1_set_ROM_banks()
{
  ROMBANK0 = ROMPAGE( ( (Map1_256K_base << 5) + (Map1_bank1 & ((256/8)-1)) ) % ( NesHeader.byRomSize << 1 ) );  
  ROMBANK1 = ROMPAGE( ( (Map1_256K_base << 5) + (Map1_bank2 & ((256/8)-1)) ) % ( NesHeader.byRomSize << 1 ) );
  ROMBANK2 = ROMPAGE( ( (Map1_256K_base << 5) + (Map1_bank3 & ((256/8)-1)) ) % ( NesHeader.byRomSize << 1 ) );
  ROMBANK3 = ROMPAGE( ( (Map1_256K_base << 5) + (Map1_bank4 & ((256/8)-1)) ) % ( NesHeader.byRomSize << 1 ) ); 
}

/*-------------------------------------------------------------------*/
/*  Mapper 1 Write Function                                          */
/*-------------------------------------------------------------------*/
void Map1_Write( WORD wAddr, BYTE byData )
{
/*
 * MMC1
 */
  NES_DWORD dwRegNum;

  // if write is to a different reg, reset
  if( ( wAddr & 0x6000 ) != ( Map1_Last_Write_Addr & 0x6000 ) )
  {
    Map1_Cnt = 0;
    Map1_Latch = 0x00;
  }
  Map1_Last_Write_Addr = wAddr;

  // if bit 7 set, reset and return
  if ( byData & 0x80 )
  {
    Map1_Cnt = 0;
    Map1_Latch = 0x00;
    return;
  }

  if ( byData & 0x01 ) Map1_Latch |= ( 1 << Map1_Cnt );
  Map1_Cnt++;
  if( Map1_Cnt < 5 ) return;

  dwRegNum = ( wAddr & 0x7FFF ) >> 13;
  Map1_Regs[ dwRegNum ] = Map1_Latch;

  Map1_Cnt = 0;
  Map1_Latch = 0x00;

  switch( dwRegNum )
  {
    case 0:
      {
        // set mirroring
        if( Map1_Regs[0] & 0x02 )
        {
          if( Map1_Regs[0] & 0x01 )
          {
            InfoNES_Mirroring( 0 );
          }
          else
          {
            InfoNES_Mirroring( 1 );
          }
        }
        else
        {
          // one-screen mirroring
          if( Map1_Regs[0] & 0x01 )
          {
            InfoNES_Mirroring( 2 );
          }
          else
          {
            InfoNES_Mirroring( 3 );
          }
        }
      }
      break;

    case 1:
      {
        BYTE byBankNum = Map1_Regs[1];

        if ( Map1_Size == Map1_1024K )
        {
          if ( Map1_Regs[0] & 0x10 )
          {
            if ( Map1_swap )
            {
              Map1_256K_base = (Map1_Regs[1] & 0x10) >> 4;
              if(Map1_Regs[0] & 0x08)
              {
                Map1_256K_base |= ((Map1_Regs[2] & 0x10) >> 3);
              }
              Map1_set_ROM_banks();
              Map1_swap = 0;
            }
            else
            {
              Map1_swap = 1;
            }
          }
          else
          {
            // use 1st or 4th 256K banks
            Map1_256K_base = ( Map1_Regs[1] & 0x10 ) ? 3 : 0;
            Map1_set_ROM_banks();
          }
        }
        else if((Map1_Size == Map1_512K) && (!NesHeader.byVRomSize))
        {
          Map1_256K_base = (Map1_Regs[1] & 0x10) >> 4;
          Map1_set_ROM_banks();
        }
        else if ( NesHeader.byVRomSize )
        {
          // set VROM bank at $0000
          if ( Map1_Regs[0] & 0x10 )
          {
            // swap 4K
            byBankNum <<= 2;
            PPUBANK[ 0 ] = VROMPAGE( (byBankNum+0) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 1 ] = VROMPAGE( (byBankNum+1) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 2 ] = VROMPAGE( (byBankNum+2) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 3 ] = VROMPAGE( (byBankNum+3) % (NesHeader.byVRomSize << 3) );
            InfoNES_SetupChr();
          }
          else
          {
            // swap 8K
            byBankNum <<= 2;
            PPUBANK[ 0 ] = VROMPAGE( (byBankNum+0) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 1 ] = VROMPAGE( (byBankNum+1) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 2 ] = VROMPAGE( (byBankNum+2) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 3 ] = VROMPAGE( (byBankNum+3) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 4 ] = VROMPAGE( (byBankNum+4) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 5 ] = VROMPAGE( (byBankNum+5) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 6 ] = VROMPAGE( (byBankNum+6) % (NesHeader.byVRomSize << 3) );
            PPUBANK[ 7 ] = VROMPAGE( (byBankNum+7) % (NesHeader.byVRomSize << 3) );
            InfoNES_SetupChr();
          }
        }
      }
      break;

    case 2:
      {
        BYTE byBankNum = Map1_Regs[2];

        if((Map1_Size == Map1_1024K) && (Map1_Regs[0] & 0x08))
        {
          if(Map1_swap)
          {
            Map1_256K_base =  (Map1_Regs[1] & 0x10) >> 4;
            Map1_256K_base |= ((Map1_Regs[2] & 0x10) >> 3);
            Map1_set_ROM_banks();
            Map1_swap = 0;
          }
          else
          {
            Map1_swap = 1;
          }
        }

        if(!NesHeader.byVRomSize) 
        {
          if ( Map1_Regs[ 0 ] & 0x10 )
          {
            byBankNum <<= 2;
#if 0
            PPUBANK[ 4 ] = VRAMPAGE0( byBankNum+0 );
            PPUBANK[ 5 ] = VRAMPAGE0( byBankNum+1 );
            PPUBANK[ 6 ] = VRAMPAGE0( byBankNum+2 );
            PPUBANK[ 7 ] = VRAMPAGE0( byBankNum+3 );
#else
            PPUBANK[ 4 ] = CRAMPAGE( byBankNum+0 );
            PPUBANK[ 5 ] = CRAMPAGE( byBankNum+1 );
            PPUBANK[ 6 ] = CRAMPAGE( byBankNum+2 );
            PPUBANK[ 7 ] = CRAMPAGE( byBankNum+3 );
#endif
            InfoNES_SetupChr();
            break;
          }
        }

        // set 4K VROM bank at $1000
        if(Map1_Regs[0] & 0x10)
        {
          // swap 4K
          byBankNum <<= 2;
          PPUBANK[ 4 ] = VROMPAGE( (byBankNum+0) % (NesHeader.byVRomSize << 3) );
          PPUBANK[ 5 ] = VROMPAGE( (byBankNum+1) % (NesHeader.byVRomSize << 3) );
          PPUBANK[ 6 ] = VROMPAGE( (byBankNum+2) % (NesHeader.byVRomSize << 3) );
          PPUBANK[ 7 ] = VROMPAGE( (byBankNum+3) % (NesHeader.byVRomSize << 3) );
          InfoNES_SetupChr(); 
        }
      }
      break;

    case 3:
      {
        BYTE byBankNum = Map1_Regs[3];

        // set ROM bank
        if ( Map1_Regs[0] & 0x08 )
        {
          // 16K of ROM
          byBankNum <<= 1;

          if ( Map1_Regs[0] & 0x04 )
          {
            // 16K of ROM at $8000
            Map1_bank1 = byBankNum;
            Map1_bank2 = byBankNum+1;
            Map1_bank3 = Map1_HI1;
            Map1_bank4 = Map1_HI2;
          }
          else
          {
            // 16K of ROM at $C000
            if(Map1_Size == Map1_SMALL)
            {
              Map1_bank1 = 0;
              Map1_bank2 = 1;
              Map1_bank3 = byBankNum;
              Map1_bank4 = byBankNum+1;
            }
          }
        }
        else
        {
          // 32K of ROM at $8000
          byBankNum <<= 1;

          Map1_bank1 = byBankNum;
          Map1_bank2 = byBankNum+1;
          if(Map1_Size == Map1_SMALL)
          {
            Map1_bank3 = byBankNum+2;
            Map1_bank4 = byBankNum+3;
          }
        }
        Map1_set_ROM_banks();
      }
      break;
  }
}
