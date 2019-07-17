
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "py/objtype.h"
#include "py/objstr.h"
#include "py/objint.h"
#include "py/mperrno.h"
#include "fpioa.h"
#include "fpioa_des.h"
	
/*Please don't modify this macro*/
#define DES_SPACE_NUM(str) (sizeof("                                   ")-sizeof("   "))-strlen(str)
#define FUN_SPACE_NUM(str) (sizeof("	                  ")-sizeof("  "))-strlen(str)

typedef struct _Maix_fpioa_obj_t {
    mp_obj_base_t base;
}Maix_fpioa_obj_t;

const mp_obj_type_t Maix_fpioa_type;


STATIC mp_obj_t Maix_set_function(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	enum {
		ARG_pin,
		ARG_func,
	};
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_pin, 	MP_ARG_INT, {.u_int = 0} },
		{ MP_QSTR_func,	MP_ARG_INT, {.u_int = 0} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	uint16_t pin_num = args[ARG_pin].u_int;
	fpioa_function_t func_num = args[ARG_func].u_int;
	
	if(pin_num > FPIOA_NUM_IO)		
		mp_raise_ValueError("Don't have this Pin");
	
	if(func_num < 0 || func_num > USABLE_FUNC_NUM)
		mp_raise_ValueError("This function is invalid");
	
	if(0 != fpioa_set_function(pin_num,func_num))
	{
		mp_printf(&mp_plat_print, "[Maix]:Opps!Can not set fpioa\n");
		mp_raise_OSError(MP_EIO);
	}
    return mp_const_true;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(Maix_set_function_obj, 0,Maix_set_function);

STATIC mp_obj_t Maix_get_Pin_num(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	enum {
		ARG_func,
	};
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_func,	MP_ARG_INT, {.u_int = 0} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	
	fpioa_function_t fun_num = args[ARG_func].u_int;
	
	if(fun_num < 0 || fun_num > USABLE_FUNC_NUM)
		mp_raise_ValueError("This function is invalid");
	
	int Pin_num = fpioa_get_io_by_function(fun_num);
	
	if(-1 == Pin_num)
	{
		return mp_const_none;
	}
    return MP_OBJ_NEW_SMALL_INT(Pin_num);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(Maix_get_Pin_num_obj, 0,Maix_get_Pin_num);


STATIC mp_obj_t Maix_fpioa_help(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
	enum {
		ARG_func,
	};
	static const mp_arg_t allowed_args[] = {
		{ MP_QSTR_func,	 MP_ARG_INT, {.u_int = USABLE_FUNC_NUM} },
	};
	mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];  
	mp_arg_parse_all(n_args-1, pos_args+1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
	char* des_space_str = NULL;
	char* fun_space_str = NULL;
	if(args[ARG_func].u_int > USABLE_FUNC_NUM)
	{
		mp_printf(&mp_plat_print, "No this funciton Description\n");
		return mp_const_false;
	}
	mp_printf(&mp_plat_print, "+-------------------+----------------------------------+\n") ;
	mp_printf(&mp_plat_print, "|     Function      |            Description           |\n") ;
	mp_printf(&mp_plat_print, "+-------------------+----------------------------------+\n") ;
	if(args[ARG_func].u_int == USABLE_FUNC_NUM)
	{
		
		for(int i = 0;i < USABLE_FUNC_NUM ; i++)
		{    
			/*malloc memory*/
			des_space_str = (char*)malloc(DES_SPACE_NUM(func_description[i])+1);
			fun_space_str = (char*)malloc(FUN_SPACE_NUM( func_name[i])+1);
			
			memset(des_space_str,' ',DES_SPACE_NUM(func_description[i]));
			des_space_str[DES_SPACE_NUM(func_description[i])] = '\0';
			
			memset(fun_space_str,' ',FUN_SPACE_NUM( func_name[i]));
			fun_space_str[FUN_SPACE_NUM( func_name[i])] = '\0';
			
		
			mp_printf(&mp_plat_print, "|  %s%s|  %s%s|\n", func_name[i],fun_space_str,func_description[i],des_space_str) ;
			free(des_space_str);
			free(fun_space_str);
			mp_printf(&mp_plat_print, "+-------------------+----------------------------------+\n") ;
		}
	}
	else
	{	
		des_space_str = (char*)malloc(DES_SPACE_NUM(func_description[args[ARG_func].u_int])+1);
		fun_space_str = (char*)malloc(FUN_SPACE_NUM(func_name[args[ARG_func].u_int])+1);
		
		memset(des_space_str,' ',DES_SPACE_NUM(func_description[args[ARG_func].u_int]));
		des_space_str[DES_SPACE_NUM(func_description[args[ARG_func].u_int])] = '\0';
		
		memset(fun_space_str,' ',FUN_SPACE_NUM(func_name[args[ARG_func].u_int]));
		fun_space_str[FUN_SPACE_NUM(func_name[args[ARG_func].u_int])] = '\0';
		
		mp_printf(&mp_plat_print, "|  %s%s|  %s%s|\n", func_name[args[ARG_func].u_int],fun_space_str,func_description[args[ARG_func].u_int],des_space_str) ;
		free(des_space_str);
		free(fun_space_str);
		mp_printf(&mp_plat_print, "+-------------------+----------------------------------+\n") ;
	}
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(Maix_fpioa_help_obj, 0,Maix_fpioa_help);

STATIC mp_obj_t Maix_fpioa_make_new() {
    
    Maix_fpioa_obj_t *self = m_new_obj(Maix_fpioa_obj_t);
    self->base.type = &Maix_fpioa_type;

    return self;
}

STATIC const mp_rom_map_elem_t Maix_fpioa_locals_dict_table[] = {
    // fpioa methods
    { MP_ROM_QSTR(MP_QSTR_set_function), MP_ROM_PTR(&Maix_set_function_obj) },
    { MP_ROM_QSTR(MP_QSTR_help), MP_ROM_PTR(&Maix_fpioa_help_obj) },
	{ MP_ROM_QSTR(MP_QSTR_get_Pin_num), MP_ROM_PTR(&Maix_get_Pin_num_obj) },
	{MP_ROM_QSTR(MP_QSTR_JTAG_TCLK	   ), MP_ROM_INT(0	)},
	{MP_ROM_QSTR(MP_QSTR_JTAG_TDI	   ), MP_ROM_INT(1	)},
	{MP_ROM_QSTR(MP_QSTR_JTAG_TMS	   ), MP_ROM_INT(2	)},
	{MP_ROM_QSTR(MP_QSTR_JTAG_TDO	   ), MP_ROM_INT(3	)},
	{MP_ROM_QSTR(MP_QSTR_SPI0_D0	   ), MP_ROM_INT(4	)},
	{MP_ROM_QSTR(MP_QSTR_SPI0_D1	   ), MP_ROM_INT(5	)},
	{MP_ROM_QSTR(MP_QSTR_SPI0_D2	   ), MP_ROM_INT(6	)},
	{MP_ROM_QSTR(MP_QSTR_SPI0_D3	   ), MP_ROM_INT(7	)},
	{MP_ROM_QSTR(MP_QSTR_SPI0_D4	   ), MP_ROM_INT(8	)},
	{MP_ROM_QSTR(MP_QSTR_SPI0_D5	   ), MP_ROM_INT(9	)},
	{MP_ROM_QSTR(MP_QSTR_SPI0_D6	   ), MP_ROM_INT(10 )},
	{MP_ROM_QSTR(MP_QSTR_SPI0_D7	   ), MP_ROM_INT(11 )},
	{MP_ROM_QSTR(MP_QSTR_SPI0_SS0	   ), MP_ROM_INT(12 )},
	{MP_ROM_QSTR(MP_QSTR_SPI0_SS1	   ), MP_ROM_INT(13 )},
	{MP_ROM_QSTR(MP_QSTR_SPI0_SS2	   ), MP_ROM_INT(14 )},
	{MP_ROM_QSTR(MP_QSTR_SPI0_SS3	   ), MP_ROM_INT(15 )},
	{MP_ROM_QSTR(MP_QSTR_SPI0_ARB	   ), MP_ROM_INT(16 )},
	{MP_ROM_QSTR(MP_QSTR_SPI0_SCLK	   ), MP_ROM_INT(17 )},
	{MP_ROM_QSTR(MP_QSTR_UARTHS_RX	   ), MP_ROM_INT(18 )},
	{MP_ROM_QSTR(MP_QSTR_UARTHS_TX	   ), MP_ROM_INT(19 )},
	{MP_ROM_QSTR(MP_QSTR_RESV6		   ), MP_ROM_INT(20 )},
	{MP_ROM_QSTR(MP_QSTR_RESV7		   ), MP_ROM_INT(21 )},
	{MP_ROM_QSTR(MP_QSTR_CLK_SPI1	   ), MP_ROM_INT(22 )},
	{MP_ROM_QSTR(MP_QSTR_CLK_I2C1	   ), MP_ROM_INT(23 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS0	   ), MP_ROM_INT(24 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS1	   ), MP_ROM_INT(25 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS2	   ), MP_ROM_INT(26 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS3	   ), MP_ROM_INT(27 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS4	   ), MP_ROM_INT(28 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS5	   ), MP_ROM_INT(29 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS6	   ), MP_ROM_INT(30 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS7	   ), MP_ROM_INT(31 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS8	   ), MP_ROM_INT(32 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS9	   ), MP_ROM_INT(33 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS10	   ), MP_ROM_INT(34 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS11	   ), MP_ROM_INT(35 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS12	   ), MP_ROM_INT(36 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS13	   ), MP_ROM_INT(37 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS14	   ), MP_ROM_INT(38 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS15	   ), MP_ROM_INT(39 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS16	   ), MP_ROM_INT(40 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS17	   ), MP_ROM_INT(41 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS18	   ), MP_ROM_INT(42 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS19	   ), MP_ROM_INT(43 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS20	   ), MP_ROM_INT(44 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS21	   ), MP_ROM_INT(45 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS22	   ), MP_ROM_INT(46 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS23	   ), MP_ROM_INT(47 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS24	   ), MP_ROM_INT(48 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS25	   ), MP_ROM_INT(49 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS26	   ), MP_ROM_INT(50 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS27	   ), MP_ROM_INT(51 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS28	   ), MP_ROM_INT(52 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS29	   ), MP_ROM_INT(53 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS30	   ), MP_ROM_INT(54 )},
	{MP_ROM_QSTR(MP_QSTR_GPIOHS31	   ), MP_ROM_INT(55 )},
	{MP_ROM_QSTR(MP_QSTR_GPIO0		   ), MP_ROM_INT(56 )},
	{MP_ROM_QSTR(MP_QSTR_GPIO1		   ), MP_ROM_INT(57 )},
	{MP_ROM_QSTR(MP_QSTR_GPIO2		   ), MP_ROM_INT(58 )},
	{MP_ROM_QSTR(MP_QSTR_GPIO3		   ), MP_ROM_INT(59 )},
	{MP_ROM_QSTR(MP_QSTR_GPIO4		   ), MP_ROM_INT(60 )},
	{MP_ROM_QSTR(MP_QSTR_GPIO5		   ), MP_ROM_INT(61 )},
	{MP_ROM_QSTR(MP_QSTR_GPIO6		   ), MP_ROM_INT(62 )},
	{MP_ROM_QSTR(MP_QSTR_GPIO7		   ), MP_ROM_INT(63 )},
	{MP_ROM_QSTR(MP_QSTR_UART1_RX	   ), MP_ROM_INT(64 )},
	{MP_ROM_QSTR(MP_QSTR_UART1_TX	   ), MP_ROM_INT(65 )},
	{MP_ROM_QSTR(MP_QSTR_UART2_RX	   ), MP_ROM_INT(66 )},
	{MP_ROM_QSTR(MP_QSTR_UART2_TX	   ), MP_ROM_INT(67 )},
	{MP_ROM_QSTR(MP_QSTR_UART3_RX	   ), MP_ROM_INT(68 )},
	{MP_ROM_QSTR(MP_QSTR_UART3_TX	   ), MP_ROM_INT(69 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_D0	   ), MP_ROM_INT(70 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_D1	   ), MP_ROM_INT(71 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_D2	   ), MP_ROM_INT(72 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_D3	   ), MP_ROM_INT(73 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_D4	   ), MP_ROM_INT(74 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_D5	   ), MP_ROM_INT(75 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_D6	   ), MP_ROM_INT(76 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_D7	   ), MP_ROM_INT(77 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_SS0	   ), MP_ROM_INT(78 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_SS1	   ), MP_ROM_INT(79 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_SS2	   ), MP_ROM_INT(80 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_SS3	   ), MP_ROM_INT(81 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_ARB	   ), MP_ROM_INT(82 )},
	{MP_ROM_QSTR(MP_QSTR_SPI1_SCLK	   ), MP_ROM_INT(83 )},
	{MP_ROM_QSTR(MP_QSTR_SPI_SLAVE_D0  ), MP_ROM_INT(84 )},
	{MP_ROM_QSTR(MP_QSTR_SPI_SLAVE_SS  ), MP_ROM_INT(85 )},
	{MP_ROM_QSTR(MP_QSTR_SPI_SLAVE_SCLK), MP_ROM_INT(86 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_MCLK	   ), MP_ROM_INT(87 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_SCLK	   ), MP_ROM_INT(88 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_WS	   ), MP_ROM_INT(89 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_IN_D0    ), MP_ROM_INT(90 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_IN_D1    ), MP_ROM_INT(91 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_IN_D2    ), MP_ROM_INT(92 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_IN_D3    ), MP_ROM_INT(93 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_OUT_D0   ), MP_ROM_INT(94 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_OUT_D1   ), MP_ROM_INT(95 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_OUT_D2   ), MP_ROM_INT(96 )},
	{MP_ROM_QSTR(MP_QSTR_I2S0_OUT_D3   ), MP_ROM_INT(97 )},
	{MP_ROM_QSTR(MP_QSTR_I2S1_MCLK	   ), MP_ROM_INT(98 )},
	{MP_ROM_QSTR(MP_QSTR_I2S1_SCLK	   ), MP_ROM_INT(99 )},
	{MP_ROM_QSTR(MP_QSTR_I2S1_WS	   ), MP_ROM_INT(100)},
	{MP_ROM_QSTR(MP_QSTR_I2S1_IN_D0    ), MP_ROM_INT(101)},
	{MP_ROM_QSTR(MP_QSTR_I2S1_IN_D1    ), MP_ROM_INT(102)},
	{MP_ROM_QSTR(MP_QSTR_I2S1_IN_D2    ), MP_ROM_INT(103)},
	{MP_ROM_QSTR(MP_QSTR_I2S1_IN_D3    ), MP_ROM_INT(104)},
	{MP_ROM_QSTR(MP_QSTR_I2S1_OUT_D0   ), MP_ROM_INT(105)},
	{MP_ROM_QSTR(MP_QSTR_I2S1_OUT_D1   ), MP_ROM_INT(106)},
	{MP_ROM_QSTR(MP_QSTR_I2S1_OUT_D2   ), MP_ROM_INT(107)},
	{MP_ROM_QSTR(MP_QSTR_I2S1_OUT_D3   ), MP_ROM_INT(108)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_MCLK	   ), MP_ROM_INT(109)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_SCLK	   ), MP_ROM_INT(110)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_WS	   ), MP_ROM_INT(111)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_IN_D0    ), MP_ROM_INT(112)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_IN_D1    ), MP_ROM_INT(113)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_IN_D2    ), MP_ROM_INT(114)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_IN_D3    ), MP_ROM_INT(115)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_OUT_D0   ), MP_ROM_INT(116)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_OUT_D1   ), MP_ROM_INT(117)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_OUT_D2   ), MP_ROM_INT(118)},
	{MP_ROM_QSTR(MP_QSTR_I2S2_OUT_D3   ), MP_ROM_INT(119)},
	{MP_ROM_QSTR(MP_QSTR_RESV0		   ), MP_ROM_INT(120)},
	{MP_ROM_QSTR(MP_QSTR_RESV1		   ), MP_ROM_INT(121)},
	{MP_ROM_QSTR(MP_QSTR_RESV2		   ), MP_ROM_INT(122)},
	{MP_ROM_QSTR(MP_QSTR_RESV3		   ), MP_ROM_INT(123)},
	{MP_ROM_QSTR(MP_QSTR_RESV4		   ), MP_ROM_INT(124)},
	{MP_ROM_QSTR(MP_QSTR_RESV5		   ), MP_ROM_INT(125)},
	{MP_ROM_QSTR(MP_QSTR_I2C0_SCLK	   ), MP_ROM_INT(126)},
	{MP_ROM_QSTR(MP_QSTR_I2C0_SDA	   ), MP_ROM_INT(127)},
	{MP_ROM_QSTR(MP_QSTR_I2C1_SCLK	   ), MP_ROM_INT(128)},
	{MP_ROM_QSTR(MP_QSTR_I2C1_SDA	   ), MP_ROM_INT(129)},
	{MP_ROM_QSTR(MP_QSTR_I2C2_SCLK	   ), MP_ROM_INT(130)},
	{MP_ROM_QSTR(MP_QSTR_I2C2_SDA	   ), MP_ROM_INT(131)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_XCLK	   ), MP_ROM_INT(132)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_RST	   ), MP_ROM_INT(133)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_PWDN	   ), MP_ROM_INT(134)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_VSYNC    ), MP_ROM_INT(135)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_HREF	   ), MP_ROM_INT(136)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_PCLK	   ), MP_ROM_INT(137)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_D0	   ), MP_ROM_INT(138)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_D1	   ), MP_ROM_INT(139)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_D2	   ), MP_ROM_INT(140)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_D3	   ), MP_ROM_INT(141)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_D4	   ), MP_ROM_INT(142)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_D5	   ), MP_ROM_INT(143)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_D6	   ), MP_ROM_INT(144)},
	{MP_ROM_QSTR(MP_QSTR_CMOS_D7	   ), MP_ROM_INT(145)},
	{MP_ROM_QSTR(MP_QSTR_SCCB_SCLK	   ), MP_ROM_INT(146)},
	{MP_ROM_QSTR(MP_QSTR_SCCB_SDA	   ), MP_ROM_INT(147)},
	{MP_ROM_QSTR(MP_QSTR_UART1_CTS	   ), MP_ROM_INT(148)},
	{MP_ROM_QSTR(MP_QSTR_UART1_DSR	   ), MP_ROM_INT(149)},
	{MP_ROM_QSTR(MP_QSTR_UART1_DCD	   ), MP_ROM_INT(150)},
	{MP_ROM_QSTR(MP_QSTR_UART1_RI	   ), MP_ROM_INT(151)},
	{MP_ROM_QSTR(MP_QSTR_UART1_SIR_IN  ), MP_ROM_INT(152)},
	{MP_ROM_QSTR(MP_QSTR_UART1_DTR	   ), MP_ROM_INT(153)},
	{MP_ROM_QSTR(MP_QSTR_UART1_RTS	   ), MP_ROM_INT(154)},
	{MP_ROM_QSTR(MP_QSTR_UART1_OUT2    ), MP_ROM_INT(155)},
	{MP_ROM_QSTR(MP_QSTR_UART1_OUT1    ), MP_ROM_INT(156)},
	{MP_ROM_QSTR(MP_QSTR_UART1_SIR_OUT ), MP_ROM_INT(157)},
	{MP_ROM_QSTR(MP_QSTR_UART1_BAUD    ), MP_ROM_INT(158)},
	{MP_ROM_QSTR(MP_QSTR_UART1_RE	   ), MP_ROM_INT(159)},
	{MP_ROM_QSTR(MP_QSTR_UART1_DE	   ), MP_ROM_INT(160)},
	{MP_ROM_QSTR(MP_QSTR_UART1_RS485_EN), MP_ROM_INT(161)},
	{MP_ROM_QSTR(MP_QSTR_UART2_CTS	   ), MP_ROM_INT(162)},
	{MP_ROM_QSTR(MP_QSTR_UART2_DSR	   ), MP_ROM_INT(163)},
	{MP_ROM_QSTR(MP_QSTR_UART2_DCD	   ), MP_ROM_INT(164)},
	{MP_ROM_QSTR(MP_QSTR_UART2_RI	   ), MP_ROM_INT(165)},
	{MP_ROM_QSTR(MP_QSTR_UART2_SIR_IN  ), MP_ROM_INT(166)},
	{MP_ROM_QSTR(MP_QSTR_UART2_DTR	   ), MP_ROM_INT(167)},
	{MP_ROM_QSTR(MP_QSTR_UART2_RTS	   ), MP_ROM_INT(168)},
	{MP_ROM_QSTR(MP_QSTR_UART2_OUT2    ), MP_ROM_INT(169)},
	{MP_ROM_QSTR(MP_QSTR_UART2_OUT1    ), MP_ROM_INT(170)},
	{MP_ROM_QSTR(MP_QSTR_UART2_SIR_OUT ), MP_ROM_INT(171)},
	{MP_ROM_QSTR(MP_QSTR_UART2_BAUD    ), MP_ROM_INT(172)},
	{MP_ROM_QSTR(MP_QSTR_UART2_RE	   ), MP_ROM_INT(173)},
	{MP_ROM_QSTR(MP_QSTR_UART2_DE	   ), MP_ROM_INT(174)},
	{MP_ROM_QSTR(MP_QSTR_UART2_RS485_EN), MP_ROM_INT(175)},
	{MP_ROM_QSTR(MP_QSTR_UART3_CTS	   ), MP_ROM_INT(176)},
	{MP_ROM_QSTR(MP_QSTR_UART3_DSR	   ), MP_ROM_INT(177)},
	{MP_ROM_QSTR(MP_QSTR_UART3_DCD	   ), MP_ROM_INT(178)},
	{MP_ROM_QSTR(MP_QSTR_UART3_RI	   ), MP_ROM_INT(179)},
	{MP_ROM_QSTR(MP_QSTR_UART3_SIR_IN  ), MP_ROM_INT(180)},
	{MP_ROM_QSTR(MP_QSTR_UART3_DTR	   ), MP_ROM_INT(181)},
	{MP_ROM_QSTR(MP_QSTR_UART3_RTS	   ), MP_ROM_INT(182)},
	{MP_ROM_QSTR(MP_QSTR_UART3_OUT2    ), MP_ROM_INT(183)},
	{MP_ROM_QSTR(MP_QSTR_UART3_OUT1    ), MP_ROM_INT(184)},
	{MP_ROM_QSTR(MP_QSTR_UART3_SIR_OUT ), MP_ROM_INT(185)},
	{MP_ROM_QSTR(MP_QSTR_UART3_BAUD    ), MP_ROM_INT(186)},
	{MP_ROM_QSTR(MP_QSTR_UART3_RE	   ), MP_ROM_INT(187)},
	{MP_ROM_QSTR(MP_QSTR_UART3_DE	   ), MP_ROM_INT(188)},
	{MP_ROM_QSTR(MP_QSTR_UART3_RS485_EN), MP_ROM_INT(189)},
	{MP_ROM_QSTR(MP_QSTR_TIMER0_TOGGLE1), MP_ROM_INT(190)},
	{MP_ROM_QSTR(MP_QSTR_TIMER0_TOGGLE2), MP_ROM_INT(191)},
	{MP_ROM_QSTR(MP_QSTR_TIMER0_TOGGLE3), MP_ROM_INT(192)},
	{MP_ROM_QSTR(MP_QSTR_TIMER0_TOGGLE4), MP_ROM_INT(193)},
	{MP_ROM_QSTR(MP_QSTR_TIMER1_TOGGLE1), MP_ROM_INT(194)},
	{MP_ROM_QSTR(MP_QSTR_TIMER1_TOGGLE2), MP_ROM_INT(195)},
	{MP_ROM_QSTR(MP_QSTR_TIMER1_TOGGLE3), MP_ROM_INT(196)},
	{MP_ROM_QSTR(MP_QSTR_TIMER1_TOGGLE4), MP_ROM_INT(197)},
	{MP_ROM_QSTR(MP_QSTR_TIMER2_TOGGLE1), MP_ROM_INT(198)},
	{MP_ROM_QSTR(MP_QSTR_TIMER2_TOGGLE2), MP_ROM_INT(199)},
	{MP_ROM_QSTR(MP_QSTR_TIMER2_TOGGLE3), MP_ROM_INT(200)},
	{MP_ROM_QSTR(MP_QSTR_TIMER2_TOGGLE4), MP_ROM_INT(201)},
	{MP_ROM_QSTR(MP_QSTR_CLK_SPI2	   ), MP_ROM_INT(202)},
	{MP_ROM_QSTR(MP_QSTR_CLK_I2C2	   ), MP_ROM_INT(203)},	
};
STATIC MP_DEFINE_CONST_DICT(Maix_fpioa_locals_dict, Maix_fpioa_locals_dict_table);	
const mp_obj_type_t Maix_fpioa_type = {
	{ &mp_type_type },
	.name = MP_QSTR_FPIOA,
	.make_new = Maix_fpioa_make_new,
	.locals_dict = (mp_obj_dict_t*)&Maix_fpioa_locals_dict,
};



