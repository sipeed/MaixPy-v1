#include "maixpy.h"
#include "sdcard.h"
void * __dso_handle = 0 ;
extern sdcard_config_t config;

int main()
{
    sdcard_config_t amigo = { 10, 6, 11, 26, SD_CS_PIN };
    config = amigo;
    maixpy_main();
    return 0;
}
