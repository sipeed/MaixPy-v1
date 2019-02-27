#include "InfoNES_System.h"
#include "InfoNES.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "fpioa.h"
#include "dmac.h"
#include "fpioa.h"
#include "i2s.h"
#include "plic.h"
#include "uarths.h"
#include "bsp.h"

#include "FreeRTOS.h"
#include "task.h"
#include "InfoNES_System.h"
#include "InfoNES.h"
#include "sysctl.h"
#include "py/mpstate.h"

extern uint8_t g_dvp_buf[];
extern NES_DWORD * FrameBuffer;

extern int wait_us;
extern int audio_turn;	//63
int g_sample_rate = 44100;
uint16_t final_wave[2048];
int waveptr=0;
int wavflag=0;
bool i2s_idle = true;
bool is_exit_to_menu = true;

#define _D //printf("%d\n",__LINE__)


WORD NesPalette[64]={
  0x738E,0x88C4,0xA800,0x9808,0x7011,0x1015,0x0014,0x004F,
  0x0148,0x0200,0x0280,0x11C0,0x59C3,0x0000,0x0000,0x0000,
  0xBDD7,0xEB80,0xE9C4,0xF010,0xB817,0x581C,0x015B,0x0A59,
  0x0391,0x0480,0x0540,0x3C80,0x8C00,0x0000,0x0000,0x0000,
  0xFFDF,0xFDC7,0xFC8B,0xFC48,0xFBDE,0xB39F,0x639F,0x3CDF,
  0x3DDE,0x1690,0x4EC9,0x9FCB,0xDF40,0x0000,0x0000,0x0000,
  0xFFDF,0xFF15,0xFE98,0xFE5A,0xFE1F,0xDE1F,0xB5DF,0xAEDF,
  0xA71F,0xA7DC,0xBF95,0xCFD6,0xF7D3,0x0000,0x0000,0x0000,
};

/* Menu screen */
int InfoNES_Menu()
{
	if(is_exit_to_menu)
		return -1;
	return 0;
}


extern const BYTE nes_rom[];
/* Read ROM image file */
int InfoNES_ReadRom( const char *pszFileName )
{
/*
 *  Read ROM image file
 *
 *  Parameters
 *    const char *pszFileName          (Read)
 *
 *  Return values
 *     0 : Normally
 *    -1 : Error
 */


  // Read ROM Header 
  BYTE * rom = (BYTE*)nes_rom;
  memcpy( &NesHeader, rom, sizeof(NesHeader));
  if ( memcmp( NesHeader.byID, "NES\x1a", 4 ) != 0 )
  {
    // not .nes file 
    return -1;
  }
  rom += sizeof(NesHeader);

  // Clear SRAM 
  memset( SRAM, 0, SRAM_SIZE );

  // If trainer presents Read Triner at 0x7000-0x71ff 
  if ( NesHeader.byInfo1 & 4 )
  {
    //memcpy( &SRAM[ 0x1000 ], rom, 512);
	rom += 512;
  }

  // Allocate Memory for ROM Image 
  ROM = rom;
  rom += NesHeader.byRomSize * 0x4000;

  if ( NesHeader.byVRomSize > 0 )
  {
    // Allocate Memory for VROM Image 
	VROM = (BYTE*)rom;
	rom += NesHeader.byVRomSize * 0x2000;
  }

  // Successful
  return 0;
}


/* Release a memory for ROM */
void InfoNES_ReleaseRom()
{
}

static int exchang_data_byte(uint8_t* addr,uint32_t length)
{
  if(NULL == addr)
    return -1;
  uint8_t data = 0;
  for(int i = 0 ; i < length ;i = i + 2)
  {
    data = addr[i];
    addr[i] = addr[i + 1];
    addr[i + 1] = data;
  }
  return 0;
}

/* Transfer the contents of work frame on the screen */
void InfoNES_LoadFrame()
{
	exchang_data_byte(WorkFrame, NES_DISP_WIDTH*NES_DISP_HEIGHT*2);
	lcd_draw_picture(32, 0, NES_DISP_WIDTH, NES_DISP_HEIGHT, (uint32_t *)WorkFrame);
	return;
}


/* Get a joypad state */
//wasd:上下左右  kl:AB nm:sel,start

#define SELECT_MASK	(1<<0)
#define L3_MASK		(1<<1)
#define R3_MASK		(1<<2)
#define START_MASK	(1<<3)
#define UP_MASK		(1<<4)
#define RIGHT_MASK	(1<<5)
#define DOWN_MASK	(1<<6)
#define LEFT_MASK	(1<<7)

#define L2_MASK		(1<<0)
#define R2_MASK		(1<<1)
#define L1_MASK		(1<<2)
#define R1_MASK		(1<<3)
#define TRI_MASK	(1<<4)
#define CIR_MASK	(1<<5)
#define CRO_MASK	(1<<6)
#define REC_MASK	(1<<7)

extern int nes_stick;

