#include "sipeed_conv.h"
#define _P(...) //mp_printf(&mp_plat_print, __VA_ARGS__)
//激活函数折点表，设置为y=x，即直接输出卷积结果
//y=(uint8_t)((((uint64_t)(x - x_start) * y_mul) >> shift) + bias);
 
kpu_activate_table_t active_addr __attribute__((aligned(256))) = {
 .activate_para = {  //x =36bit
  {.data = {.shift_number=0, .y_mul=0, .x_start=0x800000000 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }},
  {.data = {.shift_number=0, .y_mul=1, .x_start=0 }}
 },
.activate_para_bias0.data = {
  .result_bias = {0,0,0,0,0,0,0,0}
 },
 .activate_para_bias1.data = {
  .result_bias = {0,0,0,0,0,0,0,0}
 }
};

//y = (x*norm_mul)>>norm_shift + norm_add
kpu_batchnorm_argument_t bwsx_base_addr[] __attribute__((aligned(128))) = {
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
 {.batchnorm.data = {.norm_mul = 1, .norm_add = 0x0, .norm_shift = 0}},
};


//卷积参数
kpu_layer_argument_t la __attribute__((aligned(128)));
//max for 3in*3out, you can modify it
uint16_t conv_data_u16[9*3*3] __attribute__((aligned(128)));

//池化类型，0表示跳过
//0x1 代表步长为 2 的 2x2 max pooling,
//0x2 代表步长为 2 的 2x2 mean pooling,
//0x3 代表步长为 4 的 4x4 max pooling,
//0x4 代表步长为 4 的 4x4 mean pooling,
//0x5 代表步长为 2 的 2x2 left_top pooling,
//0x6 代表步长为 2 的 2x2 right_bottom pooling,
//0x7 代表步长为 4 的 4x4 left_top pooling,
//0x8 代表步长为 1 的 2x2 mean pooling,
//0x9 代表步长为 1 的 2x2 max pooling
#define AI_MEM_SIZE 0x200000

static float min(float* data, uint32_t len)
{
	int i;
	float m=data[0];
	for(i=0;i<len;i++)
	{
		if(data[i]<m) m = data[i];
	}
	return m;
}

static float max(float* data, uint32_t len)
{
	int i;
	float m=data[0];
	for(i=0;i<len;i++)
	{
		if(data[i]>m) m = data[i];
	}
	return m;
}

//global var: la, active_addr, bwsx_base_addr
static void conv_float2u16(float* data, uint16_t* data_u16, int len)
{
	float dmin, drange,arg_x;
	volatile float scale;
	uint16_t y_mul;
	int i, shift_number;
	dmin=min(data,len);
	drange=max(data,len)-dmin;
	scale = (65535.0/drange);

	//scale conv
	_P("convert conv parm: -------------\r\n");
	for(i=0;i<len;i++)
	{
		//float tmp =(float)((double)(data[i]-dmin)*scale);
		data_u16[i]=(uint16_t)((data[i]-dmin)*scale);
		_P("0x%04x\t",data_u16[i]);
		if(i%9==8) {_P("\r\n");}
	}
	//set arg_x & shr_x
	_P("set arg_x & shr_x: -------------\r\n");
	arg_x=scale*(dmin>=0?dmin:-dmin);
	for(i=0;(arg_x<(float)(0x400000)) && (arg_x!=0);i++)
	{
		arg_x*=2;
		//_P("argx=%f, shrx=%d\r\n", arg_x, i);
	}
	la.conv_value.data.arg_x = dmin>=0 ? (uint32_t)(arg_x) : (uint32_t)(0x1000000-(uint32_t)arg_x);
	la.conv_value.data.shr_x = i;
	_P("arg_x=0x%x, shr_x=%d\r\n",la.conv_value.data.arg_x, la.conv_value.data.shr_x);
	//set act table
	_P("set act table: -------------\r\n");
	_P("origin scale=%f\r\n",scale);
	scale=1.0/scale;
	for(i=0;scale<=16383.0;i++)
	{
		scale=scale*2;
	}
	shift_number=i;
	y_mul=(uint16_t)(scale);
	_P("shift_number=%d, y_mul=%d\r\n", shift_number, y_mul);
	for(i=1;i<16;i++)
	{
		active_addr.activate_para[i].data.shift_number=shift_number;
		active_addr.activate_para[i].data.y_mul=y_mul;
		active_addr.activate_para[i].data.x_start=0;
	}
	return;
}

