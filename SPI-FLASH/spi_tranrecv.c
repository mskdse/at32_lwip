#include "spi_tranrecv.h"
#include <string.h>

/* 

SPI1
spi1_cs------>pb2
spi1_mosi---->pb5
spi1_miso---->pb4
spi1_clk----->pb3

*/

static dma_init_type dma_tx_cfg;
static dma_init_type dma_rx_cfg;
static uint8_t dma_rtx_buf[MAX_TRANRECV_SIZE] __attribute__((aligned(4)));

void spi_tranrecv_init(void)
{
	crm_periph_clock_enable(CRM_GPIOB_PERIPH_CLOCK,TRUE);
	crm_periph_clock_enable(CRM_SPI1_PERIPH_CLOCK,TRUE);
	crm_periph_clock_enable(CRM_DMA1_PERIPH_CLOCK,TRUE);
	
	gpio_init_type gpiox_cfg;
	gpiox_cfg.gpio_drive_strength=GPIO_DRIVE_STRENGTH_STRONGER;
	gpiox_cfg.gpio_mode=GPIO_MODE_OUTPUT;
	gpiox_cfg.gpio_out_type=GPIO_OUTPUT_PUSH_PULL;
	gpiox_cfg.gpio_pins=GPIO_PINS_2;
	gpiox_cfg.gpio_pull=GPIO_PULL_NONE;
	gpio_init(GPIOB,&gpiox_cfg);
	
	gpiox_cfg.gpio_mode=GPIO_MODE_MUX;
	gpiox_cfg.gpio_pins=GPIO_PINS_3;
	gpio_init(GPIOB,&gpiox_cfg);
	gpiox_cfg.gpio_pins=GPIO_PINS_4;
	gpio_init(GPIOB,&gpiox_cfg);
	gpiox_cfg.gpio_pins=GPIO_PINS_5;
	gpio_init(GPIOB,&gpiox_cfg);
	
	gpio_pin_mux_config(GPIOB,GPIO_PINS_SOURCE3,GPIO_MUX_5);
	gpio_pin_mux_config(GPIOB,GPIO_PINS_SOURCE4,GPIO_MUX_5);
	gpio_pin_mux_config(GPIOB,GPIO_PINS_SOURCE5,GPIO_MUX_5);
	
	spi_init_type spix_cfg;
	spi_default_para_init(&spix_cfg);
	spix_cfg.clock_phase=SPI_CLOCK_PHASE_1EDGE;
	spix_cfg.clock_polarity=SPI_CLOCK_POLARITY_LOW;
	spix_cfg.cs_mode_selection=SPI_CS_SOFTWARE_MODE;
	spix_cfg.first_bit_transmission=SPI_FIRST_BIT_MSB;
	spix_cfg.frame_bit_num=SPI_FRAME_8BIT;
	spix_cfg.master_slave_mode=SPI_MODE_MASTER;
	spix_cfg.mclk_freq_division=SPI_MCLK_DIV_256;
	spix_cfg.transmission_mode=SPI_TRANSMIT_FULL_DUPLEX;
	spi_init(SPI1,&spix_cfg);
	
	spi_i2s_dma_transmitter_enable(SPI1,TRUE);
	spi_i2s_dma_receiver_enable(SPI1,TRUE);
	
	dmamux_enable(DMA1, TRUE);
	
	dma_reset(DMA1_CHANNEL1);
	dma_default_para_init(&dma_tx_cfg);
	dma_tx_cfg.buffer_size=0;
	dma_tx_cfg.direction=DMA_DIR_MEMORY_TO_PERIPHERAL;
	dma_tx_cfg.loop_mode_enable=FALSE;
	dma_tx_cfg.memory_base_addr=(uint32_t)dma_rtx_buf;
	dma_tx_cfg.memory_data_width=DMA_MEMORY_DATA_WIDTH_BYTE;
	dma_tx_cfg.memory_inc_enable=TRUE;
	dma_tx_cfg.peripheral_base_addr=(uint32_t)&SPI1->dt;
	dma_tx_cfg.peripheral_data_width=DMA_PERIPHERAL_DATA_WIDTH_BYTE;
	dma_tx_cfg.peripheral_inc_enable=FALSE;
	dma_tx_cfg.priority=DMA_PRIORITY_HIGH;
	dma_init(DMA1_CHANNEL1,&dma_tx_cfg);
	dmamux_init(DMA1MUX_CHANNEL1, DMAMUX_DMAREQ_ID_SPI1_TX);
	
	dma_reset(DMA1_CHANNEL2);
	dma_default_para_init(&dma_rx_cfg);
	dma_rx_cfg.buffer_size=0;
	dma_rx_cfg.direction=DMA_DIR_PERIPHERAL_TO_MEMORY;
	dma_rx_cfg.loop_mode_enable=FALSE;
	dma_rx_cfg.memory_base_addr=(uint32_t)dma_rtx_buf;
	dma_rx_cfg.memory_data_width=DMA_MEMORY_DATA_WIDTH_BYTE;
	dma_rx_cfg.memory_inc_enable=TRUE;
	dma_rx_cfg.peripheral_base_addr=(uint32_t)&SPI1->dt;
	dma_rx_cfg.peripheral_data_width=DMA_PERIPHERAL_DATA_WIDTH_BYTE;
	dma_rx_cfg.peripheral_inc_enable=FALSE;
	dma_rx_cfg.priority=DMA_PRIORITY_HIGH;
	dma_init(DMA1_CHANNEL2,&dma_rx_cfg);
	dmamux_init(DMA1MUX_CHANNEL2, DMAMUX_DMAREQ_ID_SPI1_RX);
	
	dma_channel_enable(DMA1_CHANNEL1,FALSE);
	dma_channel_enable(DMA1_CHANNEL2,FALSE);
	
	gpio_bits_set(GPIOB,GPIO_PINS_2);
	
	spi_enable(SPI1, TRUE);
}

void spi_tranrecv_cs_low(void)
{
	gpio_bits_reset(GPIOB,GPIO_PINS_2);
}

void spi_tranrecv_cs_high(void)
{
	gpio_bits_set(GPIOB,GPIO_PINS_2);
}

void spi_tranrecv_buf(uint8_t *tbuf,uint8_t *rbuf,uint16_t len)
{
    dma_channel_enable(DMA1_CHANNEL1, FALSE);
    dma_channel_enable(DMA1_CHANNEL2, FALSE);

    dma_flag_clear(DMA1_FDT1_FLAG);
    dma_flag_clear(DMA1_FDT2_FLAG);

    memcpy(dma_rtx_buf, tbuf, len);

    dma_tx_cfg.buffer_size = len;
    dma_rx_cfg.buffer_size = len;

    dma_init(DMA1_CHANNEL1, &dma_tx_cfg);
    dma_init(DMA1_CHANNEL2, &dma_rx_cfg);

    dma_channel_enable(DMA1_CHANNEL2, TRUE);
    dma_channel_enable(DMA1_CHANNEL1, TRUE);

    while(!dma_flag_get(DMA1_FDT2_FLAG));
	  while(!dma_flag_get(DMA1_FDT1_FLAG));
    while(spi_i2s_flag_get(SPI1, SPI_I2S_BF_FLAG) != RESET);

    if(rbuf!=NULL) memcpy(rbuf, dma_rtx_buf, len);
}