NES_DWORD dwKeyPad1;
NES_DWORD dwKeyPad2;
NES_DWORD dwKeySystem;
static int state[8]={0};

#define REPEAT_N 16
void InfoNES_PadState( NES_DWORD *pdwPad1, NES_DWORD *pdwPad2, NES_DWORD *pdwSystem )
{
	int ch;
	dwKeyPad1=0;
	dwKeyPad2=0;
	dwKeySystem=0;
	if(nes_stick==0)
	{
		while((ch=uart_rx_char(MP_STATE_PORT(Maix_stdio_uart)))!=-1)
		{
			switch((char)ch)
			{
			case 'd':	//right
			  dwKeyPad1 |= ( 1 << 7 );state[7]=REPEAT_N;
			  break;

			case 'a':	//left
			  dwKeyPad1 |= ( 1 << 6 );state[6]=REPEAT_N;
			  break;

			case 's':	//down
			  dwKeyPad1 |= ( 1 << 5 );state[5]=REPEAT_N;
			  break;
			  
			case 'w':	//up
			  dwKeyPad1 |= ( 1 << 4 );state[4]=REPEAT_N;
			  break;

			case 'm':	//start
			  dwKeyPad1 |= ( 1 << 3 );state[3]=0;
			  break;

			case 'n':	//select
			  dwKeyPad1 |= ( 1 << 2 );state[2]=0;
			  break;

			case 'j':   // 'A'
			  dwKeyPad1 |= ( 1 << 1 );state[1]=REPEAT_N;
			  break;

			case 'k': 	// 'B' 
			  dwKeyPad1 |= ( 1 << 0 );state[0]=REPEAT_N;
			  break;
			/**********************/
			case 'r':
				wait_us++;
				printf("wait_us:%d\r\n",wait_us);
				break;
			case 'f':
				wait_us--;
				if(wait_us<0)wait_us=0;
				printf("wait_us:%d\r\n",wait_us);
				break;
			case 't':
				audio_turn++;
				i2s_set_sample_rate(I2S_DEVICE_0, g_sample_rate*audio_turn/100);	
				printf("audio_turn:%d\r\n",audio_turn);
				break;
			case 'g':
				audio_turn--;
				if(audio_turn<0)audio_turn=0;
				i2s_set_sample_rate(I2S_DEVICE_0, g_sample_rate*audio_turn/100);	
				printf("audio_turn:%d\r\n",audio_turn);
				break;
			case 0x1B://ESC
				printf("exit\r\n");
				dwKeySystem |= PAD_SYS_QUIT;
				is_exit_to_menu = true;
				break;
			default:
				break;
			}
		}
		for(int i=0;i<8;i++)
		{
			if(state[i])
			{
				state[i]--;
				dwKeyPad1 |= (1<<i);
			}
		}
	}
	else if(nes_stick==1)
	{
		uint8_t buf[9];
		uint8_t select,l3,r3,start,up,right,down,left;
		uint8_t l2,r2,l1,r1,tri,cir,cro,rec;
		ps2_read_status(buf);
		for (uint8_t i = 0; i < 9; i++)
        {
            //printf("0x%x ", buf[i]);
        }
		//printf("\r\n");
		select=!(buf[3]&SELECT_MASK);
		l3=!(buf[3]&L3_MASK);
		r3=!(buf[3]&R3_MASK);
		start=!(buf[3]&START_MASK);
		up=!(buf[3]&UP_MASK);
		right=!(buf[3]&RIGHT_MASK);
		down=!(buf[3]&DOWN_MASK);
		left=!(buf[3]&LEFT_MASK);
//printf("select=%d, l3=%d, r3=%d, start=%d, up=%d, right=%d, down=%d, left=%d\r\n",\
				select,l3,r3,start,up,right,down,left);
		l2=!(buf[4]&L2_MASK);
		r2=!(buf[4]&R2_MASK);
		l1=!(buf[4]&L1_MASK);
		r1=!(buf[4]&R1_MASK);
		tri=!(buf[4]&TRI_MASK);
		cir=!(buf[4]&CIR_MASK);
		cro=!(buf[4]&CRO_MASK);
		rec=!(buf[4]&REC_MASK);
//printf("l2=%d, r2=%d, l1=%d, r1=%d, tri=%d, cir=%d, cro=%d, rec=%d\r\n",\
				l2,r2,l1,r1,tri,cir,cro,rec);
		//				B 		A 		sel 		start 		up 		down 	left 		right
        dwKeyPad1 = (cro<<0)|(rec<<1)|(select<<2)|(start<<3)|(up<<4)|(down<<5)|(left<<6)|(right<<7);
		
		if(l1){
			wait_us++;
			printf("wait_us:%d\r\n",wait_us);
		}
		if(l2){
			wait_us--;
			if(wait_us<0)wait_us=0;
			printf("wait_us:%d\r\n",wait_us);
		}
		if(r1){
			audio_turn++;
			i2s_set_sample_rate(I2S_DEVICE_0, g_sample_rate*audio_turn/100);	
			printf("audio_turn:%d\r\n",audio_turn);
		}
		if(r2){
			audio_turn--;
			if(audio_turn<0)audio_turn=0;
			i2s_set_sample_rate(I2S_DEVICE_0, g_sample_rate*audio_turn/100);	
			printf("audio_turn:%d\r\n",audio_turn);
		}
	}
	*pdwPad1   = dwKeyPad1;
	*pdwPad2   = dwKeyPad2;
	*pdwSystem = dwKeySystem;
	return;
}


