/*******       DTW.C         ********/
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "ADC.h"
#include "VAD.h"
#include "MFCC.h"
#include "DTW.h"

/*
 *	DTW算法 通过局部优化的方法实现加权距离总和最小

	时间规整函数：
	C={c(1),c(2),…,c(N)}
	N为路径长度，c(n)=(i(n),j(n))表示第n个匹配点是有参考模板的
第i(n)个特征矢量与待测模板的第j(n)个特征矢量构成的匹配点对，两
者之间的距离d(x(i(n)),y(j(n)))称为匹配距离。
	时间规整函数满足一下约束：
	1.单调性，规整函数单调增加。
	2.起点终点约束，起点对起点，终点对终点。
	3.连续性，不允许跳过任何一点。
	4.最大规整量不超过某一极限值。|i(n)-j(n)|<M,M为窗宽。规整
函数所处的区域位于平行四边形内。局部路径约束，用于限制当第n步
为(i(n),j(n))时，前几步存在几种可能的路径。

	DTW步骤：
	1.初始化。令i(0)=j(0)=0,i(N)=in_frm_num,j(N)=mdl_frm_num.
确定一个平行四边形，有两个位于(0,0)和(in_frm_num,mdl_frm_num)的顶点，相邻斜边斜
率分别是2和1/2.规整函数不可超出此平行四边形。
	2.递推求累计距离。

	若输入特征与特征模板的帧数差别过大，直接将匹配距离设为最大
	frm_in_num<(frm_mdl_num/2)||frm_in_num>(frm_mdl_num*2)
*/
int64_t avr_in[mfcc_num];
int64_t avr_mdl[mfcc_num];

int64_t standard_in[mfcc_num];
int64_t standard_mdl[mfcc_num];
int dtw_data[vv_frm_max*vv_frm_max];

struct pointOritation//节点方向，用来回溯每个W点
{
	int frontI, frontJ;
};
int g[vv_frm_max][vv_frm_max];
struct pointOritation pOritation[vv_frm_max][vv_frm_max];//用来存放

void gArray(int *p, int n, int m, int *g, struct pointOritation *pr)
{
	int i = 0, j = 0;

	*g = (*p) * 2;//起始点（最左上角的点）
	for (i = 1; i < m; i++) {//最上面一横
		*(g + i) =  *(g + i - 1) + *(p + i);
		(pr + i)->frontI = 0;
		(pr + i)->frontJ = i - 1;
	}

	for (i = 1; i < n; i++) {//最左边的一竖
		*(g+i*m+0) =  *(g+(i-1)*m+0)+(*(p+i*m+0));
		(pr+i*m+0)->frontI = i-1;
		(pr+i*m+0)->frontJ = 0;
	}

    //计算剩余网格的G值
	for (i = 1; i < n; i++) {
		for (j = 1; j < m; j++) {
			int left, up, incline;

			left = *(g + i*m+j-1) + *(p + i * m + j);
			up = *(g + (i - 1) * m + j) + *(p + i * m + j);
			incline = *(g + (i - 1) * m + j - 1) + (*(p + i * m + j)) * 2;

	    //从左、上、斜三个方向选出最小的
			int min = left;

			*(g + i * m + j) = min;
			(pr + i * m + j)->frontI = i;
			(pr + i * m + j)->frontJ = j - 1;

			if (min > up) {
				min = up;
				*(g + i * m + j) = min;
				(pr + i * m + j)->frontI = i - 1;
				(pr + i * m + j)->frontJ = j;
			}
			if (min > incline) {
				min = incline;
				*(g + i * m + j) = min;
				(pr + i * m + j)->frontI = i - 1;
				(pr + i * m + j)->frontJ = j - 1;
			}
		}
	}
#if 0
    //输出G数组
	for (i = 0; i < n; i++) {
		for (j = 0; j < m; j++)
			printf("%d, ", *(g+i*m+j));
		printf("\n");
	}
	//输出方向数组
	for (i = 0; i < n; i++) {
		for (j = 0; j < m; j++)
			printf("(%d,%d), ", (pr+i*m+j)->frontI, (pr+i*m+j)->frontJ);
		printf("\n");
	}
   #endif
}
int printPath(struct pointOritation *po, int n, int m, int *g)
{
    //从最后一个点向前输出路径节点
	int i = n-1, j = m - 1;
	int step = 0;

	while (1) {
		int ii = (po + i * m + j)->frontI, jj = (po + i * m + j)->frontJ;

		if (i == 0 && j == 0)
			break;
 //       printf("(%d,%d):%d\n",i,j,*(g+i*m+j));
		i = ii;
		j = jj;
		step++;
	}
 //   printf("distance1 = %d\n", *(g+(n-1)*m + m -1) / step);
	return step;
	//printf("distance = %d\n", *(g+(n-1)*m + m -1) / step);
}

