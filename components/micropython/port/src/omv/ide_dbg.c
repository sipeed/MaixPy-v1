/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * USB debug support.
 *
 */
#include "mp.h"
#include "imlib.h"
#include "sensor.h"
#include "framebuffer.h"
#include "ide_dbg.h"
#include "nlr.h"
#include "lexer.h"
#include "parse.h"
#include "compile.h"
#include "runtime.h"
#include "omv_boardconfig.h"
#include "genhdr/mpversion.h"
#include "mpconfigboard.h"
#include "uarths.h"
#include "uart.h"
#include "fpioa.h"
#include "buffer.h"
#include "vfs_internal.h"
#include "sha256.h"
#include "string.h"
#include "objstr.h"
#include "sipeed_sys.h"

#if CONFIG_MAIXPY_IDE_SUPPORT

static volatile int xfer_bytes;   // bytes sent
static volatile int xfer_length;  // bytes need to send
static enum usbdbg_cmd cmd;
static volatile bool is_ide_mode = false;

static volatile bool script_need_interrupt = false;
static volatile bool script_ready;
static volatile bool script_running;
static vstr_t script_buf;
static mp_obj_exception_t ide_exception; //IDE interrupt
static mp_obj_str_t   ide_exception_str;
static mp_obj_tuple_t* ide_exception_str_tuple = NULL;
static mp_obj_t mp_const_ide_interrupt = (mp_obj_t)(&ide_exception);

static uint8_t ide_dbg_cmd_buf[IDE_DBG_MAX_PACKET] = {IDE_DBG_CMD_START_FLAG};

static volatile uint8_t ide_dbg_cmd_len_count = 0;
static size_t           temp_size;
static volatile bool    is_busy_sending = false; // sending data
static volatile bool    is_sending_jpeg = false; // sending jpeg (frame buf) data
extern Buffer_t g_uart_send_buf_ide;
static volatile uint32_t ide_file_save_status = 0; //0: ok, 1: busy recieve data, 2:eror memory, 3:open file err, 
                                                   //4: write file error, 5: busy saving, 6:parity check fail, others: unkown error
static uint32_t ide_file_length = 0;
static uint8_t* p_data_temp = NULL;

bool  ide_get_script_status()
{
    if (is_ide_mode) {
        return script_running;
    }
    else if (ide_file_save_status != 0)
    {
        return false;
    }
    return true;
}

static void vstr_init_00(vstr_t *vstr, size_t alloc) {
    if (alloc < 1) {
        alloc = 1;
    }
    if(vstr->buf)
        free(vstr->buf);
    vstr->alloc = 0;
    vstr->len = 0;
    vstr->buf = malloc(alloc);
    if(vstr->buf)
        vstr->alloc = alloc;
    vstr->fixed_buf = false;
}

static void vstr_init_11(vstr_t *vstr, size_t alloc) {
    if (alloc < 1) {
        alloc = 1;
    }
    if(vstr->alloc<= alloc)
        return;
    if(vstr->buf)
        free(vstr->buf);
    vstr->alloc = 0;
    vstr->len = 0;
    vstr->buf = malloc(alloc);
    if(vstr->buf)
        vstr->alloc = alloc;
    vstr->fixed_buf = false;
}

STATIC void vstr_ensure_extra_00(vstr_t *vstr, size_t size) {
    if (vstr->len + size > vstr->alloc) {
        if (vstr->fixed_buf) {
            // We can't reallocate, and the caller is expecting the space to
            // be there, so the only safe option is to raise an exception.
            mp_raise_msg(&mp_type_RuntimeError, NULL);
        }
        size_t new_alloc = ((vstr->len + size)+7)/8*8 + 64;
        char *new_buf = realloc(vstr->buf, new_alloc);
        if(!new_buf)
        {
            mp_raise_msg(&mp_type_MemoryError, "reduce code size please!");
        }
        vstr->alloc = new_alloc;
        vstr->buf = new_buf;
    }
}

void vstr_add_strn_00(vstr_t *vstr, const char *str, size_t len) {
    vstr_ensure_extra_00(vstr, len);
    memmove(vstr->buf + vstr->len, str, len);
    vstr->len += len;
}


