/*
 * Copyright (c) 2017 Simon Goldschmidt
 * All rights reserved. 
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT 
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 * 
 * Author: Simon Goldschmidt
 *
 */


#include <lwip/opt.h>
#include <lwip/arch.h>
#if !NO_SYS
#include "sys_arch.h"
#endif
#include <lwip/stats.h>
#include <lwip/debug.h>
#include <lwip/sys.h>

#include <string.h>
#if (__ARMCC_VERSION > 6000000)
#include "cmsis_armclang.h"
#else

#include "cmsis_armcc.h"
#endif
static int inHandlerMode (void);
/* Determine whether we are in thread mode or handler mode. */
static int inHandlerMode (void)
{
  return __get_IPSR() != 0;
}


#define SYS_THREAD_MAX 6

static u16_t s_nextthread = 0;

u32_t lwip_sys_now;

u32_t
sys_jiffies(void)
{
  return lwip_sys_now;
}


u32_t
sys_now(void)
{
  return  xTaskGetTickCount();
}
SemaphoreHandle_t lwip_sys_mutex;
void
sys_init(void)
{
	lwip_sys_mutex = xSemaphoreCreateMutex();
}

#if !NO_SYS

test_sys_arch_waiting_fn the_waiting_fn;

void
test_sys_arch_wait_callback(test_sys_arch_waiting_fn waiting_fn)
{
  the_waiting_fn = waiting_fn;
}

err_t
sys_sem_new(sys_sem_t *sem, u8_t count)
{
  *sem = xSemaphoreCreateBinary();/* 创建二值信号量 */
  
  if(*sem == NULL)
  {
    return ERR_MEM;
  }
  if(count == 0)
  {
    xSemaphoreTake(*sem,0);/* 获取信号量 */
  }
  else
  {
    xSemaphoreGive(*sem);/* 释放信号量 */
  }
  return ERR_OK;
}

void
sys_sem_free(sys_sem_t *sem)
{
  vSemaphoreDelete(*sem);/* 删除信号量 */
}

void
sys_sem_set_invalid(sys_sem_t *sem)
{
  *sem = NULL;    
}

