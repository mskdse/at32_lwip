#include "at32f435_437_int.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "ethernetif.h"
#include "netconf.h"

void NMI_Handler(void) {}
void HardFault_Handler(void) { while(1) {} }
void MemManage_Handler(void) { while(1) {} }
void BusFault_Handler(void) { while(1) {} }
void UsageFault_Handler(void) { while(1) {} }
void DebugMon_Handler(void) {}

void EMAC_IRQHandler(void)
{
  emac_dma_flag_clear(EMAC_DMA_RI_FLAG);
  emac_dma_flag_clear(EMAC_DMA_NIS_FLAG);
}