bool ide_debug_init0()
{    
    ide_exception_str.data = (const byte*)"IDE interrupt";
    ide_exception_str.len  = 13;
    ide_exception_str.base.type = &mp_type_str;
    ide_exception_str.hash = qstr_compute_hash(ide_exception_str.data, ide_exception_str.len);
    ide_exception_str_tuple = (mp_obj_tuple_t*)malloc(sizeof(mp_obj_tuple_t)+sizeof(mp_obj_t)*1);
    if(ide_exception_str_tuple==NULL)
        return false;
    ide_exception_str_tuple->base.type = &mp_type_tuple;
    ide_exception_str_tuple->len = 1;
    ide_exception_str_tuple->items[0] = MP_OBJ_FROM_PTR(&ide_exception_str);
    ide_exception.base.type = &mp_type_Exception;
    ide_exception.traceback_alloc = 0;
    ide_exception.traceback_len = 0;
    ide_exception.traceback_data = NULL;
    ide_exception.args = ide_exception_str_tuple;
    memset(&script_buf, 0, sizeof(vstr_t));
    // vstr_init(&script_buf, 32);
    vstr_init_00(&script_buf, 1024*5);
    return true;
}

void ide_dbg_init()
{
    xfer_length = 0;
    xfer_bytes  = 0;
    is_busy_sending = false;
    script_ready=false;
    script_running=false;
    // vstr_clear(&script_buf);
    // vstr_init(&script_buf, 32);
    vstr_init_11(&script_buf, 1024*5);
    // mp_const_ide_interrupt = mp_obj_new_exception_msg(&mp_type_Exception, "IDE interrupt");
}

void ide_dbg_init2()
{
    int err;
    mp_obj_t f = vfs_internal_open("/flash/ide_mode.conf", "w", &err);
    vfs_internal_close(f, &err);
    is_ide_mode = true;
}

void ide_dbg_init3()
{
    int err;
    vfs_internal_remove("/flash/ide_mode.conf", &err);
    is_ide_mode = true;
}

bool is_ide_dbg_mode()
{
    return is_ide_mode;
}


