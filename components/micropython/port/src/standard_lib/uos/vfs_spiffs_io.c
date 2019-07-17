#include <stdio.h>

#include "py/binary.h"
#include "py/objarray.h"
#include "py/misc.h"
#include "py/runtime.h"
#include "vfs_spiffs.h"

#include "w25qxx.h"
#include "sleep.h"
#include "syscalls.h"

#include "spiffs_config.h"
#include "spiffs_configport.h"

typedef struct _spiffs_FILE{
    spiffs* fs;
    spiffs_file fd;
} spiffs_FILE;


s32_t sys_spiffs_read(uint32_t addr, uint32_t size, uint8_t *buf)
{
    uint32_t phy_addr=addr;
    w25qxx_status_t res = w25qxx_read_data_dma(phy_addr, buf, size,W25QXX_QUAD_FAST);
	#if open_fs_debug
    mp_printf(&mp_plat_print, "flash read addr:%x size:%d buf_head:%x %x\n",phy_addr,size,buf[0],buf[1]);
	#endif
    if (res != W25QXX_OK) {
		#if open_fs_debug
        mp_printf(&mp_plat_print, "spifalsh read err\n");
		#endif
        return res;
    }
    return res;
}
s32_t sys_spiffs_write(uint32_t addr, uint32_t size, uint8_t *buf)
{
    int phy_addr=addr;
    
    w25qxx_status_t res = w25qxx_write_data_dma(phy_addr, buf, size);
	#if open_fs_debug
    mp_printf(&mp_plat_print, "flash write addr:%x size:%d buf_head:%x,%x\n",phy_addr,size,buf[0],buf[1]);
	#endif
    if (res != W25QXX_OK) {
		#if open_fs_debug
        mp_printf(&mp_plat_print, "spifalsh write err\n");
		#endif
        return res;
    }
    return res;
}
s32_t sys_spiffs_erase(uint32_t addr, uint32_t size)
{
    int phy_addr=addr;
	#if open_fs_debug
    mp_printf(&mp_plat_print, "flash erase addr:%x size:%f\n",phy_addr,size/1024.00);
	#endif
    w25qxx_status_t res = w25qxx_sector_erase_dma(phy_addr);
    if (res != W25QXX_OK) {
		#if open_fs_debug
        mp_printf(&mp_plat_print, "spifalsh erase err\n");
		#endif
        return res;
    }
    return res;
}

s32_t spiffs_read_method(spiffs* fs,uint32_t addr, uint32_t size, uint8_t *buf)
{
	spiffs_user_mount_t *vfs = fs->user_data;
    if (vfs == NULL) {
        return -1;
    }
    if (vfs->flags == SYS_SPIFFS) {
		w25qxx_status_t res = sys_spiffs_read(addr,size,buf);
		if(res == W25QXX_OK)
			 return SPIFFS_OK;
    } else {
		mp_obj_array_t data = {{&mp_type_bytearray}, BYTEARRAY_TYPECODE, 0, size, buf};
		vfs->read_obj[2] = MP_OBJ_FROM_PTR(&data);//read buf
        vfs->read_obj[3] = MP_OBJ_NEW_SMALL_INT(size);//size
		vfs->read_obj[4] = MP_OBJ_NEW_SMALL_INT(addr);//addr this addr means offset of buf
        mp_call_method_n_kw(3, 0, vfs->read_obj);
		return SPIFFS_OK;
        // TODO handle error return
    }
   return SPIFFS_OK;
}


s32_t spiffs_write_method(spiffs* fs,uint32_t addr, uint32_t size, uint8_t *buf)
{
	spiffs_user_mount_t *vfs = fs->user_data;
    if (vfs == NULL) {
        return -1;
    }
    if (vfs->flags == SYS_SPIFFS) {
		w25qxx_status_t res = sys_spiffs_write(addr,size,buf);
		if(res == W25QXX_OK)
			 return SPIFFS_OK;
    } else {	
		mp_obj_t data = mp_obj_new_bytearray(size, buf);
		vfs->write_obj[2] = MP_OBJ_FROM_PTR(data);//read buf
        vfs->write_obj[3] = MP_OBJ_NEW_SMALL_INT(size);//size
		vfs->write_obj[4] = MP_OBJ_NEW_SMALL_INT(addr);//addr this addr means offset of buf
        mp_call_method_n_kw(3, 0, vfs->write_obj);
		return SPIFFS_OK;
        // TODO handle error return
    }
	return SPIFFS_OK;
}


s32_t spiffs_erase_method(spiffs* fs,uint32_t addr, uint32_t size)
{
	spiffs_user_mount_t *vfs = fs->user_data;
    if (vfs == NULL) {
        return -1;
    }
    if (vfs->flags == SYS_SPIFFS) {
		w25qxx_status_t res = sys_spiffs_erase(addr,size);
		if(res == W25QXX_OK)
			 return SPIFFS_OK;
    } else {	
   		vfs->erase_obj[2] = MP_OBJ_NEW_SMALL_INT(size);
        vfs->erase_obj[3] = MP_OBJ_NEW_SMALL_INT(addr);
        mp_call_method_n_kw(2, 0, vfs->erase_obj);
		// TODO handle error return
		return SPIFFS_OK;
    } 
	return SPIFFS_OK;
}


int mp_module_spiffs_mount(spiffs* fs,spiffs_config* cfg)
{
	u8_t* spiffs_work_buf = (u8_t*)m_malloc(cfg->log_page_size * 2 );
	u8_t* spiffs_fds = (u8_t*)m_malloc(cfg->log_block_size/cfg->log_page_size*4);
	u8_t* spiffs_cache_buf = (u8_t*)m_malloc((cfg->log_page_size+cfg->log_block_size/cfg->log_page_size)*4);
	int res = SPIFFS_mount(fs,
						   cfg,
						   spiffs_work_buf,
						   spiffs_fds,
						   cfg->log_block_size/cfg->log_page_size*4,
					       spiffs_cache_buf,
						   (cfg->log_block_size/cfg->log_page_size)*4,
						   0);
	mp_printf(&mp_plat_print, "[MAIXPY]:Spiffs Mount %s \n", res?"failed":"successful");
	return res;
}

int mp_module_spiffs_format(spiffs* fs)
{
	
	SPIFFS_unmount(fs);
	mp_printf(&mp_plat_print, "[MAIXPY]:Spiffs Unmount.\n");
	mp_printf(&mp_plat_print, "[MAIXPY]:Spiffs Formating...\n");
	s32_t format_res=SPIFFS_format(fs);
	mp_printf(&mp_plat_print, "[MAIXPY]:Spiffs Format %s \n",format_res?"failed":"successful");
	return format_res;
}

