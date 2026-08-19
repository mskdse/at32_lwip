/**
  **************************************************************************
  * @file     msc_diskio.c
  * @brief    usb mass storage disk function
  **************************************************************************
  *
  * Copyright (c) 2025, Artery Technology, All rights reserved.
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
#include "msc_diskio.h"
#include "cdc_msc_class.h"


/** @addtogroup 437_USB_device_cdc_msc
  * @{
  */
uint8_t scsi_inquiry[MSC_SUPPORT_MAX_LUN][SCSI_INQUIRY_DATA_LENGTH] =
{
  /* lun = 0 */
  {
    0x00,         /* peripheral device type (direct-access device) */
    0x80,         /* removable media bit */
    0x00,         /* ansi version, ecma version, iso version */
    0x01,         /* respond data format */
    SCSI_INQUIRY_DATA_LENGTH - 5, /* additional length */
    0x00, 0x00, 0x00, /* reserved */
    'A', 'T', '3', '2', ' ', ' ', ' ', ' ', /* vendor information "AT32" */
    'D', 'i', 's', 'k', '0', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', /* Product identification "Disk" */
    '2', '.', '0', '0'  /* product revision level */
  }
};

/**
  * @brief  get disk inquiry
  * @param  lun: logical units number
  * @retval inquiry string
  */
uint8_t *get_inquiry(uint8_t lun)
{
  if(lun < MSC_SUPPORT_MAX_LUN)
    return (uint8_t *)scsi_inquiry[lun];
  else
    return NULL;
}

/**
  * @brief  disk read
  * @param  lun: logical units number
  * @param  addr: logical address
  * @param  read_buf: pointer to read buffer
  * @param  len: read length
  * @retval status of usb_sts_type
  */
usb_sts_type msc_disk_read(uint8_t lun, uint64_t addr, uint8_t *read_buf, uint32_t len)
{
  switch(lun)
  {
    case SPI_FLASH_LUN:
		{
			w25x80_read_data(addr,read_buf,len);
		}break;
    case SD_LUN:
      break;
    default:
      break;
  }
  return USB_OK;
}

/**
  * @brief  disk write
  * @param  lun: logical units number
  * @param  addr: logical address
  * @param  buf: pointer to write buffer
  * @param  len: write length
  * @retval status of usb_sts_type
  */
usb_sts_type msc_disk_write(uint8_t lun, uint64_t addr, uint8_t *buf, uint32_t len)
{
  switch(lun)
  {
    case SPI_FLASH_LUN:
		{
			uint32_t sector;
			uint32_t count;

			sector = addr / 4096;
			count = (len + 4095) / 4096;

			for(uint32_t i=0;i<count;i++)
			{
				w25x80_sector_erase(sector * 4096);
				w25x80_write_data(sector * 4096,buf,4096);
				sector++;
				buf += 4096;
			}
		}break;
    case SD_LUN:
      break;
    default:
      break;;
  }
  return USB_OK;
}

/**
  * @brief  disk capacity
  * @param  lun: logical units number
  * @param  blk_nbr: pointer to number of block
  * @param  blk_size: pointer to block size
  * @retval status of usb_sts_type
  */
usb_sts_type msc_disk_capacity(uint8_t lun, uint32_t *blk_nbr, uint32_t *blk_size)
{
  switch(lun)
  {
    case SPI_FLASH_LUN:
		{
			*blk_nbr =   256;
      *blk_size = 4096;
		}break;
    case SD_LUN:
      break;
    default:
      break;
  }
  return USB_OK;
}

/**
  * @}
  */

/**
  * @}
  */