/*
 *	获取两个特征矢量之间的距离
	参数
	frm_ftr1	特征矢量1
	frm_ftr2	特征矢量2
	返回值
	dis			矢量距离
*/
uint32_t get_dis(int16_t *frm_ftr1, int16_t *frm_ftr2)
{
	uint8_t	i;

#if 0
	#if 1
	uint32_t	dis;
	int32_t dif;	//两矢量相同维度上的差值

	dis = 0;
	for (i = 0; i < mfcc_num; i++) {
		//USART1_printf("dis=%d ",dis);
		dif = frm_ftr1[i]-frm_ftr2[i];
		dis += (dif*dif);
	}
	//USART1_printf("dis=%d ",dis);
	dis = sqrtf(dis);
	//USART1_printf("%d\r\n",dis);
	return dis;
	#else
	uint32_t	dis;
	int32_t dif;
	int32_t avr;

	dis = 0;
	for (i = 0; i < mfcc_num; i++) {
		//USART1_printf("dis=%d ",dis);
		avr = (frm_ftr1[i]+frm_ftr2[i]) / 2;
		dif = sqrtf((frm_ftr1[i] - avr) * (frm_ftr1[i] - avr) + (frm_ftr2[i] - avr) * (frm_ftr2[i] - avr));

		dif = (frm_ftr1[i]-frm_ftr2[i]) * 100 / dif;

		dis += (dif*dif);
	}
	//USART1_printf("dis=%d ",dis);
	dis = sqrtf(dis);
	//USART1_printf("%d\r\n",dis);
	return dis;
	#endif
#else
	#if 1
	int64_t dif = 0;
	int64_t dif_a = 0;
	int64_t dif_b = 0;
	int32_t dif_r = 0;

	for (i = 0; i < mfcc_num; i++) {
		dif += (frm_ftr1[i]*frm_ftr2[i]);

		dif_a += (frm_ftr1[i] * frm_ftr1[i]);
		dif_b += (frm_ftr2[i] * frm_ftr2[i]);
	}
	dif_r = 1000 - (dif * 1000 / sqrt(dif_a * dif_b));

	return dif_r;
	#else
	int64_t dif = 0;
	int64_t dif_a = 0;
	int64_t dif_b = 0;
	int32_t dif_r = 0;

	for (i = 0; i < mfcc_num; i++) {
		dif += ((frm_ftr1[i] - avr_mdl[i]) * (frm_ftr2[i] - avr_in[i]));
		dif_a += ((frm_ftr1[i] - avr_mdl[i]) * (frm_ftr1[i] - avr_mdl[i]));
		dif_b += ((frm_ftr2[i] - avr_in[i]) * (frm_ftr2[i] - avr_in[i]));
	}
	dif_r = 1000 - (dif * 1000 / (sqrt(dif_a * dif_b)));

	return dif_r;
	#endif

#endif
}

//平行四边形两外两顶点 X坐标值
static uint16_t	X1;			//上边交点
static uint16_t	X2;			//下边交点
static int	in_frm_num;	//输入特征帧数
static int	mdl_frm_num;//特征模板帧数

#define ins		0
#define outs	1

/*
 *	范围控制

*/
uint8_t dtw_limit(uint16_t x, uint16_t y)
{
	if (x < X1) {
		if (y >= ((2*x)+2))
			return outs;
	} else {
		if ((2*y+in_frm_num-2*mdl_frm_num) >= (x+4))
			return outs;
	}

	if (x < X2) {
		if ((2*y+2) <= x)
			return outs;
	} else {
		if ((y+4) <= (2*x+mdl_frm_num-2*in_frm_num))
			return outs;
	}

	return ins;
}

/*
 *	DTW 动态时间规整
	参数
	ftr_in	:输入特征值
	ftr_mdl	:特征模版
	返回值
	dis		:累计匹配距离
*/

