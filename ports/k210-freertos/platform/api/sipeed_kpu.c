#include "sipeed_kpu.h"
#include "../drivers/include/w25qxx.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "py/mpprint.h"

char* layer_type_name0[]={
	"KL_INVALID",
	"KL_ADD",
	"KL_QUANTIZED_ADD",
	"KL_GLOBAL_MAX_POOL2D",
	"KL_QUANTIZED_GLOBAL_MAX_POOL2D",
	"KL_GLOBAL_AVERAGE_POOL2D",
	"KL_QUANTIZED_GLOBAL_AVERAGE_POOL2D",
	"KL_MAX_POOL2D",
	"KL_QUANTIZED_MAX_POOL2D",
	"KL_AVERAGE_POOL2D",
	"KL_QUANTIZED_AVERAGE_POOL2D",
	"KL_QUANTIZE",
	"KL_DEQUANTIZE",
	"KL_REQUANTIZE",
	"KL_L2_NORMALIZATION",
	"KL_SOFTMAX",
	"KL_CONCAT",
	"KL_QUANTIZED_CONCAT",
	"KL_FULLY_CONNECTED",
	"KL_QUANTIZED_FULLY_CONNECTED",
	"KL_TENSORFLOW_FLATTEN",
	"KL_QUANTIZED_TENSORFLOW_FLATTEN"
};
char* layer_type_name1[]={
	"KL_K210_CONV",
	"KL_K210_ADD_PADDING",
	"KL_K210_REMOVE_PADDING",
	"KL_K210_UPLOAD"
};
/*
upcase letter: unsigned
lowcase letter: signed
case BYTEARRAY_TYPECODE:
case 'B': case 'B':
	align = size = 1; break;
case 'h': case 'H':
	align = alignof(short);
	size = sizeof(short); break;
case 'i': case 'I':
	align = alignof(int);
	size = sizeof(int); break;
case 'l': case 'L':
	align = alignof(long);
	size = sizeof(long); break;
case 'q': case 'Q':
	align = alignof(long long);
	size = sizeof(long long); break;
case 'P': case 'O': case 'S':
	align = alignof(void*);
	size = sizeof(void*); break;
case 'f':
	align = alignof(float);
	size = sizeof(float); break;
case 'd':
	align = alignof(double);
	size = sizeof(double); break;
*/

char layer_type_dtype0[]={
	0,   	//"KL_INVALID",
	'f', 	//"KL_ADD",
	'B', 	//"KL_QUANTIZED_ADD",
	0,   	//"KL_GLOBAL_MAX_POOL2D",
	0, 		//"KL_QUANTIZED_GLOBAL_MAX_POOL2D",
	'f',	//"KL_GLOBAL_AVERAGE_POOL2D",
	0,		//"KL_QUANTIZED_GLOBAL_AVERAGE_POOL2D",
	0,		//"KL_MAX_POOL2D",
	'B',	//"KL_QUANTIZED_MAX_POOL2D",
	'f',	//"KL_AVERAGE_POOL2D",
	'B',	//"KL_QUANTIZED_AVERAGE_POOL2D",
	'B',	//"KL_QUANTIZE",
	'f',	//"KL_DEQUANTIZE",
	'B',	//"KL_REQUANTIZE",
	'f',	//"KL_L2_NORMALIZATION",
	'f',	//"KL_SOFTMAX",
	'f',	//"KL_CONCAT",
	'B',	//"KL_QUANTIZED_CONCAT",
	'f',	//"KL_FULLY_CONNECTED",
	'B',	//"KL_QUANTIZED_FULLY_CONNECTED",
	'f',	//"KL_TENSORFLOW_FLATTEN",
	'B',	//"KL_QUANTIZED_TENSORFLOW_FLATTEN"
};
char layer_type_dtype1[]={
	'B',	//"KL_K210_CONV",
	'B',	//"KL_K210_ADD_PADDING",
	'B',	//"KL_K210_REMOVE_PADDING",
	'B',	//"KL_K210_UPLOAD"
};


