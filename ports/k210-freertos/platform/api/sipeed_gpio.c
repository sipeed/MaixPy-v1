#include <stdio.h>
#include <fpioa.h>
#include <gpio.h>
#include <gpio_common.h>
void test_gpio(void)
{
    fpioa_set_function(12,FUNC_GPIO0);
    fpioa_set_function(13,FUNC_GPIO1);
    fpioa_set_function(14,FUNC_GPIO2);

    gpio_init();

    gpio_set_drive_mode(0, GPIO_DM_OUTPUT);
    gpio_set_pin(0, GPIO_PV_LOW);

    gpio_set_drive_mode(1, GPIO_DM_OUTPUT);
    gpio_set_pin(1, GPIO_PV_LOW);

    gpio_set_drive_mode(2, GPIO_DM_OUTPUT);
    gpio_set_pin(2, GPIO_PV_LOW);

}