/* memcpy */
void *InfoNES_MemoryCopy( void *dest, const void *src, int count )
{
    return memcpy(dest,src,count);
}


/* memset */
void *InfoNES_MemorySet( void *dest, int c, int count )
{
    return memset(dest,c,count);
}


/* Print debug message */
void InfoNES_DebugPrint( char *pszMsg )
{
}


/* Wait */
void InfoNES_Wait()
{
	usleep(wait_us); //
}


/* For Sound Emulation */


/* Sound Initialize */
void InfoNES_SoundInit( void )
{				  
}

static int on_irq_dma3(void *ctx)
{
	i2s_idle = true;
}

/* Sound Open */
int InfoNES_SoundOpen( int samples_per_sync, int sample_rate )
{
	  waveptr = 0;
  	wavflag = 0;

	//speaker's dac
    fpioa_set_function(34, FUNC_I2S0_OUT_D0);
    fpioa_set_function(35, FUNC_I2S0_SCLK);
    fpioa_set_function(33, FUNC_I2S0_WS);	
	//dmac_init();
	i2s_init(I2S_DEVICE_0, I2S_TRANSMITTER, 0x03); //mask of ch
	i2s_tx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_0,
                          RESOLUTION_16_BIT, SCLK_CYCLES_32,
                          /*TRIGGER_LEVEL_1*/ TRIGGER_LEVEL_4,
                          RIGHT_JUSTIFYING_MODE);
	g_sample_rate = sample_rate;
	printf("samples_per_sync=%d, sample_rate=%d\r\n", samples_per_sync, sample_rate*audio_turn/100);
	// i2s_set_sample_rate(I2S_DEVICE_0, g_sample_rate*audio_turn/100);	
	i2s_set_sample_rate(I2S_DEVICE_0, g_sample_rate);	
	dmac_set_irq(DMAC_CHANNEL3, on_irq_dma3, NULL, 1);
	/* Successful */
	is_exit_to_menu = false;
	return 1;
}

#include "io.h"
extern volatile i2s_t *const i2s[3]; //TODO: remove register, replace with function

/* Sound Close */
void InfoNES_SoundClose( void )
{
		//TODO: replace register version with function
    ier_t u_ier;
    u_ier.reg_data = readl(&i2s[I2S_DEVICE_0]->ier);
    u_ier.ier.ien = 0;
    writel(u_ier.reg_data, &i2s[I2S_DEVICE_0]->ier);
    ccr_t u_ccr;
    u_ccr.reg_data = readl(&i2s[I2S_DEVICE_0]->ccr);
    u_ccr.ccr.dma_tx_en = 0;
    writel(u_ccr.reg_data, &i2s[I2S_DEVICE_0]->ccr);
}


/* Sound Output 5 Waves - 2 Pulse, 1 Triangle, 1 Noise, 1 DPCM */
void InfoNES_SoundOutput(int samples, BYTE *wave1, BYTE *wave2, BYTE *wave3, BYTE *wave4, BYTE *wave5)
{
	int i;
	int16_t tmp;
	for (i = 0; i < samples; i++) 
	{
		// final_wave[ waveptr ] = 
		// (( wave1[i] + wave2[i] + wave3[i] + wave4[i] + wave5[i] ) / 5)<<8;
		tmp = (int16_t)wave1[i] +(int16_t)wave2[i] +(int16_t)wave3[i] +(int16_t)wave4[i] +(int16_t)wave5[i];
		tmp = (int16_t)tmp/5.0*256;
		final_wave[ waveptr ] = (uint16_t)tmp;
		waveptr++;
		if ( waveptr == 2048 ) 
		{
			waveptr = 0;
			wavflag = 2;
		} 
		else if ( waveptr == 1024)
		{
			wavflag = 1;
		}
	}

	if ( i2s_idle && wavflag )
	{
		i2s_idle = false;
		i2s_play(I2S_DEVICE_0, DMAC_CHANNEL3, &final_wave[(wavflag - 1) << 10], 1024,1024, 16, 1);
		//i2s_send_data_dma(I2S_DEVICE_0, &final_wave[(wavflag - 1) << 10], samples * 2, DMAC_CHANNEL3);
		wavflag = 0;
	}
}


/* Print system message */
void InfoNES_MessageBox( char *pszMsg, ... )
{
	char pszErr[ 128 ];
	va_list args;
	// Create the message body
	va_start( args, pszMsg );
	vsprintf( pszErr, pszMsg, args );  pszErr[ 127 ] = '\0';
	va_end( args );
	return;
}