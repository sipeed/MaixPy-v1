/*===================================================================*/
/*                                                                   */
/*  K6502_RW.h : 6502 Reading/Writing Operation for NES              */
/*               This file is included in K6502.cpp                  */
/*                                                                   */
/*  2000/5/23   InfoNES Project ( based on pNesX )                   */
/*                                                                   */
/*===================================================================*/

#ifndef K6502_RW_H_INCLUDED
#define K6502_RW_H_INCLUDED

/*-------------------------------------------------------------------*/
/*  Include files                                                    */
/*-------------------------------------------------------------------*/

#include "InfoNES.h"
#include "InfoNES_System.h"
#include "InfoNES_pAPU.h"


/*===================================================================*/
/*                                                                   */
/*            K6502_ReadZp() : Reading from the zero page            */
/*                                                                   */
/*===================================================================*/
static inline BYTE K6502_ReadZp( BYTE byAddr )
{
/*
 *  Reading from the zero page
 *
 *  Parameters
 *    BYTE byAddr              (Read)
 *      An address inside the zero page
 *
 *  Return values
 *    Read Data
 */

  return NES_RAM[ byAddr ];
}

/*===================================================================*/
/*                                                                   */
/*               K6502_Read() : Reading operation                    */
/*                                                                   */
/*===================================================================*/
static inline BYTE K6502_Read( WORD wAddr )
{
/*
 *  Reading operation
 *
 *  Parameters
 *    WORD wAddr              (Read)
 *      Address to read
 *
 *  Return values
 *    Read data
 *
 *  Remarks
 *    0x0000 - 0x1fff  NES_RAM ( 0x800 - 0x1fff is mirror of 0x0 - 0x7ff )
 *    0x2000 - 0x3fff  PPU
 *    0x4000 - 0x5fff  Sound
 *    0x6000 - 0x7fff  SRAM ( Battery Backed )
 *    0x8000 - 0xffff  ROM
 *
 */
  BYTE byRet;

  switch ( wAddr & 0xe000 )
  {
    case 0x0000:  /* NES_RAM */
      return NES_RAM[ wAddr & 0x7ff ];

    case 0x2000:  /* PPU */
      if ( ( wAddr & 0x7 ) == 0x7 )   /* PPU Memory */
      {
        WORD addr = PPU_Addr & 0x3fff;

        // Set return value;
	byRet = PPU_R7;

        // Increment PPU Address
        PPU_Addr += PPU_Increment;

        // Read PPU Memory
        PPU_R7 = PPUBANK[ addr >> 10 ][ addr & 0x3ff ];

        return byRet;
      }
      else
      if ( ( wAddr & 0x7 ) == 0x4 )   /* SPR_RAM I/O Register */
      {
        return SPRRAM[ PPU_R3++ ];
      }
      else                            
      if ( ( wAddr & 0x7 ) == 0x2 )   /* PPU Status */
      {
        // Set return value
        byRet = PPU_R2;

#if 0
        // Reset a V-Blank flag
        PPU_R2 &= ~R2_IN_VBLANK;
#endif

        // Reset address latch
        PPU_Latch_Flag = 0;

        // Make a Nametable 0 in V-Blank
        if ( PPU_Scanline >= SCAN_VBLANK_START && !( PPU_R0 & R0_NMI_VB ) )
        {
          PPU_R0 &= ~R0_NAME_ADDR;
          PPU_NameTableBank = NAME_TABLE0;
        }
        return byRet;
      }
      else /* $2000, $2001, $2003, $2005, $2006 */
      {
	return PPU_R7;
      }
      break;

    case 0x4000:  /* Sound */
      if ( wAddr == 0x4014 ) 
      {
	return wAddr & 0xff;
      }
      else
      if ( wAddr == 0x4015 )
      {
        // APU control
        byRet = APU_Reg[ 0x19 ];
	if ( ApuC1Atl > 0 ) byRet |= (1<<0);
	if ( ApuC2Atl > 0 ) byRet |= (1<<1);
	if (  !ApuC3Holdnote ) {
	  if ( ApuC3Atl > 0 ) byRet |= (1<<2);
	} else {
	  if ( ApuC3Llc > 0 ) byRet |= (1<<2);
	}
	if ( ApuC4Atl > 0 ) byRet |= (1<<3);

	// FrameIRQ
        APU_Reg[ 0x19 ] &= ~0x40;
        return byRet;
      }
      else
      if ( wAddr == 0x4016 )
      {
        // Set Joypad1 data
        byRet = (BYTE)( ( PAD1_Latch >> PAD1_Bit ) & 1 ) | 0x40;
        PAD1_Bit = ( PAD1_Bit == 23 ) ? 0 : ( PAD1_Bit + 1 );
        return byRet;
      }
      else
      if ( wAddr == 0x4017 )
      {
        // Set Joypad2 data
        byRet = (BYTE)( ( PAD2_Latch >> PAD2_Bit ) & 1 ) | 0x40;
        PAD2_Bit = ( PAD2_Bit == 23 ) ? 0 : ( PAD2_Bit + 1 );
        return byRet;
      }
      else 
      {
        /* Return Mapper Register*/
        return MapperReadApu( wAddr );
      }
      break;
      // The other sound registers are not readable.

    case 0x6000:  /* SRAM */
      if ( ROM_SRAM )
      {
        return SRAM[ wAddr & 0x1fff ];
      } else {    /* SRAM BANK */
        return SRAMBANK[ wAddr & 0x1fff ];
      }

    case 0x8000:  /* ROM BANK 0 */
      return ROMBANK0[ wAddr & 0x1fff ];

    case 0xa000:  /* ROM BANK 1 */
      return ROMBANK1[ wAddr & 0x1fff ];

    case 0xc000:  /* ROM BANK 2 */
      return ROMBANK2[ wAddr & 0x1fff ];

    case 0xe000:  /* ROM BANK 3 */
      return ROMBANK3[ wAddr & 0x1fff ];
  }

  return ( wAddr >> 8 ); /* when a register is not readable the upper half
                            address is returned. */
}

