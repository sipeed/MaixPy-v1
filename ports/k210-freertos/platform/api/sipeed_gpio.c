#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <fpioa.h>
/*
handle_t gio;

void test_gpio(void)
{
    fpioa_set_function(12,FUNC_GPIO0);
    fpioa_set_function(13,FUNC_GPIO1);
    fpioa_set_function(14,FUNC_GPIO2);

    gio = io_open("/dev/gpio1");//gpio
    configASSERT(gio);

    gpio_set_drive_mode(gio, 0, GPIO_DM_OUTPUT);
    gpio_set_pin_value(gio, 0, GPIO_PV_LOW);

    gpio_set_drive_mode(gio, 1, GPIO_DM_OUTPUT);
    gpio_set_pin_value(gio, 1, GPIO_PV_LOW);

    gpio_set_drive_mode(gio, 2, GPIO_DM_OUTPUT);
    gpio_set_pin_value(gio, 2, GPIO_PV_LOW);

}
*/
