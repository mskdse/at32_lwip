#ifndef _W25X80_H_
#define _W25X80_H_


#include "spi_tranrecv.h"               // Device header
#include <string.h>


#define SECTOR_SIZE    4096
#define SECTOR_NUM      256
#define PAGE_SIZE       256

void w25x80_init(void);
uint8_t w25x80_readid(void);
void w25x80_sector_erase(uint32_t addr);
void w25x80_read_data(uint32_t addr,uint8_t *rbuf,uint32_t len);
void w25x80_write_data(uint32_t addr, uint8_t *wbuf, uint32_t len);

#endif