ide_dbg_status_t ide_dbg_ack_data(machine_uart_obj_t* uart)
{
    uint32_t length = 0;
ack_start:
    length = xfer_length - xfer_bytes;
    length = (length>IDE_DBG_MAX_PACKET) ? IDE_DBG_MAX_PACKET : length;
    if(length == 0)
    {
        if(cmd == USBDBG_NONE)
            is_busy_sending = false;
        return IDE_DBG_STATUS_OK;
    }

    switch (cmd)
    {
        case USBDBG_FW_VERSION:
        {
            ((uint32_t*)ide_dbg_cmd_buf)[0] = MICROPY_VERSION_MAJOR;
            ((uint32_t*)ide_dbg_cmd_buf)[1] = MICROPY_VERSION_MINOR;
            ((uint32_t*)ide_dbg_cmd_buf)[2] = MICROPY_VERSION_MICRO;
            cmd = USBDBG_NONE;
            break;
        }

        case USBDBG_TX_BUF_LEN:
        {
            *((uint32_t*)ide_dbg_cmd_buf) = Buffer_Size(&g_uart_send_buf_ide);
            cmd = USBDBG_NONE;
            break;
        }

        case USBDBG_TX_BUF:
        {
            Buffer_Gets(&g_uart_send_buf_ide, ide_dbg_cmd_buf, length);
            if (xfer_bytes+ length == xfer_length)
            {
                cmd = USBDBG_NONE;
            }
            break;
        }

        case USBDBG_FRAME_SIZE:
            {
                // Return 0 if FB is locked or not ready.
                ((uint32_t*)ide_dbg_cmd_buf)[0] = 0;
                // Try to lock FB. If header size == 0 frame is not ready
                if (mutex_try_lock(&(JPEG_FB()->lock), MUTEX_TID_IDE))
                {
                    // If header size == 0 frame is not ready
                    if (JPEG_FB()->size == 0)
                    {
                        // unlock FB
                        mutex_unlock(&(JPEG_FB()->lock), MUTEX_TID_IDE);
                    } else
                    {
                        // Return header w, h and size/bpp
                        ((uint32_t*)ide_dbg_cmd_buf)[0] = JPEG_FB()->w;
                        ((uint32_t*)ide_dbg_cmd_buf)[1] = JPEG_FB()->h;
                        ((uint32_t*)ide_dbg_cmd_buf)[2] = JPEG_FB()->size;
                    }
                }
            }
            cmd = USBDBG_NONE;
            break;

        case USBDBG_FRAME_DUMP:
            if (xfer_bytes < xfer_length)
            {
                if(MICROPY_UARTHS_DEVICE == uart->uart_num)
                {
                    temp_size = uarths_send_data(JPEG_FB()->pixels, xfer_length);
                }
                else if(UART_DEVICE_MAX > uart->uart_num)
                {
                    // uart_configure(uart->uart_num, (size_t)uart->baudrate, (size_t)uart->bitwidth, uart->stop,  uart->parity);
                    temp_size= uart_send_data(uart->uart_num, (const char*)JPEG_FB()->pixels, xfer_length);
                }
                cmd = USBDBG_NONE;
                xfer_bytes = xfer_length;
                JPEG_FB()->w = 0;
                JPEG_FB()->h = 0;
                JPEG_FB()->size = 0;
                mutex_unlock(&JPEG_FB()->lock, MUTEX_TID_IDE);
            }
            break;

        case USBDBG_ARCH_STR:
        {
            snprintf((char *) ide_dbg_cmd_buf, IDE_DBG_MAX_PACKET, "%s [%s:%08X%08X%08X]",
                    OMV_ARCH_STR, OMV_BOARD_TYPE,0,0,0);//TODO: UID
            cmd = USBDBG_NONE;
            break;
        }

        case USBDBG_SCRIPT_RUNNING:
        {
            *((uint32_t*)ide_dbg_cmd_buf) = script_running?1:0;
            cmd = USBDBG_NONE;
            break;
        }
        case USBDBG_FILE_SAVE_STATUS:
        {
            *((uint32_t*)ide_dbg_cmd_buf) = ide_file_save_status;
            cmd = USBDBG_NONE;
            break;
        }
        case USBDBG_QUERY_STATUS:
        {
            *((uint32_t*)ide_dbg_cmd_buf) = 0xFFEEBBAA;
            cmd = USBDBG_NONE;
            break;
        }
        default: /* error */
            break;
    }
    if(length && !is_sending_jpeg)
    {
        if(MICROPY_UARTHS_DEVICE == uart->uart_num)
        {
            temp_size = uarths_send_data(ide_dbg_cmd_buf, length);
        }
        else if(UART_DEVICE_MAX > uart->uart_num)
            temp_size= uart_send_data(uart->uart_num, (const char*)ide_dbg_cmd_buf, length);
        xfer_bytes += length;
        if( xfer_bytes < xfer_length )
            goto ack_start;
    }
    if(cmd == USBDBG_NONE)
    {
        is_sending_jpeg = false;
        is_busy_sending = false;
    }
    return IDE_DBG_STATUS_OK;
}

