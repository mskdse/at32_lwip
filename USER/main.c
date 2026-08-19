#include "at32f435_437.h"
#include "at32f435_437_clock.h"
#include "debug_usart.h"
#include "w25x80.h"


#include "FreeRTOS.h"
#include "task.h"
/* 防止打印资源抢占 */
#define RTOS_PRINTF(x)  do { \
    taskENTER_CRITICAL(); \
    printf x; \
    taskEXIT_CRITICAL(); \
} while(0)

#include "at32_emac_phy.h"
#include "lwip/dns.h"
#include "lwip/netif.h"


#include "ff.h"


#include "usb_conf.h"
#include "usb_core.h"
#include "usbd_int.h"
#include "cdc_msc_class.h"
#include "cdc_msc_desc.h"
/* usb global struct define */
otg_core_type otg_core_struct;
#if defined ( __ICCARM__ ) /* iar compiler */
  #pragma data_alignment=4
#endif
ALIGNED_HEAD uint8_t usb_buffer[256] ALIGNED_TAIL;
void usb_clock48m_select(usb_clk48_s clk_s);
void usb_gpio_config(void);
void usb_low_power_wakeup_config(void);


FATFS g_file_system;
FRESULT res;


extern void tcpip_stack_init(void);
extern void lwip_pkt_handle(void);
extern void lwip_periodic_handle(volatile uint32_t localtime);
extern struct netif netif;

TaskHandle_t network_handler;
TaskHandle_t dns_handler;
TaskHandle_t usb_handler;

static void network_task_function(void *pvParameters);
static void dns_task_function(void *pvParameters);
static void usb_task_function(void *pvParameters);

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
    RTOS_PRINTF(("DNS: %s -> %s\r\n", name, ipaddr_ntoa(ipaddr)));
  }
  else
  {
    RTOS_PRINTF(("DNS: %s resolve failed\r\n", name));
  }
  dns_done = 1;
}