int kpu_model_flash_get_size(uint32_t model_addr)
{
	int err = 0;
	//Read head first
	kpu_kmodel_header_t model_header;
	w25qxx_status_t status = w25qxx_read_data_dma(model_addr, &model_header, sizeof(kpu_kmodel_header_t), W25QXX_QUAD_FAST);
	if(status != W25QXX_OK)
	{
		mp_printf(&mp_plat_print, "err: read head err!");
		err = -2;//read error
		goto error;
	}

	mp_printf(&mp_plat_print, "v=%d, flag=%d, arch=%d, layer len=%d, mem=%d, out cnt=%d\r\n", \
		model_header.version, model_header.flags, model_header.arch, model_header.layers_length, model_header.main_mem_usage, model_header.output_count);
    if (model_header.version != 3)	//we only support V3 now
	{
		mp_printf(&mp_plat_print, "err: we only support V3 now, get V%d\r\n", model_header.version);
		err = -3;//read error
        goto error;
	}
	//Get layers
	uint32_t output_count = model_header.output_count;
    uint32_t layers_length = model_header.layers_length;
	uint32_t layer_start = model_addr + sizeof(kpu_kmodel_header_t) + \
						output_count*sizeof(kpu_model_output_t);
	int i;
	uint32_t current_layer = layer_start;
	uint32_t sum_size = 0;
	kpu_model_layer_header_t layer_header;
	for(i=0; i < layers_length; i++)
	{
		
		w25qxx_status_t status = w25qxx_read_data_dma(current_layer, &layer_header, sizeof(kpu_model_layer_header_t), W25QXX_QUAD_FAST);
		if(status != W25QXX_OK)
		{
			err = -4;//read error
			goto error;
		}
		
		if(layer_header.body_size <=0 )
		{
			err = -5;//read value error
			goto error;
		}
		
		sum_size += layer_header.body_size;
		current_layer += sizeof(kpu_model_layer_header_t);
	}
						
	//mp_printf(&mp_plat_print, "kmodel size: %d bytes\r\n", current_layer - model_addr + sum_size);			
    return current_layer - model_addr + sum_size;
	
error:
    mp_printf(&mp_plat_print, "[MAIXPY]kpu: kpu_model_get_size  error %d", err);
    return -1;
}

char* kpu_model_getname_from_type(kpu_model_layer_type_t type)
{
	if(type <0 || (type > KL_QUANTIZED_TENSORFLOW_FLATTEN && type < KL_K210_CONV) || type > KL_K210_UPLOAD)
		return layer_type_name0[0];	//invalid
	if(type < KL_K210_CONV)
		return layer_type_name0[type];
	else
		return layer_type_name1[type - KL_K210_CONV];
}

char* kpu_model_getdtype_from_type(kpu_model_layer_type_t type)
{
	if(type <0 || (type > KL_QUANTIZED_TENSORFLOW_FLATTEN && type < KL_K210_CONV) || type > KL_K210_UPLOAD)
		return 0;	//invalid
	if(type < KL_K210_CONV)
		return layer_type_dtype0[type];
	else
		return layer_type_dtype1[type - KL_K210_CONV];
}

kpu_model_layer_type_t kpu_model_get_layer_type(kpu_model_context_t *ctx, uint32_t index)
{
	uint8_t* model_data = ctx->model_buffer;
	if(index >= ctx->layers_length)
	{
		mp_printf(&mp_plat_print, "err: index > layers_length!\r\n");
		return -1;
	}
	kpu_model_layer_header_t* layer_header=ctx->layer_headers + index;
	return layer_header->type;
}

char* kpu_model_get_layer_type_string(kpu_model_context_t *ctx, uint32_t index)
{
	kpu_model_layer_type_t type  = kpu_model_get_layer_type(ctx, index);
	return kpu_model_getname_from_type(type);
}

int kpu_model_get_layer_size(kpu_model_context_t *ctx, uint32_t index)
{
	uint8_t* model_data = ctx->model_buffer;
	if(index >= ctx->layers_length)
	{
		mp_printf(&mp_plat_print, "err: index > layers_length!\r\n");
		return -1;
	}
	kpu_model_layer_header_t* layer_header=ctx->layer_headers + index;
	return layer_header->body_size;
}



int kpu_model_print_layer_info(kpu_model_context_t *ctx)
{
	uint8_t* model_data = ctx->model_buffer;
	int i;
	kpu_model_layer_header_t* layer_header=ctx->layer_headers;// + index;
	for(i=0;i<ctx->layers_length;i++)
	{
		mp_printf(&mp_plat_print, "layer[%d]: %s, %d bytes\r\n", i, kpu_model_getname_from_type(layer_header->type), layer_header->body_size);
		layer_header++;
	}
	return;
}

