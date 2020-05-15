#include <stdio.h>
#include <float.h>
#include <limits.h>

#include "imlib.h"
#include "xalloc.h"


static void svd22(const float a[4], float u[4], float s[2], float v[4])
{
    s[0] = (sqrt(pow(a[0] - a[3], 2) + pow(a[1] + a[2], 2)) + sqrt(pow(a[0] + a[3], 2) + pow(a[1] - a[2], 2))) / 2;
    s[1] = fabs(s[0] - sqrt(pow(a[0] - a[3], 2) + pow(a[1] + a[2], 2)));
    v[2] = (s[0] > s[1]) ? sin((atan2(2 * (a[0] * a[1] + a[2] * a[3]), a[0] * a[0] - a[1] * a[1] + a[2] * a[2] - a[3] * a[3])) / 2) : 0;
    v[0] = sqrt(1 - v[2] * v[2]);
    v[1] = -v[2];
    v[3] = v[0];
    u[0] = (s[0] != 0) ? -(a[0] * v[0] + a[1] * v[2]) / s[0] : 1;
    u[2] = (s[0] != 0) ? -(a[2] * v[0] + a[3] * v[2]) / s[0] : 0;
    u[1] = (s[1] != 0) ? (a[0] * v[1] + a[1] * v[3]) / s[1] : -u[2];
    u[3] = (s[1] != 0) ? (a[2] * v[1] + a[3] * v[3]) / s[1] : u[0];
    v[0] = -v[0];
    v[2] = -v[2];
}

//2D affine
#define MAX_POINT_CNT 10
void imlib_affine_getTansform(uint16_t *src, uint16_t *dst, uint16_t cnt, float* TT)
{
    int i, j, k;
    float src_mean[2] = {0.0f};
    float dst_mean[2] = {0.0f};
    for(i = 0; i < cnt * 2; i += 2)
    {
        src_mean[0] += dst[i];
        src_mean[1] += dst[i + 1];
        dst_mean[0] += src[i];
        dst_mean[1] += src[i + 1];
    }
    src_mean[0] /= cnt;
    src_mean[1] /= cnt;
    dst_mean[0] /= cnt;
    dst_mean[1] /= cnt;

    float src_demean[MAX_POINT_CNT][2] = {0.0f};
    float dst_demean[MAX_POINT_CNT][2] = {0.0f};

    for(i = 0; i < cnt; i++)
    {
        src_demean[i][0] = dst[2 * i] - src_mean[0];
        src_demean[i][1] = dst[2 * i + 1] - src_mean[1];
        dst_demean[i][0] = src[2 * i] - dst_mean[0];
        dst_demean[i][1] = src[2 * i + 1] - dst_mean[1];
    }

    float A[2][2] = {0.0f};
    for(i = 0; i < 2; i++)
    {
        for(k = 0; k < 2; k++)
        {
            for(j = 0; j < cnt; j++)
            {
                A[i][k] += dst_demean[j][i] * src_demean[j][k];
            }
            A[i][k] /= cnt;
        }
    }

    float(*T)[3] = (float(*)[3]) TT;
    T[0][0] = 1;T[0][1] = 0;T[0][2] = 0;
    T[1][0] = 0;T[1][1] = 1;T[1][2] = 0;
    T[2][0] = 0;T[2][1] = 0;T[2][2] = 1;

    float U[2][2] = {0};
    float S[2] = {0};
    float V[2][2] = {0};
    svd22(&A[0][0], &U[0][0], S, &V[0][0]);

    T[0][0] = U[0][0] * V[0][0] + U[0][1] * V[1][0];
    T[0][1] = U[0][0] * V[0][1] + U[0][1] * V[1][1];
    T[1][0] = U[1][0] * V[0][0] + U[1][1] * V[1][0];
    T[1][1] = U[1][0] * V[0][1] + U[1][1] * V[1][1];

    float scale = 1.0f;
    float src_demean_mean[2] = {0.0f};
    float src_demean_var[2] = {0.0f};
    for(i = 0; i < cnt; i++)
    {
        src_demean_mean[0] += src_demean[i][0];
        src_demean_mean[1] += src_demean[i][1];
    }
    src_demean_mean[0] /= cnt;
    src_demean_mean[1] /= cnt;

    for(i = 0; i < cnt; i++)
    {
        src_demean_var[0] += (src_demean_mean[0] - src_demean[i][0]) * (src_demean_mean[0] - src_demean[i][0]);
        src_demean_var[1] += (src_demean_mean[1] - src_demean[i][1]) * (src_demean_mean[1] - src_demean[i][1]);
    }
    src_demean_var[0] /= (cnt);
    src_demean_var[1] /= (cnt);
    scale = 1.0f / (src_demean_var[0] + src_demean_var[1]) * (S[0] + S[1]);
    T[0][2] = dst_mean[0] - scale * (T[0][0] * src_mean[0] + T[0][1] * src_mean[1]);
    T[1][2] = dst_mean[1] - scale * (T[1][0] * src_mean[0] + T[1][1] * src_mean[1]);
    T[0][0] *= scale;
    T[0][1] *= scale;
    T[1][0] *= scale;
    T[1][1] *= scale;
}

