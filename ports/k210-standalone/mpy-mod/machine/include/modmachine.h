#ifndef MICROPY_INCLUDED_K210_MODMACHINE_H
#define MICROPY_INCLUDED_K210_MODMACHINE_H

#include "py/obj.h"

extern const mp_obj_type_t machine_uarths_type;
extern const mp_obj_type_t machine_uart_type;
extern const mp_obj_type_t machine_pin_type;
extern const mp_obj_type_t machine_pwm_type;
extern const mp_obj_type_t machine_timer_type;
extern const mp_obj_type_t machine_st7789_type;
extern const mp_obj_type_t machine_ov2640_type;
extern const mp_obj_type_t machine_burner_type;
extern const mp_obj_type_t machine_demo_face_detect_type;
extern const mp_obj_type_t machine_zmodem_type;
extern const mp_obj_type_t machine_spiflash_type;
extern const mp_obj_type_t machine_fpioa_type;
extern const mp_obj_type_t machine_ws2812_type;
extern const mp_obj_type_t machine_test_type;
extern const mp_obj_type_t machine_devmem_type;
extern const mp_obj_type_t machine_esp8285_type;
void machine_pins_init(void);
void machine_pins_deinit(void);

#endif // MICROPY_INCLUDED_K210_MODMACHINE_H