ide_dbg_status_t ide_dbg_receive_data(machine_uart_obj_t* uart, uint8_t* data)
{
    switch (cmd) {
        case USBDBG_SCRIPT_EXEC:
            // check if GC is locked before allocating memory for vstr. If GC was locked
            // at least once before the script is fully uploaded xfer_bytes will be less
            // than the total length (xfer_length) and the script will Not be executed.
            if (!script_running && !gc_is_locked()) {
                vstr_add_strn_00(&script_buf, (const char*)data, 1);
                if (xfer_bytes+1 == xfer_length) {
                    // Set script ready flag
                    script_ready = true;

                    // Set script running flag
                    script_running = true;

                    // Disable IDE IRQ (re-enabled by pyexec or main).
                    // usbdbg_set_irq_enabled(false);
                    // Clear interrupt traceback
                    mp_obj_exception_clear_traceback(mp_const_ide_interrupt);
                    // Interrupt running REPL
                    // Note: setting pendsv explicitly here because the VM is probably
                    // waiting in REPL and the soft interrupt flag will not be checked.
                    // nlr_jump(mp_const_ide_interrupt);
                    MP_STATE_VM(mp_pending_exception) = mp_const_ide_interrupt;//MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception));
                    #if MICROPY_ENABLE_SCHEDULER
                    if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
                        MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
                    }
                    #endif
                }
            }
            break;

        case USBDBG_FILE_SAVE:
        {
            p_data_temp[xfer_bytes] = *data;
            if (xfer_bytes+1 == xfer_length) {
                //save to FS
                ide_file_save_status = 5;
                mp_obj_exception_clear_traceback(mp_const_ide_interrupt);
                MP_STATE_VM(mp_pending_exception) = mp_const_ide_interrupt;//MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception));
                #if MICROPY_ENABLE_SCHEDULER
                if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
                    MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
                }
                #endif
            }
            break;
        }
        case USBDBG_TEMPLATE_SAVE: {
            //TODO: save image support
            // image_t image ={
            //     .w = MAIN_FB()->w,
            //     .h = MAIN_FB()->h,
            //     .bpp = MAIN_FB()->bpp,
            //     .pixels = MAIN_FB()->pixels
            // };

            // // null terminate the path
            // length = (length == 64) ? 63:length;
            // ((char*)buffer)[length] = 0;

            // rectangle_t *roi = (rectangle_t*)buffer;
            // char *path = (char*)buffer+sizeof(rectangle_t);

            // imlib_save_image(&image, path, roi, 50);
            // raise a flash IRQ to flush image
            //NVIC->STIR = FLASH_IRQn;
            break;
        }

        case USBDBG_DESCRIPTOR_SAVE: {
            //TODO: save descriptor support
            // image_t image ={
            //     .w = MAIN_FB()->w,
            //     .h = MAIN_FB()->h,
            //     .bpp = MAIN_FB()->bpp,
            //     .pixels = MAIN_FB()->pixels
            // };

            // // null terminate the path
            // length = (length == 64) ? 63:length;
            // ((char*)buffer)[length] = 0;

            // rectangle_t *roi = (rectangle_t*)buffer;
            // char *path = (char*)buffer+sizeof(rectangle_t);

            // py_image_descriptor_from_roi(&image, path, roi);
            break;
        }
        default: /* error */
            break;
    }
    return IDE_DBG_STATUS_OK;
}

