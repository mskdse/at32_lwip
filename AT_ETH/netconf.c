/**
  **************************************************************************
  * @file     netconf.c
  * @version  v2.0.0
  * @date     2020-11-02
  * @brief    network connection configuration
  **************************************************************************
  *                       Copyright notice & Disclaimer
  *
  * The software Board Support Package (BSP) that is made available to 
  * download from Artery official website is the copyrighted work of Artery. 
  * Artery authorizes customers to use, copy, and distribute the BSP 
  * software and its related documentation for the purpose of design and 
  * development in conjunction with Artery microcontrollers. Use of the 
  * software is governed by this copyright notice and the following disclaimer.
  *
  * THIS SOFTWARE IS PROVIDED ON "AS IS" BASIS WITHOUT WARRANTIES,
  * GUARANTEES OR REPRESENTATIONS OF ANY KIND. ARTERY EXPRESSLY DISCLAIMS,
  * TO THE FULLEST EXTENT PERMITTED BY LAW, ALL EXPRESS, IMPLIED OR
  * STATUTORY OR OTHER WARRANTIES, GUARANTEES OR REPRESENTATIONS,
  * INCLUDING BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT.
  *
  **************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "lwip/memp.h"
#include "lwip/tcp.h"
#include "lwip/tcp_impl.h"
#include "lwip/udp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "ethernetif.h"
#include "netconf.h"
#include "stdio.h"
#include "at32_emac_phy.h"
#include "lwip/tcpip.h"

/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
#define MAC_ADDR_LENGTH                  (6)
#define ADDR_LENGTH                      (4)
/* Private variables ---------------------------------------------------------*/
struct netif netif;
TaskHandle_t link_status_handler;
volatile uint32_t tcp_timer = 0;
volatile uint32_t arp_timer = 0;
#if LWIP_DNS
volatile uint32_t dns_timer = 0;
#endif

static uint8_t mac_address[MAC_ADDR_LENGTH] = {0x00, 0xE0, 0x00, 0x11, 0x22, 0x33};
#if LWIP_DHCP
volatile uint32_t dhcp_fine_timer = 0;
volatile uint32_t dhcp_coarse_timer = 0;
#else
static uint8_t local_ip[ADDR_LENGTH]   = {169, 254, 79, 50};
static uint8_t local_gw[ADDR_LENGTH]   = {169, 254, 79, 81};
static uint8_t local_mask[ADDR_LENGTH] = {255, 255, 0, 0};
#endif

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  initializes the lwip stack
  * @param  none
  * @retval none
  */
void tcpip_stack_init(void)
{
  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;
 
	tcpip_init(NULL,NULL);


#if LWIP_DHCP  //need DHCP server
  ipaddr.addr = 0;
  netmask.addr = 0;
  gw.addr = 0;

#else
  IP4_ADDR(&ipaddr, local_ip[0], local_ip[1], local_ip[2], local_ip[3]);
  IP4_ADDR(&netmask, local_mask[0], local_mask[1], local_mask[2], local_mask[3]);
  IP4_ADDR(&gw, local_gw[0], local_gw[1], local_gw[2], local_gw[3]);
#endif

  lwip_set_mac_address(mac_address);

  /* - netif_add(struct netif *netif, struct ip_addr *ipaddr,
            struct ip_addr *netmask, struct ip_addr *gw,
            void *state, err_t (* init)(struct netif *netif),
            err_t (* input)(struct pbuf *p, struct netif *netif))
    
   Adds your network interface to the netif_list. Allocate a struct
  netif and pass a pointer to this structure as the first argument.
  Give pointers to cleared ip_addr structures when using DHCP,
  or fill them with sane numbers otherwise. The state pointer may be NULL.

  The init function pointer must point to a initialization function for
  your ethernet netif interface. The following code illustrates it's use.*/
  
  if(netif_add(&netif, &ipaddr, &netmask, &gw, NULL, &ethernetif_init, &tcpip_input) == NULL)
  {
    while(1);
  }
  
  /*  Registers the default network interface.*/
  netif_set_default(&netif);
	
#if LWIP_DHCP
  /*  Creates a new DHCP client for this interface on the first call.
  Note: you must call dhcp_fine_tmr() and dhcp_coarse_tmr() at
  the predefined regular intervals after starting the client.
  You can peek in the netif->dhcp struct for the actual DHCP status.*/
  dhcp_start(&netif);
#endif
	
	/*  When the netif is fully configured this function must be called.*/
  netif_set_up(&netif);  
	
	printf("IP:%s\r\n", ipaddr_ntoa(&netif.ip_addr));
  printf("MASK:%s\r\n", ipaddr_ntoa(&netif.netmask));
  printf("GW:%s\r\n", ipaddr_ntoa(&netif.gw));
  
  /* Set the link callback function, this function is called on change of link status*/
  netif_set_link_callback(&netif, ethernetif_update_config);
  
  /* Create the Ethernet link handler thread */
  xTaskCreate((TaskFunction_t)ethernetif_set_link, "ethernetif_set_link", 512, &netif, tskIDLE_PRIORITY + 3, &link_status_handler);
}

