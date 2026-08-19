#ifndef _SPI_TRANRECV_H_
#define _SPI_TRANRECV_H_


#include "at32f435_437.h"               // Device header

#define MAX_TRANRECV_SIZE       1024

void spi_tranrecv_init(void);
void spi_tranrecv_cs_low(void);
void spi_tranrecv_cs_high(void);
void spi_tranrecv_buf(uint8_t *tbuf,uint8_t *rbuf,uint16_t len);


#endif