void sipeed_conv_init(kpu_task_t* task, uint16_t w, uint16_t h, uint8_t ch_in, uint8_t ch_out, float* conv_data) 
{
	conv_float2u16(conv_data, conv_data_u16, 9*ch_in*ch_out);	//3x3 kernel
	la.kernel_offset.data.coef_row_offset = 0;					//固定为0
	la.kernel_offset.data.coef_column_offset = 0;				//固定为0
	//激活函数配置-
	la.kernel_calc_type_cfg.data.load_act=1;					//使能激活函数
	la.kernel_calc_type_cfg.data.active_addr = (uint64_t)&active_addr;
																//初始化激活表
	//row_switch_addr = math.ceil(i_row_wid / 64)
	//channel_switch_addr = i_col_high * row_switch_addr	
	la.kernel_calc_type_cfg.data.row_switch_addr = (w+63)/64;	//图像宽度占用的单元数
	la.kernel_calc_type_cfg.data.channel_switch_addr = (w+63)/64*h; 	
	la.kernel_calc_type_cfg.data.coef_size = 0; 				//固定为0
	la.kernel_calc_type_cfg.data.coef_group = 1; 		
	//中断设置--
	la.interrupt_enabe.data.depth_wise_layer = 0; 				//常规卷积层
	la.interrupt_enabe.data.int_en = 1;							//使能中断
	la.interrupt_enabe.data.full_add = 0; 						//??
	la.interrupt_enabe.data.ram_flag = 1;						//??
	//dma设置，知道是输出数据使用的DMA--
	la.dma_parameter.data.dma_total_byte = w*h*ch_out-1;		//总共的DMA传输数量	
	la.dma_parameter.data.send_data_out = 1;					//使能数据的dma输出
	la.dma_parameter.data.channel_byte_num = w*h-1;				//单通道的DMA传输数量
	//卷积运算参数设置--
	// arg_x 为24bit,shr_x 为4bit, 在conv_float2u16中设置
	/*	
	la.conv_value.data.arg_x = 0;
	la.conv_value.data.shr_x = 0;			
	la.conv_value.data.arg_w = 0;
	la.conv_value.data.shr_w = 0;
	la.conv_value2.data.arg_add = 0;
	*/
	//写回设置--
	la.write_back_cfg.data.wb_row_switch_addr = (w+63)/64; 		//ceil(16/64)=1
	la.write_back_cfg.data.wb_channel_switch_addr = (w+63)/64*h;			//16*1
	la.write_back_cfg.data.wb_group = 1;	//64/w
	//图像尺寸设置--
	la.image_size.data.i_row_wid = w-1;							//输入长宽
	la.image_size.data.i_col_high = h-1;
	la.image_size.data.o_row_wid = w-1;							//输出长宽
	la.image_size.data.o_col_high = h-1;
	//池化类型设置-
	la.kernel_pool_type_cfg.data.bypass_conv = 0;				//不略过卷积
	la.kernel_pool_type_cfg.data.pad_value = 0x0;				//边界填充0
	la.kernel_pool_type_cfg.data.load_para = 1;					//允许归一化
	la.kernel_pool_type_cfg.data.pad_type = 0;					//使用填充值
	la.kernel_pool_type_cfg.data.kernel_type = 1;				//3x3
	la.kernel_pool_type_cfg.data.pool_type = 0;					//池化类型，跳过
	la.kernel_pool_type_cfg.data.dma_burst_size = 15;			//dma突发传送大小，16字节
	la.kernel_pool_type_cfg.data.bwsx_base_addr = (uint64_t)&bwsx_base_addr;	
																//批归一化首地址
	la.kernel_pool_type_cfg.data.first_stride = h<256?0:1;		//图像高度未超过255
	//图像通道设置--
	la.image_channel_num.data.o_ch_num_coef = ch_out-1;			//一次性参数加载可计算的通道数
	la.image_channel_num.data.i_ch_num = ch_in-1;					//输入通道
	la.image_channel_num.data.o_ch_num = ch_out-1;				//输出通道
	//卷积参数设置-
	la.kernel_load_cfg.data.load_time = 0;						//卷积加载次数，不超过72KB，只加载一次
	la.kernel_load_cfg.data.para_size = 2*9*ch_in*ch_out;		//卷积参数大小
	la.kernel_load_cfg.data.para_start_addr = (uint64_t)conv_data_u16;
																//起始地址
	la.kernel_load_cfg.data.load_coor = 1;						//允许加载卷积参数
	//计算地址设置--
	la.image_addr.data.image_src_addr=(uint64_t)0x0;			//一个为0
	la.image_addr.data.image_dst_addr=(uint64_t)(AI_MEM_SIZE/64-(w+63)/64*h*ch_out);	

	/* init kpu task*/
	task->layers = &la;
	task->layers_length = 1;    								//单层
	task->eight_bit_mode = 0;   								//16bit模式
	task->output_scale = 1.0;   								//输出的缩放
	task->output_bias = 0;										//输出的偏置
}

void sipeed_conv_run(kpu_task_t* task, uint8_t* img_src, uint8_t* img_dst, plic_irq_callback_t callback)
{
	/* start to calculate */
	kpu_run(task, DMAC_CHANNEL5, img_src, img_dst, callback);
}