ide_dbg_status_t ide_dbg_dispatch_cmd(machine_uart_obj_t* uart, uint8_t* data)
{
    uint32_t length;

    if(ide_dbg_cmd_len_count==0)
    {
        if( is_busy_sending ) // throw out data //TODO: maybe need queue data?
            return IDE_DBG_DISPATCH_STATUS_BUSY;
        length = xfer_length - xfer_bytes;
        if(length)//receive data from IDE
        {
            ide_dbg_receive_data(uart, data);
            ++xfer_bytes;
            return IDE_DBG_STATUS_OK;
        }
        if(*data == IDE_DBG_CMD_START_FLAG)
            ide_dbg_cmd_len_count = 1;
    }
    else
    {
        ide_dbg_cmd_buf[ide_dbg_cmd_len_count++] = *data;
        if(ide_dbg_cmd_len_count < 6)
            return IDE_DBG_DISPATCH_STATUS_WAIT;
        length = *( (uint32_t*)(ide_dbg_cmd_buf+2) );
        cmd = ide_dbg_cmd_buf[1];
        switch (cmd) {
            case USBDBG_FW_VERSION:
            {
                int err;
                xfer_bytes = 0;
                xfer_length = length;
                vfs_internal_remove("/flash/ide_mode.conf", &err);
                break;
            }
            case USBDBG_FRAME_SIZE:
                xfer_bytes = 0;
                xfer_length = length;
                break;

            case USBDBG_FRAME_DUMP:
                xfer_bytes = 0;
                xfer_length = length;
                if(length)
                    is_sending_jpeg = true;
                break;

            case USBDBG_ARCH_STR:
                xfer_bytes = 0;
                xfer_length = length;
                break;

            case USBDBG_SCRIPT_EXEC:
                xfer_bytes = 0;
                xfer_length = length;
                vstr_reset(&script_buf);
                break;

            case USBDBG_SCRIPT_STOP:
            {
                if (script_running) {
                    // Set script running flag
                    script_running = false;
                    // // Disable IDE IRQ (re-enabled by pyexec or main).
                    // usbdbg_set_irq_enabled(false);

                    // interrupt running code by raising an exception
                    mp_obj_exception_clear_traceback(mp_const_ide_interrupt);
                    // pendsv_nlr_jump_hard(mp_const_ide_interrupt);
                    MP_STATE_VM(mp_pending_exception) = mp_const_ide_interrupt; //MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception));
                    #if MICROPY_ENABLE_SCHEDULER
                    if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
                        MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
                    }
                    #endif
                    script_need_interrupt = true;
                }
                cmd = USBDBG_NONE;
            }
                break;

            case USBDBG_FILE_SAVE:
                xfer_bytes = 0;
                xfer_length = length;
                ide_file_save_status = 0;
                if(length)
                {
                    ide_file_save_status = 1;
                    ide_file_length = length;
                    if( p_data_temp )
                    {
                        free(p_data_temp);
                    }
                    p_data_temp = malloc( (length%4)? (length+4-(length%4)): length );
                    if(!p_data_temp)
                    {
                        xfer_length = 0;
                        ide_file_length = 0;
                        ide_file_save_status = 2;
                    }
                }
                break;
            case USBDBG_FILE_SAVE_STATUS:
                xfer_bytes = 0;
                xfer_length = length;
                break;
            case USBDBG_SCRIPT_RUNNING:
                xfer_bytes = 0;
                xfer_length =length;
                break;

            case USBDBG_TEMPLATE_SAVE:
            case USBDBG_DESCRIPTOR_SAVE:
                /* save template */
                xfer_bytes = 0;
                xfer_length =length;
                break;

            case USBDBG_ATTR_WRITE: {
                if(ide_dbg_cmd_len_count < 10)
                    return IDE_DBG_DISPATCH_STATUS_WAIT;
                /* write sensor attribute */
                int16_t attr = *( (int16_t*)(ide_dbg_cmd_buf+6) );
                int16_t val = *( (int16_t*)(ide_dbg_cmd_buf+8) );
                switch (attr) {
                    case ATTR_CONTRAST:
                        sensor_set_contrast(val);
                        break;
                    case ATTR_BRIGHTNESS:
                        sensor_set_brightness(val);
                        break;
                    case ATTR_SATURATION:
                        sensor_set_saturation(val);
                        break;
                    case ATTR_GAINCEILING:
                        sensor_set_gainceiling(val);
                        break;
                    default:
                        break;
                }
                cmd = USBDBG_NONE;
                break;
            }

            case USBDBG_SYS_RESET:
                sipeed_sys_reset();
                break;

            case USBDBG_FB_ENABLE: 
            {
                if(ide_dbg_cmd_len_count < 8)
                    return IDE_DBG_DISPATCH_STATUS_WAIT;
                int16_t enable = *( (int16_t*)(ide_dbg_cmd_buf+6) );
                JPEG_FB()->enabled = enable;
                if (enable == 0) {
                    // When disabling framebuffer, the IDE might still be holding FB lock.
                    // If the IDE is not the current lock owner, this operation is ignored.
                    mutex_unlock(&JPEG_FB()->lock, MUTEX_TID_IDE);
                }
                xfer_bytes = 0;
                xfer_length = length;
                cmd = USBDBG_NONE;
                break;
            }

            case USBDBG_TX_BUF:
            case USBDBG_TX_BUF_LEN:
                xfer_bytes = 0;
                xfer_length = length;
                break;
            case USBDBG_QUERY_STATUS:
                xfer_bytes = 0;
                xfer_length = length;
                break;
            default: /* error */
                cmd = USBDBG_NONE;
                break;
        }
        ide_dbg_cmd_len_count = 0; // all cmd data received ok
        if(length && (cmd&0x80) ) // need send data to IDE
        {
            is_busy_sending = true;
            ide_dbg_ack_data(uart);    // ack data
        }
    }
    return IDE_DBG_STATUS_OK;
}