uint8_t* kpu_model_get_layer_body(kpu_model_context_t *ctx, uint32_t index)
{
	uint8_t* model_data = ctx->model_buffer;
	int i;
	if(index >= ctx->layers_length)
	{
		mp_printf(&mp_plat_print, "err: index > layers_length!\r\n");
		return -1;
	}
	kpu_model_layer_header_t* layer_header=ctx->layer_headers;// + sizeof(kpu_model_layer_header_t) * index;
	uint8_t* addr = ctx->body_start;
	for(i=0; i<index; i++)
	{
		addr += layer_header->body_size;
		layer_header++;
	}
	
	return addr;
}

/*
typedef struct
{
    uint32_t flags;
    uint32_t main_mem_out_address;
    uint32_t layer_offset;
    uint32_t weights_offset;
    uint32_t bn_offset;
    uint32_t act_offset;
} kpu_model_conv_layer_argument_t;
*/
kpu_layer_argument_t* kpu_model_get_conv_layer(kpu_model_context_t *ctx, uint32_t index)
{
	kpu_model_layer_type_t type = kpu_model_get_layer_type(ctx, index);
	if(type != KL_K210_CONV)
	{
		//mp_printf(&mp_plat_print, "err: layer isn't conv layer! type = %d\r\n", type);
		return 0;
	}
	uint8_t* layer_body = kpu_model_get_layer_body(ctx, index);
	return (const volatile kpu_layer_argument_t *)(ctx->model_buffer + ((const kpu_model_conv_layer_argument_t *)layer_body)->layer_offset);
}

kpu_layer_argument_t* kpu_model_get_last_conv_layer(kpu_model_context_t *ctx)
{
	uint8_t* model_data = ctx->model_buffer;
	int i, index;
	kpu_model_layer_header_t* layer_header=ctx->layer_headers;// + index;
	kpu_layer_argument_t* layer = NULL;
	index = 0;
	for(i=0;i<ctx->layers_length;i++)
	{
		if(layer_header->type == KL_K210_CONV) index = i;
		layer_header++;
	}
	return kpu_model_get_conv_layer(ctx, index);
}

static int flag_index = -1;
static void _recover_conv_output_flag(kpu_model_context_t *ctx)
{
	uint8_t* model_data = ctx->model_buffer;
	kpu_model_layer_header_t* layer_header=ctx->layer_headers;// + index;
	kpu_layer_argument_t* layer = NULL;
	if(flag_index < 0) return;
	layer_header += flag_index;
	if(layer_header->type == KL_K210_CONV)
	{
		kpu_model_conv_layer_argument_t* layer_body = kpu_model_get_layer_body(ctx, flag_index);
		layer_body->flags &= ~(KLF_MAIN_MEM_OUT);	//clear output flag
		flag_index = -1;
	}
	return;
}

int kpu_model_get_input_shape(kpu_model_context_t *ctx, uint16_t* w, uint16_t* h, uint16_t* ch)
{
	kpu_layer_argument_t* first_layer = kpu_model_get_conv_layer(ctx, 0);	//first layer must conv layer
	if(first_layer == NULL)
	{
		return -1;
	}
	*w = first_layer->image_size.data.i_row_wid+1;
	*h = first_layer->image_size.data.i_col_high+1;
	*ch = first_layer->image_channel_num.data.i_ch_num+1;
	return 0;
}

int kpu_model_get_output_shape(kpu_model_context_t *ctx, uint16_t* w, uint16_t* h, uint16_t* ch)
{
	kpu_layer_argument_t* last_layer = kpu_model_get_last_conv_layer(ctx);	
	if(last_layer == NULL)
	{
		return -1;
	}
	*w = last_layer->image_size.data.o_row_wid+1;
	*h = last_layer->image_size.data.o_col_high+1;
	*ch = last_layer->image_channel_num.data.o_ch_num+1;
	return 0;
}