/*===================================================================*/
/*                                                                   */
/*               K6502_Write() : Writing operation                    */
/*                                                                   */
/*===================================================================*/
static inline void K6502_Write( WORD wAddr, BYTE byData )
{
/*
 *  Writing operation
 *
 *  Parameters
 *    WORD wAddr              (Read)
 *      Address to write
 *
 *    BYTE byData             (Read)
 *      Data to write
 *
 *  Remarks
 *    0x0000 - 0x1fff  NES_RAM ( 0x800 - 0x1fff is mirror of 0x0 - 0x7ff )
 *    0x2000 - 0x3fff  PPU
 *    0x4000 - 0x5fff  Sound
 *    0x6000 - 0x7fff  SRAM ( Battery Backed )
 *    0x8000 - 0xffff  ROM
 *
 */

  switch ( wAddr & 0xe000 )
  {
    case 0x0000:  /* NES_RAM */
      NES_RAM[ wAddr & 0x7ff ] = byData;
      break;

    case 0x2000:  /* PPU */
      switch ( wAddr & 0x7 )
      {
        case 0:    /* 0x2000 */
          PPU_R0 = byData;
          PPU_Increment = ( PPU_R0 & R0_INC_ADDR ) ? 32 : 1;
          PPU_NameTableBank = NAME_TABLE0 + ( PPU_R0 & R0_NAME_ADDR );
          PPU_BG_Base = ( PPU_R0 & R0_BG_ADDR ) ? ChrBuf + 256 * 64 : ChrBuf;
          PPU_SP_Base = ( PPU_R0 & R0_SP_ADDR ) ? ChrBuf + 256 * 64 : ChrBuf;
          PPU_SP_Height = ( PPU_R0 & R0_SP_SIZE ) ? 16 : 8;

          // Account for Loopy's scrolling discoveries
	  PPU_Temp = ( PPU_Temp & 0xF3FF ) | ( ( ( (WORD)byData ) & 0x0003 ) << 10 );
          break;

        case 1:   /* 0x2001 */
          PPU_R1 = byData;
          break;

        case 2:   /* 0x2002 */
#if 0	  
          PPU_R2 = byData;     // 0x2002 is not writable
#endif
          break;

        case 3:   /* 0x2003 */
          // Sprite NES_RAM Address
          PPU_R3 = byData;
          break;

        case 4:   /* 0x2004 */
          // Write data to Sprite NES_RAM
          SPRRAM[ PPU_R3++ ] = byData;
          break;

        case 5:   /* 0x2005 */
          // Set Scroll Register
          if ( PPU_Latch_Flag )
          {
            // V-Scroll Register
            PPU_Scr_V_Next = ( byData > 239 ) ? byData - 240 : byData;	    
	    if ( byData > 239 ) PPU_NameTableBank ^= NAME_TABLE_V_MASK; 
            PPU_Scr_V_Byte_Next = PPU_Scr_V_Next >> 3;
            PPU_Scr_V_Bit_Next = PPU_Scr_V_Next & 7;

            // Added : more Loopy Stuff
	    PPU_Temp = ( PPU_Temp & 0xFC1F ) | ( ( ( (WORD)byData ) & 0xF8 ) << 2);
	    PPU_Temp = ( PPU_Temp & 0x8FFF ) | ( ( ( (WORD)byData ) & 0x07 ) << 12);
          }
          else
          {
            // H-Scroll Register
            PPU_Scr_H_Next = byData;
            PPU_Scr_H_Byte_Next = PPU_Scr_H_Next >> 3;
            PPU_Scr_H_Bit_Next = PPU_Scr_H_Next & 7;

            // Added : more Loopy Stuff
	    PPU_Temp = ( PPU_Temp & 0xFFE0 ) | ( ( ( (WORD)byData ) & 0xF8 ) >> 3 );
          }
          PPU_Latch_Flag ^= 1;
          break;

        case 6:   /* 0x2006 */
          // Set PPU Address
          if ( PPU_Latch_Flag )
          {
            /* Low */
#if 0
            PPU_Addr = ( PPU_Addr & 0xff00 ) | ( (WORD)byData );
#else
            PPU_Temp = ( PPU_Temp & 0xFF00 ) | ( ( (WORD)byData ) & 0x00FF);
	    PPU_Addr = PPU_Temp;
#endif
	    if ( !( PPU_R2 & R2_IN_VBLANK ) ) {
	      InfoNES_SetupScr();
	    }
          }
          else
          {
            /* High */
#if 0
            PPU_Addr = ( PPU_Addr & 0x00ff ) | ( (WORD)( byData & 0x3f ) << 8 );
            InfoNES_SetupScr();
#else
            PPU_Temp = ( PPU_Temp & 0x00FF ) | ( ( ((WORD)byData) & 0x003F ) << 8 );
#endif            
          }
          PPU_Latch_Flag ^= 1;
          break;

        case 7:   /* 0x2007 */
          {
            WORD addr = PPU_Addr;
            
            // Increment PPU Address
            PPU_Addr += PPU_Increment;
            addr &= 0x3fff;

            // Write to PPU Memory
            if ( addr < 0x2000 && byVramWriteEnable )
            {
              // Pattern Data
              ChrBufUpdate |= ( 1 << ( addr >> 10 ) );
              PPUBANK[ addr >> 10 ][ addr & 0x3ff ] = byData;
            }
            else
            if ( addr < 0x3f00 )  /* 0x2000 - 0x3eff */
            {
              // Name Table and mirror
              PPUBANK[   addr            >> 10 ][ addr & 0x3ff ] = byData;
              PPUBANK[ ( addr ^ 0x1000 ) >> 10 ][ addr & 0x3ff ] = byData;
            }
            else
            if ( !( addr & 0xf ) )  /* 0x3f00 or 0x3f10 */
            {
              // Palette mirror
              PPURAM[ 0x3f10 ] = PPURAM[ 0x3f14 ] = PPURAM[ 0x3f18 ] = PPURAM[ 0x3f1c ] = 
              PPURAM[ 0x3f00 ] = PPURAM[ 0x3f04 ] = PPURAM[ 0x3f08 ] = PPURAM[ 0x3f0c ] = byData;
              PalTable[ 0x00 ] = PalTable[ 0x04 ] = PalTable[ 0x08 ] = PalTable[ 0x0c ] =
              PalTable[ 0x10 ] = PalTable[ 0x14 ] = PalTable[ 0x18 ] = PalTable[ 0x1c ] = NesPalette[ byData ] | 0x8000;
            }
            else
	    if ( addr & 3 )
            {
              // Palette
              PPURAM[ addr ] = byData;
              PalTable[ addr & 0x1f ] = NesPalette[ byData ];
            }
          }
          break;
      }
      break;

    case 0x4000:  /* Sound */
      switch ( wAddr & 0x1f )
      {
        case 0x00:
        case 0x01:
        case 0x02:
        case 0x03:          
        case 0x04:
        case 0x05:
        case 0x06:
        case 0x07:
        case 0x08:
        case 0x09:
        case 0x0a:
        case 0x0b:
        case 0x0c:
        case 0x0d:
        case 0x0e:
        case 0x0f:
        case 0x10:
        case 0x11:	  
        case 0x12:
        case 0x13:
          // Call Function corresponding to Sound Registers
          if ( !APU_Mute )
            pAPUSoundRegs[ wAddr & 0x1f ]( wAddr, byData );
          break;

        case 0x14:  /* 0x4014 */
          // Sprite DMA
          switch ( byData >> 5 )
          {
            case 0x0:  /* NES_RAM */
              InfoNES_MemoryCopy( SPRRAM, &NES_RAM[ ( (WORD)byData << 8 ) & 0x7ff ], SPRRAM_SIZE );
              break;

            case 0x3:  /* SRAM */
              InfoNES_MemoryCopy( SPRRAM, &SRAM[ ( (WORD)byData << 8 ) & 0x1fff ], SPRRAM_SIZE );
              break;

            case 0x4:  /* ROM BANK 0 */
              InfoNES_MemoryCopy( SPRRAM, &ROMBANK0[ ( (WORD)byData << 8 ) & 0x1fff ], SPRRAM_SIZE );
              break;

            case 0x5:  /* ROM BANK 1 */
              InfoNES_MemoryCopy( SPRRAM, &ROMBANK1[ ( (WORD)byData << 8 ) & 0x1fff ], SPRRAM_SIZE );
              break;

            case 0x6:  /* ROM BANK 2 */
              InfoNES_MemoryCopy( SPRRAM, &ROMBANK2[ ( (WORD)byData << 8 ) & 0x1fff ], SPRRAM_SIZE );
              break;

            case 0x7:  /* ROM BANK 3 */
              InfoNES_MemoryCopy( SPRRAM, &ROMBANK3[ ( (WORD)byData << 8 ) & 0x1fff ], SPRRAM_SIZE );
              break;
          }
          break;

        case 0x15:  /* 0x4015 */
          InfoNES_pAPUWriteControl( wAddr, byData );
#if 0
          /* Unknown */
          if ( byData & 0x10 ) 
          {
	    byData &= ~0x80;
	  }
#endif
          break;

        case 0x16:  /* 0x4016 */
	  // For VS-Unisystem
	  MapperApu( wAddr, byData );
          // Reset joypad
          if ( !( APU_Reg[ 0x16 ] & 1 ) && ( byData & 1 ) )
          {
            PAD1_Bit = 0;
            PAD2_Bit = 0;
          }
          break;

        case 0x17:  /* 0x4017 */
          // Frame IRQ
          FrameStep = 0;
          if ( !( byData & 0xc0 ) )
          {
            FrameIRQ_Enable = 1;
          } else {
            FrameIRQ_Enable = 0;
          }
          break;
      }
//TODO:
      if ( wAddr <= 0x4017 )
      {
        /* Write to APU Register */
        APU_Reg[ wAddr & 0x1f ] = byData;
      }
      else
      {
        /* Write to APU */
        MapperApu( wAddr, byData );
      }
      break;

    case 0x6000:  /* SRAM */
      SRAM[ wAddr & 0x1fff ] = byData;

      /* Write to SRAM, when no SRAM */
      if ( !ROM_SRAM )
      {
        MapperSram( wAddr, byData );
      }
      break;

    case 0x8000:  /* ROM BANK 0 */
    case 0xa000:  /* ROM BANK 1 */
    case 0xc000:  /* ROM BANK 2 */
    case 0xe000:  /* ROM BANK 3 */
      // Write to Mapper
      MapperWrite( wAddr, byData );
      break;
  }
}

// Reading/Writing operation (WORD version)
static inline WORD K6502_ReadW( WORD wAddr ){ return K6502_Read( wAddr ) | (WORD)K6502_Read( wAddr + 1 ) << 8; };
static inline void K6502_WriteW( WORD wAddr, WORD wData ){ K6502_Write( wAddr, wData & 0xff ); K6502_Write( wAddr + 1, wData >> 8 ); };
static inline WORD K6502_ReadZpW( BYTE byAddr ){ return K6502_ReadZp( byAddr ) | ( K6502_ReadZp( byAddr + 1 ) << 8 ); };

// 6502's indirect absolute jmp(opcode: 6C) has a bug (added at 01/08/15 )
static inline WORD K6502_ReadW2( WORD wAddr )
{ 
  if ( 0x00ff == ( wAddr & 0x00ff ) )
  {
    return K6502_Read( wAddr ) | (WORD)K6502_Read( wAddr - 0x00ff ) << 8;
  } else {
    return K6502_Read( wAddr ) | (WORD)K6502_Read( wAddr + 1 ) << 8;
  }
}

#endif /* !K6502_RW_H_INCLUDED */
