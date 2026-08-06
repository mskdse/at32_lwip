#include "at32f435_437.h"
#include "at32f435_437_clock.h"
#include "debug_usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "at32_emac_phy.h"
#include "lwip/dns.h"
#include "lwip/netif.h"

extern void tcpip_stack_init(void);
extern void lwip_pkt_handle(void);
extern void lwip_periodic_handle(volatile uint32_t localtime);
extern struct netif netif;

TaskHandle_t network_handler;
TaskHandle_t dns_handler;

static void network_task_function(void *pvParameters);
static void dns_task_function(void *pvParameters);

/* DNS resolve result */
static volatile int dns_done = 0;
static ip_addr_t dns_result;

/**
  * @brief  DNS found callback - called by lwIP when resolution completes.
  */
static void dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
  (void)arg;
  if (ipaddr != NULL)
  {
    dns_result = *ipaddr;
    printf("DNS: %s -> %s\r\n", name, ipaddr_ntoa(ipaddr));
  }
  else
  {
    printf("DNS: %s resolve failed\r\n", name);
  }
  dns_done = 1;
}

int main(void)
{
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
  system_clock_config();
  debug_uart_print_init(115200);
  
  taskENTER_CRITICAL(); 
  if(xTaskCreate((TaskFunction_t)network_task_function, "Network_task",
                 512, NULL, 2, &network_handler) != pdPASS)
  {
    printf("Network task could not be created.\r\n");
  }
  else
  {
    printf("Network task was created successfully.\r\n");
  }
  taskEXIT_CRITICAL();
  vTaskStartScheduler(); 
  for(;;);
}

static void network_task_function(void *pvParameters)
{
  while(emac_system_init() == ERROR);
  tcpip_stack_init();

  /* Create DNS test task after stack is ready */
  xTaskCreate((TaskFunction_t)dns_task_function, "DNS_task",
              512, NULL, 3, &dns_handler);

  for(;;)
  {
    while(emac_received_packet_size_get() != 0)
    {
      lwip_pkt_handle();
    }
    lwip_periodic_handle(xTaskGetTickCount());
    vTaskDelay(10);
  }
}

/**
  * @brief  DNS test task - waits for IP, then resolves www.baidu.com
  */
static void dns_task_function(void *pvParameters)
{
  (void)pvParameters;

  /* Wait for DHCP to obtain an IP address */
  printf("DNS task: waiting for IP...\r\n");
  while (netif.ip_addr.addr == 0)
  {
    vTaskDelay(500);
  }
  printf("DNS task: got IP %s, resolving www.baidu.com...\r\n",
         ipaddr_ntoa(&netif.ip_addr));

  dns_done = 0;
  err_t err = dns_gethostbyname("www.baidu.com", &dns_result,
                                 dns_found, NULL);

  if (err == ERR_OK)
  {
    /* Cached / immediate result */
    printf("DNS: www.baidu.com -> %s (cached)\r\n", ipaddr_ntoa(&dns_result));
  }
  else if (err == ERR_INPROGRESS)
  {
    printf("DNS: request queued, waiting...\r\n");
  }
  else
  {
    printf("DNS: error %d\r\n", err);
  }

  /* Let lwIP process until DNS completes or timeout */
  for (int tick = 0; tick < 500 && !dns_done; tick++)
  {
    vTaskDelay(100);
  }

  if (!dns_done)
  {
    printf("DNS: timeout\r\n");
  }

  vTaskDelete(NULL);
}
