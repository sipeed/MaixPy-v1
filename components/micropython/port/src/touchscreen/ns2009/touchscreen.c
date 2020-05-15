


#include "touchscreen.h"
#include "machine_i2c.h"
#include "errno.h"
#include "ns2009.h"
#include "tscal.h"
#if defined(CONFIG_MAIXPY_TOUCH_SCREEN_DRIVER_FT52XX)
#include "ft52xx.h"
#endif

static struct ts_ns2009_pdata_t *ts_ns2009_pdata;
i2c_device_number_t m_i2c_num = 0;
static bool is_init = false;
static uint8_t drives_type = 0;

int ns2009_hal_i2c_init_default();

int touchscreen_init( void* arg)
{
    int err;
    touchscreen_config_t* config = (touchscreen_config_t*)arg;
    if( config->i2c != NULL)
    {
        m_i2c_num = config->i2c->i2c;
    }
    else
    {
        ns2009_hal_i2c_init_default();
    }
#if defined(CONFIG_MAIXPY_TOUCH_SCREEN_DRIVER_FT52XX)
    switch (config->drives_type)
    {
    case TOUCHSCREEN_TYPE_NS2009:
        drives_type = TOUCHSCREEN_TYPE_NS2009;
        ts_ns2009_pdata = ts_ns2009_probe(config->calibration, &err);
        if (ts_ns2009_pdata == NULL || err != 0){
            return err;
        }
    case TOUCHSCREEN_TYPE_FT62XX:
        drives_type = TOUCHSCREEN_TYPE_FT62XX;
        ts_ns2009_pdata = ts_ft52xx_probe(&err);
        if (ts_ns2009_pdata == NULL || err != 0){
            return err;  
        }
    default:
        break;
    }
#else
    ts_ns2009_pdata = ts_ns2009_probe(config->calibration, &err);
    if (ts_ns2009_pdata == NULL || err!=0)
        return err;
#endif
    is_init = true;
    return 0;
}
bool touchscreen_is_init()
{
    return is_init;
}

int touchscreen_read( int* type, int* x, int* y)
{
#if defined(CONFIG_MAIXPY_TOUCH_SCREEN_DRIVER_FT52XX)
    switch (drives_type) {
    case TOUCHSCREEN_TYPE_NS2009:
        ts_ns2009_poll(ts_ns2009_pdata);
        break;
    case TOUCHSCREEN_TYPE_FT62XX:
        ts_ft52xx_poll(ts_ns2009_pdata);
        break;
    default:
        break;
    }
#else
    ts_ns2009_poll(ts_ns2009_pdata);
#endif /*CONFIG_MAIXPY_TOUCH_SCREEN_DRIVER_FT52XX*/

    *x = 0;
    *y = 0;
    switch (ts_ns2009_pdata->event->type)
    {
        case TOUCH_BEGIN:
            *x = ts_ns2009_pdata->event->x;
            *y = ts_ns2009_pdata->event->y;
            *type = TOUCHSCREEN_STATUS_PRESS;
            break;

        case TOUCH_MOVE:
            *x = ts_ns2009_pdata->event->x;
            *y = ts_ns2009_pdata->event->y;
            *type = TOUCHSCREEN_STATUS_MOVE;
            break;

        case TOUCH_END:
            *x= ts_ns2009_pdata->event->x;
            *y= ts_ns2009_pdata->event->y;
            *type = TOUCHSCREEN_STATUS_RELEASE;
            break;
        default:
            break;
    }
    return 0;
}

int touchscreen_deinit()
{
#if defined(CONFIG_MAIXPY_TOUCH_SCREEN_DRIVER_FT52XX)
    switch (drives_type) {
    case TOUCHSCREEN_TYPE_NS2009:
        ts_ns2009_remove(ts_ns2009_pdata);
        break;
    case TOUCHSCREEN_TYPE_FT62XX:
        ts_ft52xx_remove(ts_ns2009_pdata);
        break;
    default:
        break;
    }    
#else
    ts_ns2009_remove(ts_ns2009_pdata);
#endif
    is_init = false;
    return 0;
}

int touchscreen_calibrate(int w, int h, int* cal)
{
#if defined(CONFIG_MAIXPY_TOUCH_SCREEN_DRIVER_FT52XX)
    return 0;
#else
    return do_tscal(ts_ns2009_pdata, w, h, cal);
#endif
}

//////////// HAL ////////////

int ns2009_hal_i2c_init_default()
{
    m_i2c_num = 0; // default i2c 0
    fpioa_set_function(30, FUNC_I2C0_SCLK +  m_i2c_num* 2);
    fpioa_set_function(31, FUNC_I2C0_SDA + m_i2c_num * 2);
    maix_i2c_init(m_i2c_num, 7, 400000);
    return 0;
}

int ns2009_hal_i2c_recv(const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf,
                  size_t receive_buf_len)
{
    return maix_i2c_recv_data(m_i2c_num, NS2009_SLV_ADDR, send_buf, send_buf_len, receive_buf, receive_buf_len, 20);
}

#if defined(CONFIG_MAIXPY_TOUCH_SCREEN_DRIVER_FT52XX)
int ft52xx_hal_i2c_recv(const uint8_t *send_buf, size_t send_buf_len, uint8_t *receive_buf,
                        size_t receive_buf_len)
{
    return maix_i2c_recv_data(m_i2c_num, FT5206_SLAVE_ADDRESS, send_buf, send_buf_len, receive_buf, receive_buf_len, 20);
}
#endif

