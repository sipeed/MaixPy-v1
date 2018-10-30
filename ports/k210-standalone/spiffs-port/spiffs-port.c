#include "w25qxx.h"
#include "spiffs-port.h"
#include "spiffs_config.h"
#include <stdio.h>
#include "sleep.h"

#define foce_format_fs 0

spiffs fs;

static u8_t spiffs_work_buf[SPIFFS_CFG_LOG_PAGE_SZ(fs)*2];
static u8_t spiffs_fds[32*4];
static u8_t spiffs_cache_buf[(SPIFFS_CFG_LOG_PAGE_SZ(fs)+32)*4];

s32_t my_spi_read(int addr, int size, char *buf)
{
    int phy_addr=addr;
    enum w25qxx_status_t res = w25qxx_read_data(phy_addr, buf, size,W25QXX_QUAD);
	#if open_fs_debug
    printf("flash read addr:%x size:%d buf_head:%x %x\n",phy_addr,size,buf[0],buf[1]);
	#endif
    if (res != W25QXX_OK) {
        printf("spifalsh read err\n");
        return SPIFFS_ERR_FULL;
    }
    return SPIFFS_OK;
}
s32_t my_spi_write(int addr, int size, char *buf)
{
    int phy_addr=addr;
    
    enum w25qxx_status_t res = w25qxx_write_data(phy_addr, buf, size);
	#if open_fs_debug
    printf("flash write addr:%x size:%d buf_head:%x,%x\n",phy_addr,size,buf[0],buf[1]);
	#endif
    if (res != W25QXX_OK) {
        printf("spifalsh write err\n");
        return SPIFFS_ERR_FULL;
    }
    return SPIFFS_OK;
}
s32_t my_spi_erase(int addr, int size)
{
    int phy_addr=addr;
    unsigned char *temp_pool;
	#if open_fs_debug
    printf("flash erase addr:%x size:%f\n",phy_addr,size/1024.00);
	#endif
    enum w25qxx_status_t res = w25qxx_32k_block_erase(phy_addr);
    if (res != W25QXX_OK) {
        printf("spifalsh erase err\n");
        return SPIFFS_ERR_FULL;
    }
    return SPIFFS_OK;
}

void test_lock(spiffs *fs) {
}

void test_unlock(spiffs *fs) {
}

void my_spiffs_init(){

	spiffs_config cfg;
	cfg.phys_size = SPIFFS_CFG_PHYS_SZ(fs); // use all spi flash
	cfg.phys_addr = SPIFFS_CFG_PHYS_ADDR(fs); // start spiffs at start of spi flash
	cfg.phys_erase_block = SPIFFS_CFG_PHYS_ERASE_SZ(fs); // according to datasheet
	cfg.log_block_size = SPIFFS_CFG_LOG_BLOCK_SZ(fs); // let us not complicate things
	cfg.log_page_size = SPIFFS_CFG_LOG_PAGE_SZ(fs); // as we said

	cfg.hal_read_f = my_spi_read;
	cfg.hal_write_f = my_spi_write;
	cfg.hal_erase_f = my_spi_erase;

	int res = SPIFFS_mount(&fs,
						   &cfg,
						   spiffs_work_buf,
						   spiffs_fds,
						   sizeof(spiffs_fds),
					       spiffs_cache_buf,
						   sizeof(spiffs_cache_buf),
						   0);
		if(foce_format_fs || res != SPIFFS_OK || res==SPIFFS_ERR_NOT_A_FS)
		{
			SPIFFS_unmount(&fs);printf("spiffs unmounted...\n");
			printf("spiffs formating...\n");
			s32_t format_res=SPIFFS_format(&fs);
			printf("spiffs formated res %d\n",format_res);
			res = SPIFFS_mount(&fs,
				&cfg,
				spiffs_work_buf,
				spiffs_fds,
				sizeof(spiffs_fds),
				spiffs_cache_buf,
				sizeof(spiffs_cache_buf),
				0);
		}
		printf("spiffs mount %s \n", res?"failed":"successful");
	return;

}

int format_fs(void)
{
	spiffs_config cfg;
	cfg.phys_size = SPIFFS_CFG_PHYS_SZ(fs); // use all spi flash
	cfg.phys_addr = SPIFFS_CFG_PHYS_ADDR(fs); // start spiffs at start of spi flash
	cfg.phys_erase_block = SPIFFS_CFG_PHYS_ERASE_SZ(fs); // according to datasheet
	cfg.log_block_size = SPIFFS_CFG_LOG_BLOCK_SZ(fs); // let us not complicate things
	cfg.log_page_size = SPIFFS_CFG_LOG_PAGE_SZ(fs); // as we said

	cfg.hal_read_f = my_spi_read;
	cfg.hal_write_f = my_spi_write;
	cfg.hal_erase_f = my_spi_erase;

	SPIFFS_unmount(&fs);printf("spiffs unmounted...\n");
	printf("spiffs formating...\n");
	s32_t format_res=SPIFFS_format(&fs);
	printf("spiffs formated res %d\n",format_res);
//	printf("spiffs formated end \n");
//	printf("w25qxx_page_program_fun addr %p\n",w25qxx_page_program_fun);
//	printf("spiffs formated \n");
	int res = SPIFFS_mount( &fs,
						&cfg,
						spiffs_work_buf,
						spiffs_fds,
						sizeof(spiffs_fds),
						spiffs_cache_buf,
						sizeof(spiffs_cache_buf),
						0);

	return res;
}








