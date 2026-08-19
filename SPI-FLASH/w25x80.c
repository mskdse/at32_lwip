#include "w25x80.h"

void w25x80_init(void)
{
	spi_tranrecv_init();
}

static void w25x80_wenable(void)
{
	uint8_t byt=0x06;
	spi_tranrecv_cs_low();
	spi_tranrecv_buf(&byt,NULL,1);
	spi_tranrecv_cs_high();
}

static void w25x80_wdisable(void)
{
	uint8_t byt=0x04;
	spi_tranrecv_cs_low();
	spi_tranrecv_buf(&byt,NULL,1);
	spi_tranrecv_cs_high();
}

static void w25x80_wait_busy(void)
{
   uint8_t buf[2];
	 while(1)
	 {
		 buf[0]=0x05;
		 buf[1]=0xFF;
		 spi_tranrecv_cs_low();
		 spi_tranrecv_buf(buf,buf,2);
		 spi_tranrecv_cs_high();
		 if(!(buf[1]&0x01)) return;
	 }
}

uint8_t w25x80_readid(void)
{
	uint8_t buf[5]={0xab,0xff,0xff,0xff,0xff};
	w25x80_wait_busy();
	spi_tranrecv_cs_low();
    spi_tranrecv_buf(buf,buf,5);
    spi_tranrecv_cs_high();
	return buf[4];
}

void w25x80_sector_erase(uint32_t addr)
{
	uint8_t buf[4]={0x20,(addr>>16)&0xff,(addr>>8)&0xff,addr&0xff};
	w25x80_wait_busy();
	w25x80_wenable();
	
	spi_tranrecv_cs_low();
	spi_tranrecv_buf(buf,NULL,4);
	spi_tranrecv_cs_high();
	
	w25x80_wait_busy();
	w25x80_wdisable();
}

void w25x80_read_data(uint32_t addr,uint8_t *rbuf,uint32_t len)
{
    uint8_t cmd[4]=
    {
        0x03,
        (addr>>16)&0xff,
        (addr>>8)&0xff,
        addr&0xff
    };
    uint32_t remain=len;
    uint8_t *ptr=rbuf;
		uint16_t chunk;
		
		w25x80_wait_busy();
    spi_tranrecv_cs_low();
    spi_tranrecv_buf(cmd,NULL,4);
    while(remain)
    {
        chunk =(remain > MAX_TRANRECV_SIZE) ?MAX_TRANRECV_SIZE :remain;
        spi_tranrecv_buf(ptr,ptr,chunk);
        ptr += chunk;
        remain -= chunk;
    }
    spi_tranrecv_cs_high();
}

void w25x80_write_data(uint32_t addr, uint8_t *wbuf, uint32_t len)
{
    uint8_t cmd[4];
    uint32_t remain = len;
    uint8_t *ptr = wbuf;
    uint16_t page_remain;
    uint16_t write_len;
    while(remain)
    {
			  w25x80_wait_busy();
        w25x80_wenable();

        page_remain = PAGE_SIZE - (addr % PAGE_SIZE);

        if(remain > page_remain) write_len = page_remain;
        else                     write_len = remain;

        cmd[0] = 0x02;
        cmd[1] = (addr >> 16) & 0xff;
        cmd[2] = (addr >> 8) & 0xff;
        cmd[3] =  addr & 0xff;


        spi_tranrecv_cs_low();
        spi_tranrecv_buf(cmd,NULL,4);
        spi_tranrecv_buf(ptr,NULL,write_len);
        spi_tranrecv_cs_high();

        addr += write_len;
        ptr += write_len;
        remain -= write_len;
    }
		w25x80_wait_busy();
    w25x80_wdisable();
}
