#ifndef __ETHERNETIF_H__
#define __ETHERNETIF_H__


#include "lwip/err.h"
#include "lwip/netif.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

/* 定义一个信号量用于PHY接受数据同步 */
extern SemaphoreHandle_t PHY_RX_xSemaphore;
/* 定义一个二值信号量用于PHY发送数据同步 */
//extern SemaphoreHandle_t PHY_TX_xSemaphore;

err_t ethernetif_init(struct netif *netif);
err_t ethernetif_input(struct netif *netif);
struct netif *ethernetif_register(void);
int ethernetif_poll(void);
void lwip_set_mac_address(unsigned char* macadd);

#ifdef SERVER

#define MAC_ADDR0 0x00
#define MAC_ADDR1 0x00
#define MAC_ADDR2 0x00
#define MAC_ADDR3 0x00
#define MAC_ADDR4 0x00
#define MAC_ADDR5 0x01

#else

#define MAC_ADDR0 0x00
#define MAC_ADDR1 0x00
#define MAC_ADDR2 0x00
#define MAC_ADDR3 0x00
#define MAC_ADDR4 0x00
//#define MAC_ADDR5 0x02
#define MAC_ADDR5 0x03
//#define MAC_ADDR5 0x04

#endif

#endif 
