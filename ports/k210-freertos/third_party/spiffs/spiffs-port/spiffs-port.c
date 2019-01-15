#include "spiffs-port.h"
#include "spiffs_config.h"
#include "spiffs_configport.h"
#include <stdio.h>
#include "w25qxx.h"

s32_t flash_read(uint32_t addr, uint32_t size, uint8_t *buf)
{
    int phy_addr=addr;
    w25qxx_status_t res = w25qxx_read_data_dma(phy_addr, buf, size, W25QXX_QUAD_FAST);
	#if open_fs_debug
    printf("flash read addr:%x size:%d buf_head:%x %x\n",phy_addr,size,buf[0],buf[1]);
	#endif
    if (res != W25QXX_OK) {
		#if open_fs_debug
        printf("spifalsh read err\n");
		#endif
        return SPIFFS_ERR_FULL;
    }
    return SPIFFS_OK;
}
s32_t flash_write(uint32_t addr, uint32_t size, uint8_t *buf)
{
    int phy_addr=addr;

    w25qxx_status_t res = w25qxx_write_data_dma(phy_addr, buf, size);
	#if open_fs_debug
    printf("flash write addr:%x size:%d buf_head:%x,%x\n",phy_addr,size,buf[0],buf[1]);
	#endif
    if (res != W25QXX_OK) {
		#if open_fs_debug
        printf("spifalsh write err\n");
		#endif
        return SPIFFS_ERR_FULL;
    }

    return SPIFFS_OK;
}
s32_t flash_erase(uint32_t addr, uint32_t size)
{
    int phy_addr=addr;
	#if open_fs_debug
    printf("flash erase addr:%x size:%f\n",phy_addr,size/1024.00);
	#endif
    w25qxx_status_t res = w25qxx_sector_erase_dma(phy_addr);
	while (w25qxx_is_busy_dma() == W25QXX_BUSY);
    if (res != W25QXX_OK) {
		#if open_fs_debug
        printf("spifalsh erase err\n");
		#endif
        return SPIFFS_ERR_FULL;
    }
    return SPIFFS_OK;
}

void test_lock(spiffs *fs) {
}

void test_unlock(spiffs *fs) {
}