bool ide_dbg_script_ready()
{
    return script_ready;
}

vstr_t* ide_dbg_get_script()
{
    return &script_buf;
}

bool      ide_dbg_need_save_file()
{
    bool ret = (ide_file_save_status==5);
    if(ide_file_save_status != 1 && !ret ) // not busy receive data
    {
        if(p_data_temp)
        {
            free(p_data_temp);
            p_data_temp = NULL;
        }
    }
    return ret;
}

int check_sha256(uint8_t* hash, uint8_t* data, size_t data_len)
{
    bool hash_ok = false;
    uint8_t* res = (uint8_t*)malloc(32);
    if(!res)
        return -1;
    sha256_hard_calculate(data, data_len, res);
    if( memcmp(hash, res, 32) == 0)
        hash_ok = true;
    free(res);
    if(hash_ok)
        return 0;
    return -2;
}




void      ide_save_file()
{
    int err;
    uint8_t* data;
    if(!p_data_temp)
    {
        ide_file_save_status = 100;//should not happen
        return ;
    }

    err = check_sha256(p_data_temp, p_data_temp+32, ide_file_length-32);
    if(err == -1)// no memory
    {
        ide_file_save_status = 2;
    }
    else if( err == -2) // parity error
    {
        ide_file_save_status = 6;
    }
    else// parity right
    {

        uint8_t* file_name = p_data_temp+32;
        int tmp = strlen((const char*)file_name)+1;
        tmp = tmp + 4-((tmp%4)?(tmp%4):4);
        data = file_name + tmp;
        uint32_t file_len = ide_file_length - 32 - tmp;
        mp_obj_t file = vfs_internal_open((const char*)file_name, "wb", &err);
        
        if( file==NULL || err!=0)
        {
            ide_file_save_status = 3;
        }
        else
        {
            mp_uint_t ret = vfs_internal_write(file, (void*)data, file_len, &err);
            if(err!=0 || ret != file_len)
                ide_file_save_status = 4;
            else
                ide_file_save_status = 0;
            vfs_internal_close(file, &err);
        }
    }
    free(p_data_temp);
    p_data_temp = NULL;
}


bool ide_dbg_interrupt_main()
{
    if (script_need_interrupt) {
        // interrupt running code by raising an exception
        mp_obj_exception_clear_traceback(mp_const_ide_interrupt);
        // pendsv_nlr_jump_hard(mp_const_ide_interrupt);
        MP_STATE_VM(mp_pending_exception) = mp_const_ide_interrupt; //MP_OBJ_FROM_PTR(&MP_STATE_VM(mp_kbd_exception));
        #if MICROPY_ENABLE_SCHEDULER
        if (MP_STATE_VM(sched_state) == MP_SCHED_IDLE) {
            MP_STATE_VM(sched_state) = MP_SCHED_PENDING;
        }
        #endif
    }
    return script_need_interrupt;
}

void ide_dbg_on_script_end()
{
    script_need_interrupt = false;
}


#else // CONFIG_MAIXPY_IDE_SUPPORT /////////////////////////////////////

bool ide_debug_init0()
{
    return true;
}

void ide_dbg_init()
{

}

void ide_dbg_init2()
{

}

void ide_dbg_init3()
{

}

bool     ide_dbg_script_ready()
{
    return false;
}
vstr_t*  ide_dbg_get_script()
{
    return NULL;
}

bool      ide_dbg_need_save_file()
{
    return false;
}

void      ide_save_file()
{

}

bool is_ide_dbg_mode()
{
    return false;
}

bool ide_dbg_interrupt_main()
{
    return true;
}

void ide_dbg_on_script_end()
{

}
#endif

