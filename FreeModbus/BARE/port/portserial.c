/*
 * FreeModbus Libary: AT32F437 USB CDC Port
 * Copyright (C) 2025
 */
 
/*
 * 先包含 AT32 库头文件（此时 TRUE/FALSE 未被定义为宏）
 */
#include "at32f435_437.h"

#include "port.h"
#include "mb.h"
#include "mbport.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* USB CDC 驱动头文件（根据你的工程路径调整） */
#include "usb_conf.h"
#include "usb_core.h"
#include "usbd_int.h"
#include "cdc_msc_class.h"
#include "cdc_msc_desc.h"

/* usb global struct define */
extern otg_core_type otg_core_struct;

/* ----------------------- 缓冲区定义 ----------------------- */
#define MB_TX_BUF_SIZE       256
#define MB_RX_BUF_SIZE       1024

static uint8_t mb_tx_buf[MB_TX_BUF_SIZE];
static volatile uint16_t mb_tx_len = 0;

static uint8_t mb_rx_buf[MB_RX_BUF_SIZE];
static volatile uint16_t mb_rx_head = 0;
static volatile uint16_t mb_rx_tail = 0;

/* ----------------------- 互斥锁 ----------------------- */
static SemaphoreHandle_t xTxMutex = NULL;
static SemaphoreHandle_t xRxMutex = NULL;

/* 发送完成标志（USB 发送完成后置位） */
static volatile BaseType_t xTxComplete = pdTRUE;

/* ----------------------- 初始化 ----------------------- */
BOOL
xMBPortSerialInit(UCHAR ucPORT, ULONG ulBaudRate, UCHAR ucDataBits,
                  eMBParity eParity, UCHAR ucStopBits)
{
    (void)ucPORT;
    (void)ulBaudRate;
    (void)ucDataBits;
    (void)eParity;
    (void)ucStopBits;

    mb_tx_len = 0;
    mb_rx_head = 0;
    mb_rx_tail = 0;

    if (xTxMutex == NULL) {
        xTxMutex = xSemaphoreCreateMutex();
    }
    if (xRxMutex == NULL) {
        xRxMutex = xSemaphoreCreateMutex();
    }

    return (xTxMutex != NULL && xRxMutex != NULL) ? TRUE : FALSE;
}

/* ----------------------- 使能/关闭收发 ----------------------- */
void
vMBPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    if (xTxEnable) {
        /* 准备发送：清空发送缓冲区 */
        taskENTER_CRITICAL();
        mb_tx_len = 0;
        taskEXIT_CRITICAL();
    } else {
        /* 发送结束：通过 USB CDC 发出所有字节 */
        uint16_t len;
        taskENTER_CRITICAL();
        len = mb_tx_len;
        taskEXIT_CRITICAL();

        if (len > 0) {
            /* 使用互斥锁保护 USB 发送 */
            if (xSemaphoreTake(xTxMutex, portMAX_DELAY) == pdTRUE) {
                uint32_t timeout = 1000000;
                xTxComplete = pdFALSE;
                while (usb_vcp_send_data(&otg_core_struct.dev, mb_tx_buf, len) != SUCCESS) {
                    if (--timeout == 0) break;
                }
                xTxComplete = pdTRUE;
                xSemaphoreGive(xTxMutex);
            }
            taskENTER_CRITICAL();
            mb_tx_len = 0;
            taskEXIT_CRITICAL();
        }

        /* 通知协议栈发送完成 */
        pxMBFrameCBTransmitterEmpty();
    }
}

/* ----------------------- 发送一个字节 ----------------------- */
BOOL
xMBPortSerialPutByte(CHAR ucByte)
{
    BOOL result = FALSE;
    taskENTER_CRITICAL();
    if (mb_tx_len < MB_TX_BUF_SIZE) {
        mb_tx_buf[mb_tx_len++] = (uint8_t)ucByte;
        result = TRUE;
    }
    taskEXIT_CRITICAL();
    return result;
}

/* ----------------------- 接收一个字节 ----------------------- */
BOOL
xMBPortSerialGetByte(CHAR *pucByte)
{
    BOOL result = FALSE;
    taskENTER_CRITICAL();
    if (mb_rx_head != mb_rx_tail) {
        *pucByte = (CHAR)mb_rx_buf[mb_rx_tail];
        mb_rx_tail = (mb_rx_tail + 1) % MB_RX_BUF_SIZE;
        result = TRUE;
    }
    taskEXIT_CRITICAL();
    return result;
}

/* ----------------------- 供 USB 任务调用的接口 ----------------------- */

/**
 * @brief 将 USB CDC 收到的数据放入 Modbus 接收缓冲区
 * @param data: 数据指针
 * @param len:  数据长度
 * @return 实际放入的字节数
 */
uint16_t
mb_usb_rx_data(uint8_t *data, uint16_t len)
{
    uint16_t count = 0;
    taskENTER_CRITICAL();
    for (uint16_t i = 0; i < len; i++) {
        uint16_t next = (mb_rx_head + 1) % MB_RX_BUF_SIZE;
        if (next != mb_rx_tail) {
            mb_rx_buf[mb_rx_head] = data[i];
            mb_rx_head = next;
            count++;
        } else {
            break;  /* 缓冲区满 */
        }
    }
    taskEXIT_CRITICAL();
    return count;
}

/**
 * @brief 检查接收缓冲区是否有数据
 */
BOOL
mb_usb_rx_available(void)
{
    return (mb_rx_head != mb_rx_tail) ? TRUE : FALSE;
}

/* Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
static void prvvUARTTxReadyISR( void )
{
    pxMBFrameCBTransmitterEmpty(  );
}

/* Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
static void prvvUARTRxISR( void )
{
    pxMBFrameCBByteReceived(  );
}

void __aeabi_assert(const char *expr, const char *file, int line)
{
    (void)expr;
    (void)file;
    (void)line;
    while(1);  // 或者触发断点
}