/**
  * @brief  called when a frame is received
  * @param  none
  * @retval none
  */
void lwip_pkt_handle(void)
{
  /* Read a received packet from the Ethernet buffers and send it to the lwIP for handling */
  if(ethernetif_input(&netif) != ERR_OK)
  {
//    while(1);
  }
}

/**
  * @brief  lwip periodic tasks
  * @param  localtime the current localtime value
  * @retval none
  */
void lwip_periodic_handle(volatile uint32_t localtime)
{
  /* DHCP address change detection: print when IP is first obtained */
  {
    static uint32_t last_ip = 0;
    if (last_ip == 0 && netif.ip_addr.addr != 0)
    {
      last_ip = netif.ip_addr.addr;
      printf("\r\n=== DHCP bound ===\r\n");
      printf("IP  : %s\r\n", ipaddr_ntoa(&netif.ip_addr));
      printf("MASK: %s\r\n", ipaddr_ntoa(&netif.netmask));
      printf("GW  : %s\r\n", ipaddr_ntoa(&netif.gw));
      printf("==================\r\n\r\n");
      /* Configure DNS server */
      {
        ip_addr_t dns_srv;
        dns_srv.addr = ipaddr_addr(DNS_SERVER_IP_ADDR);
        dns_setserver(0, &dns_srv);
      }
    }
  }

  /* TCP periodic process every 250 ms */
  if (localtime - tcp_timer >= TCP_TMR_INTERVAL)
  {
    tcp_timer =  localtime;
    tcp_tmr();
  }
  /* ARP periodic process every 5s */
  if (localtime - arp_timer >= ARP_TMR_INTERVAL)
  {
    arp_timer =  localtime;
    etharp_tmr();
  }

#if LWIP_DHCP
  /* Fine DHCP periodic process every 500ms */
  if (localtime - dhcp_fine_timer >= DHCP_FINE_TIMER_MSECS)
  {
    dhcp_fine_timer =  localtime;
    dhcp_fine_tmr();
  }
  /* DHCP Coarse periodic process every 60s */
  if (localtime - dhcp_coarse_timer >= DHCP_COARSE_TIMER_MSECS)
  {
    dhcp_coarse_timer =  localtime;
    dhcp_coarse_tmr();
  }
#endif
#if LWIP_DNS
  /* DNS periodic process every 1s */
  if (localtime - dns_timer >= DNS_TMR_INTERVAL)
  {
    dns_timer =  localtime;
    dns_tmr();
  }
#endif
  /* Process dynamic sys_timeout callbacks (DHCP retry, DNS retry, TCP, etc.) */
  lwip_timeout_poll();
}
