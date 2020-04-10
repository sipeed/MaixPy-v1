//MAIX conv acc

#include "imlib.h"
#include "sipeed_conv.h"

#define _P(...) mp_printf(&mp_plat_print, __VA_ARGS__)

//  卷积	池化	批归一化	激活	输出偏置
static float conv_data[9*3*3] ={
//R
-1,-1,-1,-1,8,-1,-1,-1,-1,
0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,
//G
0,0,0,0,0,0,0,0,0,
-1,-1,-1,-1,8,-1,-1,-1,-1,
0,0,0,0,0,0,0,0,0,
//B
0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,
-1,-1,-1,-1,8,-1,-1,-1,-1,
};

// extern uint8_t g_ai_buf_out[];
static volatile uint8_t _ai_done_flag;
static int kpu_done(void *ctx)
{
    _ai_done_flag = 1;
    return 0;
}

void imlib_conv3(image_t *img, float *krn)
{
	uint8_t* r;
	uint8_t* g;
	uint8_t* b;
	uint16_t* pix;
	int i, j;
	image_t img2;
	img2.w = (img->w + 63)&(~0x3F);
	img2.h = img->h;
	img2.bpp = img->bpp;
	img2.pixels = img->pixels;
	img2.pix_ai = img->pix_ai;
	// uint16_t c;
	//do conv cal
	kpu_task_t task;
	if(img->pix_ai == NULL || img->pixels == NULL) 
	{
		mp_printf(&mp_plat_print, "pix_ai or pixels is NULL!\n");
		return;
	}
	// padding to align 64 every row
	//TODO: maybe need check buffer size, ai buff alloc by pix_to_ai() 64B align already, alloc by sensor too
	if(img2.w != img->w)
	{
		for(i=2; i>=0; --i)
		{
			for(j=img2.h-1; j>=0; --j)
			{
				memmove(img2.pix_ai + img2.w*img2.h*i + j*img2.w, img2.pix_ai + img->w*img->h*i + j*img->w, img->w);
				memset(img2.pix_ai + img2.w*img2.h*i + j*img2.w + img->w, 0, 64 - (img->w%64));
			}
		}
	}

	// prepare conv
	r=img2.pix_ai;
	g=img2.pix_ai+(img2.w)*(img2.h);
	b=img2.pix_ai+(img2.w)*(img2.h)*2;
	pix = (uint16_t*)img2.pixels;
	//prepare conv kern
	memset((void*)conv_data,0,9*3*3*sizeof(float));	//clear
	for(j=0;j<9;j++)conv_data[0*27+0*9+j]=krn[j];
	for(j=0;j<9;j++)conv_data[1*27+1*9+j]=krn[j];
	for(j=0;j<9;j++)conv_data[2*27+2*9+j]=krn[j];
	//conv cal
	_ai_done_flag = 0;
	#if 0
	_P("w=%d, h=%d\n", img2.w, img2.h);
	for(j=0;j<8;j++)_P("%04x ",(img2.pixels)[j]);_P("\n");
	for(j=0;j<8;j++)_P("%04x ",r[j]);_P("\n");
	for(j=0;j<8;j++)_P("%04x ",g[j]);_P("\n");
	for(j=0;j<8;j++)_P("%04x ",b[j]);_P("\n");
	#endif
	// unsigned long t0,t1;
	//t0=read_cycle();
	sipeed_conv_init(&task, img2.w, img2.h, 3, 3, conv_data);
	sipeed_conv_run(&task, img2.pix_ai, img2.pix_ai, kpu_done);
	while(!_ai_done_flag);
    _ai_done_flag=0;
	//t1=read_cycle();
	//mp_printf(&mp_plat_print, "conv: %ld-%ld=%ld, %ld us!\r\n",t1,t0,(t1-t0),((t1-t0)*1000000/400000000)); 
	//convert R8G8B8 to lcd's RGB565 
	//t0=read_cycle();
	
	// recover padding
	if(img2.w != img->w)
	{
		for(i=0; i<3; ++i)
		{
			for(j=0; j<img2.h; ++j)
			{
				memmove(img2.pix_ai + img->w * img->h * i + j * img->w, img2.pix_ai + img2.w * img2.h * i + j * img2.w,  img->w);
			}
		}
	}
	r=img->pix_ai;
	g=img->pix_ai+(img->w)*(img->h);
	b=img->pix_ai+(img->w)*(img->h)*2;
	
	// ai_to_pix
	for(i = 0; i < (img->w)*(img->h); i++)
	{
		//pix[i] = COLOR_R8_G8_B8_TO_RGB565(r[i],g[i],b[i]);  //5ms
		//(_r5 << 3) | (_g6 >> 3) | ((_g6 & 0x7) << 13) | (_b5 << 8);
		pix[i] = (((uint16_t)(r[i]>>3))<<3)+(((uint16_t)g[i])>>5)+(((uint16_t)b[i]>>3)<<8)+(((uint16_t)(g[i]&0x1C)<<11));	//2.6ms
	}
	//t1=read_cycle();
	//mp_printf(&mp_plat_print, "RGB565: %ld-%ld=%ld, %ld us!\r\n",t1,t0,(t1-t0),((t1-t0)*1000000/400000000)); 
	return;
}