int imlib_affine_ai(image_t* src_img, image_t* dst_img, float* TT)
{
    int channels;
    int step = src_img->w;
    int color_step = src_img->w * src_img->h;
    int i, j, k;
	uint8_t* src_buf = src_img->pix_ai;
	uint8_t* dst_buf = dst_img->pix_ai;

	if(src_img->bpp != dst_img->bpp)
	{
		return -1;
	}
	if(src_buf == NULL || dst_buf == NULL )
	{
		return -2;
	}
	if(src_img->bpp != IMAGE_BPP_GRAYSCALE && src_img->bpp!= IMAGE_BPP_RGB565)
	{
		return -3;
	}
	if(src_img->bpp == IMAGE_BPP_GRAYSCALE){
		channels = 1;
	} else {	//RGB565
		channels = 3;
	}
    
	int dst_color_step = dst_img->w * dst_img->h;
    int dst_step = dst_img->w;

    memset(dst_buf, 0, dst_img->w * dst_img->h * channels);

    int pre_x, pre_y; //缩放前对应的像素点坐标
    int x, y;
    unsigned short color[2][2];
    float(*T)[3] = (float(*)[3])TT;
	/*printf("==========\r\n");
	printf("%.3f,%.3f,%.3f\r\n",T[0][0],T[0][1],T[0][2]);
	printf("%.3f,%.3f,%.3f\r\n",T[1][0],T[1][1],T[1][2]);
	printf("%.3f,%.3f,%.3f\r\n",T[2][0],T[2][1],T[2][2]);
	printf("##############\r\n");*/
    for(i = 0; i < dst_img->h; i++)
    {
        for(j = 0; j < dst_img->w; j++)
        {
            pre_x = (int)(T[0][0] * (j << 8) + T[0][1] * (i << 8) + T[0][2] * (1 << 8));
            pre_y = (int)(T[1][0] * (j << 8) + T[1][1] * (i << 8) + T[1][2] * (1 << 8));

            y = pre_y & 0xFF;
            x = pre_x & 0xFF;
            pre_x >>= 8;
            pre_y >>= 8;
            if(pre_x < 0 || pre_x > (src_img->w - 1) || pre_y < 0 || pre_y > (src_img->h - 1))
                continue;
            for(k = 0; k < channels; k++)
            {
                color[0][0] = src_buf[pre_y * step + pre_x + k * color_step];
                color[1][0] = src_buf[pre_y * step + (pre_x + 1) + k * color_step];
                color[0][1] = src_buf[(pre_y + 1) * step + pre_x + k * color_step];
                color[1][1] = src_buf[(pre_y + 1) * step + (pre_x + 1) + k * color_step];
                int final = (256 - x) * (256 - y) * color[0][0] + x * (256 - y) * color[1][0] + (256 - x) * y * color[0][1] + x * y * color[1][1];
                final = final >> 16;
                dst_buf[i * dst_step + j + k * dst_color_step] = final;
            }
        }
    }
	return 0;
}

int imlib_affine(image_t* src_img, image_t* dst_img, float* T)
{
    #include "printf.h"
	mp_printf(&mp_plat_print,"not support yet, please use imlib_affine_ai, then ai_to_pix\r\n");
	return -1;
}