uint32_t dtw(v_ftr_tag *ftr_in, v_ftr_tag *frt_mdl)
{
	uint32_t dis;
//	uint16_t x, y;
	uint16_t step;
	int16_t *in;
	int16_t *mdl;
//	uint32_t d_right_up, right, right_up; //up,
//	uint32_t min;
	int i, j;

	in_frm_num = ftr_in->frm_num;
	mdl_frm_num = frt_mdl->frm_num;

	if ((in_frm_num > (mdl_frm_num*3)) || ((3*in_frm_num) < mdl_frm_num)) {
		//USART1_printf("in_frm_num=%d mdl_frm_num=%d\r\n", in_frm_num,mdl_frm_num);
		return dis_err;
	} else {
		// 计算约束平行四边形顶点值
		X1 = (2*mdl_frm_num-in_frm_num)/3;
		X2 = (4*in_frm_num-2*mdl_frm_num)/3;
		in = ftr_in->mfcc_dat;
		mdl = frt_mdl->mfcc_dat;
#if 0
		for (j = 0; j < mfcc_num; j++) {
			avr_in[j] = 0;
			avr_mdl[j] = 0;
		}

		for (j = 0; j < mfcc_num; j++)
			for (i = 0; i < in_frm_num; i++)
				avr_in[j] += in[mfcc_num * i + j];
		for (j = 0; j < mfcc_num; j++)
			avr_in[j] = avr_in[j] / in_frm_num;
//
		for (j = 0; j < mfcc_num; j++)
			for (i = 0; i < mdl_frm_num; i++)
				avr_mdl[j] += mdl[mfcc_num * i + j];
		for (j = 0; j < mfcc_num; j++)
			avr_mdl[j] = avr_mdl[j] / mdl_frm_num;

		dis = get_dis(in, mdl);
		x = 1;
		y = 1;
		step = 1;
#endif
#if 0
		for (i = 0; i < in_frm_num; i++) {
			for (j = 0; j < mdl_frm_num; j++) {
				printf("%d,", get_dis(mdl, in));
				mdl += mfcc_num;
			}
			mdl = frt_mdl->mfcc_dat;
			in += mfcc_num;
			printf("\n");
		}
		in = ftr_in->mfcc_dat;
		mdl = frt_mdl->mfcc_dat;
#endif
		for (i = 0; i < in_frm_num; i++) {
			for (j = 0; j < mdl_frm_num; j++) {
				//dtw_data[i][j] = get_dis(in + (i * mfcc_num), mdl + (j * mfcc_num));
				//dtw_data[i][j] = get_dis(in, mdl);
				*(dtw_data+i*mdl_frm_num+j) = get_dis(in, mdl);
			//	printf("%d,", dtw_data[i*mdl_frm_num+j]);
				mdl += mfcc_num;
			}
			//printf("\n");
			mdl = frt_mdl->mfcc_dat;
			in += mfcc_num;
		}
		gArray(dtw_data, in_frm_num, mdl_frm_num, *g, *pOritation);
		step = printPath(*pOritation, in_frm_num, mdl_frm_num, *g);
		//printf("step=%d\r\n",step);
		dis = *((int *)g+(in_frm_num-1)*mdl_frm_num + mdl_frm_num - 1);
//		printf("dis=%d step=%d dis/step=%d\r\n",dis,step,dis/step);
		}
	return (dis/step); //步长归一化
}


void get_mean(int16_t *frm_ftr1, int16_t *frm_ftr2, int16_t *mean)
{
	uint8_t	i;

	for (i = 0; i < mfcc_num; i++) {
		mean[i] = (frm_ftr1[i]+frm_ftr2[i])/2;
	//	printf("x=%d y=%d ", frm_ftr1[i], frm_ftr2[i]);
	//	printf("mean=%d\r\n", mean[i]);
	}
}

/*
 *	从两特征矢量获取特征模板
	参数
	ftr_in1	:输入特征值
	ftr_in2	:输入特征值
	ftr_mdl	:特征模版
	返回值
	dis		:累计匹配距离
*/

uint32_t get_mdl(v_ftr_tag *ftr_in1, v_ftr_tag *ftr_in2, v_ftr_tag *ftr_mdl)
{
	uint32_t dis;
	uint16_t x, y;
	uint16_t step;
	int16_t *in1;
	int16_t *in2;
	int16_t *mdl;
	uint32_t right, right_up, d_right_up;//up,
	uint32_t min;

	in_frm_num = ftr_in1->frm_num;
	mdl_frm_num = ftr_in2->frm_num;

	if ((in_frm_num > (mdl_frm_num*2)) || ((2*in_frm_num) < mdl_frm_num)) {
		// printf("in_frm_num= %d, mdl_frm_num= %d\n", in_frm_num, mdl_frm_num);
		return dis_err;
	} else {
		// 计算约束平行四边形顶点值
		X1 = (2*mdl_frm_num-in_frm_num)/3;
		X2 = (4*in_frm_num-2*mdl_frm_num)/3;
		in1 = ftr_in1->mfcc_dat;
		in2 = ftr_in2->mfcc_dat;
		mdl = ftr_mdl->mfcc_dat;

		dis = get_dis(in1, in2);
		get_mean(in1, in2, mdl);
		x = 1;
		y = 1;
		step = 1;
		do {
			//up = (dtw_limit(x, y+1) == ins)?get_dis(in2+mfcc_num, in1):dis_err;
			d_right_up = (dtw_limit(x+1, y+2) == ins)?get_dis(in2+mfcc_num+mfcc_num, in1+mfcc_num):dis_err;
			right = (dtw_limit(x+1, y) == ins)?get_dis(in2, in1+mfcc_num):dis_err;
			right_up = (dtw_limit(x+1, y+1) == ins)?get_dis(in2+mfcc_num, in1+mfcc_num):dis_err;

			min = right_up;
			if (min > right)
				min = right;

			if (min > d_right_up)
				min = d_right_up;

			dis += min;

			if (min == right_up) {
				in1 += mfcc_num;
				x++;
				in2 += mfcc_num;
				y++;
			} else if (min == d_right_up) {
				//in2 += mfcc_num;
				//y++;
				in2 = in2 + mfcc_num + mfcc_num;
				y += 2;
				in1 += mfcc_num;
				x++;
			} else {
				in1 += mfcc_num;
				x++;
			}
			step++;

			mdl += mfcc_num;
			get_mean(in1, in2, mdl);

		//	printf("x=%d y=%d\r\n", x, y);
		} while ((x < in_frm_num) && (y < mdl_frm_num));
		// printf("step=%d\r\n", step);
		ftr_mdl->frm_num = step;
	}
	return (dis/step); //步长归一化
}

