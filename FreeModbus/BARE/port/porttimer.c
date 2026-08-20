/*
 * FreeModbus Libary: FreeRTOS Software Timer Port
 */

#include "port.h"
#include "mbport.h"

#include "FreeRTOS.h"
#include "timers.h"
#include "task.h"

/* ----------------------- 软件定时器 ----------------------- */
static TimerHandle_t xT3_5Timer = NULL;
static volatile BaseType_t xTimerExpired = pdFALSE;

/* 定时器回调函数（在 FreeRTOS 定时器服务任务中运行） */
static void
vMBTimerCallback(TimerHandle_t pxTimer)
{
    (void)pxTimer;
    xTimerExpired = pdTRUE;
}

/* ----------------------- 初始化 ----------------------- */
BOOL
xMBPortTimersInit(USHORT usTim1Timerout50us)
{
    (void)usTim1Timerout50us;

    /*
     * 创建单次软件定时器。
     * 周期：1 个 tick（通常 1ms）
     * 实际超时时间由 vMBPortTimersRestart 配合 xTimerChangePeriod 控制。
     */
    xT3_5Timer = xTimerCreate("MB_T3_5",
                              pdMS_TO_TICKS(5),   /* 默认 5ms 超时 */
                              pdFALSE,             /* 单次模式 */
                              NULL,
                              vMBTimerCallback);
    if (xT3_5Timer == NULL) {
        return FALSE;
    }
    return TRUE;
}

/* ----------------------- 使能定时器 ----------------------- */
void
vMBPortTimersEnable(void)
{
    if (xT3_5Timer != NULL) {
        xTimerExpired = pdFALSE;
        xTimerStart(xT3_5Timer, 0);
    }
}

/* ----------------------- 关闭定时器 ----------------------- */
void
vMBPortTimersDisable(void)
{
    if (xT3_5Timer != NULL) {
        xTimerStop(xT3_5Timer, 0);
        xTimerExpired = pdFALSE;
    }
}

/* ----------------------- 复位定时器（每收到一个字节调用） ----------------------- */
void
vMBPortTimersRestart(void)
{
    if (xT3_5Timer != NULL) {
        xTimerExpired = pdFALSE;
        xTimerStart(xT3_5Timer, 0);  /* 重新启动单次定时器 */
    }
}

/* ----------------------- 轮询定时器超时标志 ----------------------- */
/* 必须在 Modbus 任务中周期性调用 */
void
vMBPortTimersPoll(void)
{
    if (xTimerExpired == pdTRUE) {
        xTimerExpired = pdFALSE;
        pxMBPortCBTimerExpired();   /* 通知 FreeModbus 帧结束 */
    }
}

/* Create an ISR which is called whenever the timer has expired. This function
 * must then call pxMBPortCBTimerExpired( ) to notify the protocol stack that
 * the timer has expired.
 */
static void prvvTIMERExpiredISR( void )
{
    ( void )pxMBPortCBTimerExpired(  );
}