int kpu_model_set_output(kpu_model_context_t *ctx, uint32_t index, uint32_t layers_length)
{
	if (index >= ctx->output_count)
	{
		mp_printf(&mp_plat_print, "output index too big!\r\n");
        return -1;
	}
	ctx->layers_length = layers_length;
	kpu_model_layer_type_t layer_type = kpu_model_get_layer_type(ctx, layers_length-1);
	uint8_t* layer_body = kpu_model_get_layer_body(ctx, layers_length-1);
	kpu_model_output_t * output = ctx->outputs + index;
	_recover_conv_output_flag(ctx);	//clear output flag
    switch (layer_type)
    {
		case KL_ADD:
		{
			kpu_model_add_layer_argument_t* body = (kpu_model_add_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = body->count;
			break;
		}
		case KL_QUANTIZED_ADD:
		{
			kpu_model_quant_add_layer_argument_t* body = (kpu_model_quant_add_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = body->count;
			break;
		}
		case KL_GLOBAL_AVERAGE_POOL2D:
		{
			kpu_model_gap2d_layer_argument_t* body = (kpu_model_gap2d_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = (body->channels)*sizeof(float);
			break;
		}
		case KL_QUANTIZED_MAX_POOL2D:
		{
			kpu_model_quant_max_pool2d_layer_argument_t* body = (kpu_model_quant_max_pool2d_layer_argument_t*)layer_body;
			kpu_model_shape_t out_shape = body->out_shape;
			output->address = body->main_mem_out_address;
			output->size = (out_shape.width)*(out_shape.height)* (out_shape.channels);
			break;
		}
		case KL_AVERAGE_POOL2D:
		{
			kpu_model_ave_pool2d_layer_argument_t* body = (kpu_model_ave_pool2d_layer_argument_t*)layer_body;
			kpu_model_shape_t out_shape = body->out_shape;
			output->address = body->main_mem_out_address;
			output->size = (out_shape.width)*(out_shape.height)* (out_shape.channels)*sizeof(float);
			break;
		}
		case KL_QUANTIZE:
		{
			kpu_model_quantize_layer_argument_t* body = (kpu_model_quantize_layer_argument_t*)layer_body;
			output->address = body->mem_out_address;
			output->size = body->count;
			break;
		}
		case KL_DEQUANTIZE:
		{
			kpu_model_dequantize_layer_argument_t* body = (kpu_model_dequantize_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = (body->count)*sizeof(float);
			break;
		}
		case KL_REQUANTIZE:
		{
			kpu_model_requantize_layer_argument_t* body = (kpu_model_requantize_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = body->count;
			break;
		}
		case KL_L2_NORMALIZATION:
		{
			kpu_model_l2_norm_layer_argument_t* body = (kpu_model_l2_norm_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = body->channels * sizeof(float);
			break;
		}
		case KL_SOFTMAX:
		{
			kpu_model_softmax_layer_argument_t* body = (kpu_model_softmax_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = body->channels * sizeof(float);
			break;
		}
		case KL_CONCAT:
		case KL_QUANTIZED_CONCAT:
		{
			kpu_model_concat_layer_argument_t* body = (kpu_model_concat_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = (body->input_count) * (body->inputs_mem[0].size);
			break;
		}
		case KL_FULLY_CONNECTED:
		{
			kpu_model_fully_connected_layer_argument_t* body = (kpu_model_fully_connected_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = body->out_channels * sizeof(float);
			break;
		}
		case KL_TENSORFLOW_FLATTEN:
		{
			kpu_model_tf_flatten_layer_argument_t* body = (kpu_model_tf_flatten_layer_argument_t*)layer_body;
			kpu_model_shape_t out_shape = body->shape;
			output->address = body->main_mem_out_address;
			output->size = (out_shape.width)*(out_shape.height)* (out_shape.channels)*sizeof(float);;
			break;
		}
		case KL_K210_CONV:
		{
			kpu_model_conv_layer_argument_t* body = (kpu_model_conv_layer_argument_t*)layer_body;
			volatile kpu_layer_argument_t layer = *(const volatile kpu_layer_argument_t *)(ctx->model_buffer + body->layer_offset);
			//mp_printf(&mp_plat_print, "flags=%d, len=%d\r\n", body->flags , layers_length-1);
			if(!(body->flags))
			{
				body->flags |= KLF_MAIN_MEM_OUT;	//enable output
				flag_index = layers_length-1;
				//mp_printf(&mp_plat_print, "set flag 0x%lx, flags=%d\r\n", (long unsigned int)body, body->flags);
			}
			
			output->address = body->main_mem_out_address;
			output->size = layer.dma_parameter.data.dma_total_byte;
			break;
		}
		case KL_K210_ADD_PADDING:
			mp_printf(&mp_plat_print, "KL_K210_ADD_PADDING not support output to mainbuf!\r\n");
			return -1;
			break;
		case KL_K210_REMOVE_PADDING:
		{
			kpu_model_remove_padding_layer_argument_t* body = (kpu_model_remove_padding_layer_argument_t*)layer_body;
			output->address = body->main_mem_out_address;
			output->size = body->channels;
			break;
		}
		case KL_K210_UPLOAD:
			mp_printf(&mp_plat_print, "KL_K210_UPLOAD not support output to mainbuf!\r\n");
			return -1;
			break;
		default:
			mp_printf(&mp_plat_print, "Unknow layer type: %d!\r\n", layer_type);
			return -1;
    }
	return 0;
}



