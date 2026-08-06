#ifndef _DEBUG_USART_H_
#define _DEBUG_USART_H_

#include "at32f435_437.h"
#include <stdio.h>

/**************** define print uart ******************/
#define PRINT_UART                       USART2
#define PRINT_UART_CRM_CLK               CRM_USART2_PERIPH_CLOCK
#define PRINT_UART_TX_PIN                GPIO_PINS_5
#define PRINT_UART_TX_GPIO               GPIOD
#define PRINT_UART_TX_GPIO_CRM_CLK       CRM_GPIOD_PERIPH_CLOCK
#define PRINT_UART_TX_PIN_SOURCE         GPIO_PINS_SOURCE5
#define PRINT_UART_TX_PIN_MUX_NUM        GPIO_MUX_7

void debug_uart_print_init(uint32_t baudrate);

#endif