/* semaphores are 1-based because RAM is initialized as 0, which would be valid */
u32_t
sys_arch_sem_wait(sys_sem_t *sem, u32_t timeout)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
  portTickType StartTime,Endtime,Elapsed;
	portTickType return_val;	
  StartTime = xTaskGetTickCount();

  if(timeout != 0 )/* 有超时时间设定 */
  {
		if(inHandlerMode())
		{
			if(xSemaphoreTakeFromISR(*sem,&xHigherPriorityTaskWoken) == pdTRUE)
			{
				Endtime = xTaskGetTickCount();
				Elapsed = (Endtime - StartTime) * portTICK_RATE_MS;
				return_val = Elapsed;
			}
			else
			{
				return_val = SYS_ARCH_TIMEOUT;
			}
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
		else
		{
			if(xSemaphoreTake(*sem,timeout/portTICK_RATE_MS) == pdTRUE)
			{
				Endtime = xTaskGetTickCount();
				Elapsed = (Endtime - StartTime) * portTICK_RATE_MS;
				return_val = Elapsed;
			}
			else
			{
				return_val = SYS_ARCH_TIMEOUT;
			}
	  }
  }
  else/* 无超时时间,死等 */
  {
		if(inHandlerMode())
		{
			xSemaphoreTakeFromISR(*sem,&xHigherPriorityTaskWoken);
			Endtime = xTaskGetTickCount();
			Elapsed = (Endtime - StartTime) * portTICK_RATE_MS;
			return_val = Elapsed;
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
		else
		{
			xSemaphoreTake(*sem,portMAX_DELAY);
			Endtime = xTaskGetTickCount();
			Elapsed = (Endtime - StartTime) * portTICK_RATE_MS;
			return_val = Elapsed;
		}
  }
	return return_val;
}

void
sys_sem_signal(sys_sem_t *sem)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	if(inHandlerMode())
	{
		xSemaphoreGiveFromISR(*sem,&xHigherPriorityTaskWoken);/* 释放信号量 */
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
	{
		xSemaphoreGive(*sem);/* 释放信号量 */
	}
}

int sys_sem_valid(sys_sem_t *sem)
{
  if (*sem == NULL)
    return 0;
  else
    return 1;
}
  
err_t
sys_mutex_new(sys_mutex_t *mutex)
{ 
	*mutex = xSemaphoreCreateMutex();
	
	if(*mutex == NULL)
  {
		return ERR_MEM;
	}
  return ERR_OK;
}

void
sys_mutex_free(sys_mutex_t *mutex)
{
  /* parameter check */
  vQueueDelete(*mutex);
}

void
sys_mutex_set_invalid(sys_mutex_t *mutex)
{ 
}

void
sys_mutex_lock(sys_mutex_t *mutex)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
  if(inHandlerMode())
	{
		xSemaphoreTakeFromISR(*mutex, &xHigherPriorityTaskWoken);
	  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
	{
		xSemaphoreTake(*mutex, portMAX_DELAY);
	}
}

void
sys_mutex_unlock(sys_mutex_t *mutex)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
  if(inHandlerMode())
	{
		xSemaphoreGiveFromISR(*mutex, &xHigherPriorityTaskWoken);
	  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
	{
		xSemaphoreGive(*mutex);
	}
}


sys_thread_t
sys_thread_new(const char *name, lwip_thread_fn function, void *arg, int stacksize, int prio)
{
  xTaskHandle network_task;
  int result;
  if(s_nextthread < SYS_THREAD_MAX)
    result = xTaskCreate(function,(portCHAR *)name,stacksize,arg,prio,&network_task);
  if(result == pdPASS)
    return network_task;
  else
    return NULL;
}

err_t
sys_mbox_new(sys_mbox_t *mbox, int size)
{
  *mbox = xQueueCreate(size,sizeof(void *));
	
	if (*mbox == NULL)
  return ERR_MEM;
	
  return ERR_OK;
}

void
sys_mbox_free(sys_mbox_t *mbox)
{
	if(inHandlerMode())
	{
		if(uxQueueMessagesWaitingFromISR(*mbox))
		{
			portNOP();
		}
	}
	else
	{
		if(uxQueueMessagesWaiting(*mbox))
		{
			portNOP();
		}
  }
  vQueueDelete(*mbox);
}



void
sys_mbox_post(sys_mbox_t *q, void *msg)
{
	portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
	
	if(inHandlerMode())
	{
		while(xQueueSendToBackFromISR(*q,&msg, &xHigherPriorityTaskWoken) != pdTRUE);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
	{
		while(xQueueSendToBack(*q,&msg, portMAX_DELAY) != pdTRUE);
	}
}

err_t
sys_mbox_trypost(sys_mbox_t *q, void *msg)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	err_t state;
	if(inHandlerMode())
	{
		if(xQueueSendFromISR(*q,&msg,&xHigherPriorityTaskWoken) == pdPASS)
			state = ERR_OK;
		else
			state = ERR_MEM;
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
  }
	else
	{
		if(xQueueSend(*q,&msg,portMAX_DELAY) == pdPASS)
			state = ERR_OK;
		else
			state = ERR_MEM;
	}
	return state;
}

err_t
sys_mbox_trypost_fromisr(sys_mbox_t *q, void *msg)
{
  return sys_mbox_trypost(q, msg);
}

u32_t
sys_arch_mbox_fetch(sys_mbox_t *q, void **msg, u32_t timeout)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  portTickType StartTime,Endtime,Elapsed;
	portTickType return_val;
  void *dummuyptr;
  StartTime = xTaskGetTickCount();
  if(msg == NULL)
    msg = &dummuyptr;
	
	if(timeout != 0)
	{
		if(inHandlerMode())
		{
			if(pdTRUE == xQueueReceiveFromISR(*q,&(*msg),&xHigherPriorityTaskWoken))
			{
				Endtime = xTaskGetTickCount();
				Elapsed = (Endtime - StartTime)*portTICK_RATE_MS;
				return_val = Elapsed;
			}
			else
			{
				*msg = NULL;
				return_val = SYS_ARCH_TIMEOUT;
			}
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
		else
		{
			if(pdTRUE == xQueueReceive(*q,&(*msg),timeout/portTICK_RATE_MS))
			{
				Endtime = xTaskGetTickCount();
				Elapsed = (Endtime - StartTime)*portTICK_RATE_MS;
				return_val = Elapsed;
			}
			else
			{
				*msg = NULL;
				return_val = SYS_ARCH_TIMEOUT;
			}
		}
	}
	else
	{
		if(inHandlerMode())
		{
			while(pdTRUE == xQueueReceiveFromISR(*q,&(*msg),&xHigherPriorityTaskWoken)){};
			Endtime = xTaskGetTickCount();
			Elapsed = (Endtime - StartTime)*portTICK_RATE_MS;
			return_val = Elapsed;
		  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
		}
    else
		{
			while(pdTRUE != xQueueReceive(*q,&(*msg),portMAX_DELAY)){};
			Endtime = xTaskGetTickCount();
			Elapsed = (Endtime - StartTime)*portTICK_RATE_MS;
			return_val = Elapsed;
		}			
	}
	
	return return_val;
}

u32_t
sys_arch_mbox_tryfetch(sys_mbox_t *q, void **msg)
{
	u32_t state;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  void *dummuyptr;
  if(msg == NULL)
    msg = &dummuyptr;

	if(inHandlerMode())
	{
		if(pdTRUE == xQueueReceiveFromISR(*q,&(*msg),&xHigherPriorityTaskWoken))
		{
			state = ERR_OK;
		}
		else
		{
			state = SYS_MBOX_EMPTY;
		}
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
	else
	{
		if(pdTRUE == xQueueReceive(*q,&(*msg),0))
		{
			state = ERR_OK;
		}
		else
		{
			state = SYS_MBOX_EMPTY;
		}
	}
	return state;
}

sys_prot_t sys_arch_protect(void)
{
	portDISABLE_INTERRUPTS();
	return 0;
}

void sys_arch_unprotect(sys_prot_t pval)
{
	portENABLE_INTERRUPTS();
}

void sys_assert(const char *msg)
{
  (void)msg;
  vPortEnterCritical();
  for(;;);
}

int sys_mbox_valid(sys_mbox_t *mbox)
{
   //return ERR_OK;
	if(*mbox == NULL) {
		return 0;
	}
	else {
		return 1;
	}
}

void
sys_mbox_set_invalid(sys_mbox_t *mbox)
{
	*mbox = NULL;
}

#if LWIP_NETCONN_SEM_PER_THREAD
#error LWIP_NETCONN_SEM_PER_THREAD==1 not supported
#endif /* LWIP_NETCONN_SEM_PER_THREAD */

#endif /* !NO_SYS */