int main(void)
{
  nvic_priority_group_config(NVIC_PRIORITY_GROUP_4);
  system_clock_config();
  debug_uart_print_init(115200);
	
/* 一开始默认SRAM设置只有128KB，排查了老半天，不过这些代码依旧不行，需要使用专门的AT-ICP工具修改配置 */
//	flash_unlock();
//	printf("eopb0 set%d\r\n",flash_eopb0_config(FLASH_EOPB0_SRAM_512K));
//	flash_lock();
//  printf("eopb0=%d\r\n",USD->eopb0);


		/* 挂载 */
		res = f_mount(&g_file_system, "0:", 1);
		RTOS_PRINTF(("f_mount->res=%d\r\n", res));
	

  taskENTER_CRITICAL(); 
  if(xTaskCreate((TaskFunction_t)network_task_function, "Network_task",
                 512, NULL, 3, &network_handler) != pdPASS)
  {
    RTOS_PRINTF(("Network task could not be created.\r\n"));
  }
  else
  {
    RTOS_PRINTF(("Network task was created successfully.\r\n"));
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
	
	/* Create DNS test task after stack is ready */
  xTaskCreate((TaskFunction_t)usb_task_function, "USB_task",
              512, NULL, 3, &usb_handler);

  for(;;)
  {
    while(emac_received_packet_size_get() != 0) lwip_pkt_handle();
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
  RTOS_PRINTF(("DNS task: waiting for IP...\r\n"));
  while (netif.ip_addr.addr == 0) vTaskDelay(500);
  RTOS_PRINTF(("DNS task: got IP %s, resolving www.baidu.com...\r\n",ipaddr_ntoa(&netif.ip_addr)));

  dns_done = 0;
  err_t err = dns_gethostbyname("www.baidu.com", &dns_result,dns_found, NULL);

  if (err == ERR_OK)               RTOS_PRINTF(("DNS: www.baidu.com -> %s (cached)\r\n", ipaddr_ntoa(&dns_result)));
  else if (err == ERR_INPROGRESS)  RTOS_PRINTF(("DNS: request queued, waiting...\r\n"));
  else                             RTOS_PRINTF(("DNS: error %d\r\n", err));

  /* Let lwIP process until DNS completes or timeout */
  for (int tick = 0; tick < 500 && !dns_done; tick++) vTaskDelay(100);

  if (!dns_done)                                      RTOS_PRINTF(("DNS: timeout\r\n"));

  vTaskDelete(NULL);
}

static void usb_task_function(void *pvParameters)
{
	uint16_t data_len;

  uint32_t timeout;
	
	uint8_t send_zero_packet = 0;
	
	 /* usb gpio config */
  usb_gpio_config();

#ifdef USB_LOW_POWER_WAKUP
  usb_low_power_wakeup_config();
#endif

  /* enable otgfs clock */
  crm_periph_clock_enable(OTG_CLOCK, TRUE);

  /* select usb 48m clcok source */
  usb_clock48m_select(USB_CLK_HEXT);

  /* enable otgfs irq */
  nvic_irq_enable(OTG_IRQ, 0, 0);

  /* init usb */
  usbd_init(&otg_core_struct,
            USB_FULL_SPEED_CORE_ID,
            USB_ID,
            &cdc_msc_class_handler,
            &cdc_msc_desc_handler);
	for(;;)
	{
		/* get usb vcp receive data */
    data_len = usb_vcp_get_rxdata(&otg_core_struct.dev, usb_buffer);

    if(data_len > 0 || send_zero_packet == 1)
    {

      /* bulk transfer is complete when the endpoint does one of the following
         1 has transferred exactly the amount of data expected
         2 transfers a packet with a payload size less than wMaxPacketSize or transfers a zero-length packet
      */
      if(data_len > 0)
        send_zero_packet = 1;

      if(data_len == 0)
        send_zero_packet = 0;

      timeout = 5000000;
      do
      {
        /* send data to host */
        if(usb_vcp_send_data(&otg_core_struct.dev, usb_buffer, data_len) == SUCCESS)
        {
          break;
        }
      }while(timeout --);
    }
	}
}

/**
  * @brief  usb 48M clock select
  * @param  clk_s:USB_CLK_HICK, USB_CLK_HEXT
  * @retval none
  */
static void usb_clock48m_select(usb_clk48_s clk_s)
{
  crm_clocks_freq_type clocks_struct;
  
  if(clk_s == USB_CLK_HICK)
  {
    crm_usb_clock_source_select(CRM_USB_CLOCK_SOURCE_HICK);

    /* enable the acc calibration ready interrupt */
    crm_periph_clock_enable(CRM_ACC_PERIPH_CLOCK, TRUE);

    /* update the c1\c2\c3 value */
    acc_write_c1(7980);
    acc_write_c2(8000);
    acc_write_c3(8020);
#if (USB_ID == 0)
    acc_sof_select(ACC_SOF_OTG1);
#else
    acc_sof_select(ACC_SOF_OTG2);
#endif
    /* open acc calibration */
    acc_calibration_mode_enable(ACC_CAL_HICKTRIM, TRUE);
  }
  else
  {
    crm_clocks_freq_get(&clocks_struct);
    switch(clocks_struct.sclk_freq)
    {
      /* 48MHz */
      case 48000000:
        crm_usb_clock_div_set(CRM_USB_DIV_1);
        break;

      /* 72MHz */
      case 72000000:
        crm_usb_clock_div_set(CRM_USB_DIV_1_5);
        break;

      /* 96MHz */
      case 96000000:
        crm_usb_clock_div_set(CRM_USB_DIV_2);
        break;

      /* 120MHz */
      case 120000000:
        crm_usb_clock_div_set(CRM_USB_DIV_2_5);
        break;

      /* 144MHz */
      case 144000000:
        crm_usb_clock_div_set(CRM_USB_DIV_3);
        break;

      /* 168MHz */
      case 168000000:
        crm_usb_clock_div_set(CRM_USB_DIV_3_5);
        break;

      /* 192MHz */
      case 192000000:
        crm_usb_clock_div_set(CRM_USB_DIV_4);
        break;

      /* 216MHz */
      case 216000000:
        crm_usb_clock_div_set(CRM_USB_DIV_4_5);
        break;

      /* 240MHz */
      case 240000000:
        crm_usb_clock_div_set(CRM_USB_DIV_5);
        break;

      /* 264MHz */
      case 264000000:
        crm_usb_clock_div_set(CRM_USB_DIV_5_5);
        break;

      /* 288MHz */
      case 288000000:
        crm_usb_clock_div_set(CRM_USB_DIV_6);
        break;

      default:
        break;

    }
  }
}

/**
  * @brief  this function config gpio.
  * @param  none
  * @retval none
  */
static void usb_gpio_config(void)
{
  gpio_init_type gpio_init_struct;

  crm_periph_clock_enable(OTG_PIN_GPIO_CLOCK, TRUE);
  gpio_default_para_init(&gpio_init_struct);

  gpio_init_struct.gpio_drive_strength = GPIO_DRIVE_STRENGTH_STRONGER;
  gpio_init_struct.gpio_out_type  = GPIO_OUTPUT_PUSH_PULL;
  gpio_init_struct.gpio_mode = GPIO_MODE_MUX;
  gpio_init_struct.gpio_pull = GPIO_PULL_NONE;

  /* dp and dm */
  gpio_init_struct.gpio_pins = OTG_PIN_DP|OTG_PIN_DM;
  gpio_init(OTG_PIN_GPIO, &gpio_init_struct);

  gpio_pin_mux_config(OTG_PIN_GPIO, OTG_PIN_DP_SOURCE, OTG_PIN_MUX);
  gpio_pin_mux_config(OTG_PIN_GPIO, OTG_PIN_DM_SOURCE, OTG_PIN_MUX);

#ifdef USB_SOF_OUTPUT_ENABLE
  crm_periph_clock_enable(OTG_PIN_SOF_GPIO_CLOCK, TRUE);
  gpio_init_struct.gpio_pins = OTG_PIN_SOF;
  gpio_init(OTG_PIN_SOF_GPIO, &gpio_init_struct);
  gpio_pin_mux_config(OTG_PIN_SOF_GPIO, OTG_PIN_SOF_SOURCE, OTG_PIN_MUX);
#endif

  /* otgfs use vbus pin */
#ifndef USB_VBUS_IGNORE
  gpio_init_struct.gpio_pins = OTG_PIN_VBUS;
  gpio_init_struct.gpio_pull = GPIO_PULL_DOWN;
  gpio_pin_mux_config(OTG_PIN_GPIO, OTG_PIN_VBUS_SOURCE, OTG_PIN_MUX);
  gpio_init(OTG_PIN_GPIO, &gpio_init_struct);
#endif


}
#ifdef USB_LOW_POWER_WAKUP
/**
  * @brief  usb low power wakeup interrupt config
  * @param  none
  * @retval none
  */
static void usb_low_power_wakeup_config(void)
{
  exint_init_type exint_init_struct;

  crm_periph_clock_enable(CRM_SCFG_PERIPH_CLOCK, TRUE);
  exint_default_para_init(&exint_init_struct);

  exint_init_struct.line_enable = TRUE;
  exint_init_struct.line_mode = EXINT_LINE_INTERRUPT;
  exint_init_struct.line_select = OTG_WKUP_EXINT_LINE;
  exint_init_struct.line_polarity = EXINT_TRIGGER_RISING_EDGE;
  exint_init(&exint_init_struct);

  nvic_irq_enable(OTG_WKUP_IRQ, 0, 0);
}

/**
  * @brief  this function handles otgfs wakup interrupt.
  * @param  none
  * @retval none
  */
static void OTG_WKUP_HANDLER(void)
{
  exint_flag_clear(OTG_WKUP_EXINT_LINE);
}

#endif

/**
  * @brief  this function handles otgfs interrupt.
  * @param  none
  * @retval none
  */
void OTG_IRQ_HANDLER(void)
{
  usbd_irq_handler(&otg_core_struct);
}

/**
  * @brief  usb delay millisecond function.
  * @param  ms: number of millisecond delay
  * @retval none
  */
void usb_delay_ms(uint32_t ms)
{
  /* user can define self delay function */
  vTaskDelay(ms);
}
