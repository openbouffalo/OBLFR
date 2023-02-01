/*
kved (key/value embedded database), a simple key/value database
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <bflb_flash.h>

#include "oblfr_kved_flash.h"


#define DBG_TAG "KVED_FLASH"
#include "log.h"

uint32_t oblfr_kved_flash_sector_size(kved_flash_sector_t sec, void *drv_arg);

static uint32_t get_sector_addr(kved_flash_sector_t sec, uint16_t index, void *drv_arg) {
	uint32_t addr = ((oblfr_kved_flash_driver_t *)drv_arg)->flash_addr;
	uint32_t flash_sector_size = ((oblfr_kved_flash_driver_t *)drv_arg)->flash_sector_size;
	uint32_t start_addr = ((addr) & (~(flash_sector_size - 1)));
	uint8_t sectors_required = 0;
	/* all fall through in switch statement */
	switch (sec) {
		case KVED_FLASH_STRING_SECTOR_B:
			/* calc the size of FLASH_STR_SECTOR_A to figure out the start of string sector b*/
			sectors_required += (oblfr_kved_flash_sector_size(KVED_FLASH_STRING_SECTOR_A, drv_arg) / flash_sector_size) + 1;
		case KVED_FLASH_STRING_SECTOR_A:
			/* calc the size of FLASH_SECTOR B to figure out the start of String Sector A*/
			sectors_required += (oblfr_kved_flash_sector_size(KVED_FLASH_SECTOR_B, drv_arg) / flash_sector_size) + 1;
		case KVED_FLASH_SECTOR_B:
			/* calc the size of FLASH_SECTOR A to figure out the start of Sector B*/
			sectors_required += (oblfr_kved_flash_sector_size(KVED_FLASH_SECTOR_A, drv_arg) / flash_sector_size) + 1;
		case KVED_FLASH_SECTOR_A:
			/* flash sector A starts at start_addr */
			break;
		case KVED_FLASH_NUM_SECTORS:
			break;
	}
	//LOG_D("Sectors required for %d is %d\r\n", sec, sectors_required);
	//LOG_D("Final Start Address %x\r\n", start_addr + (sectors_required * flash_sector_size));
	assert(start_addr + (sectors_required * flash_sector_size) + (sizeof(kved_word_t) * index) != 0);
	return start_addr + (sectors_required * flash_sector_size) + (sizeof(kved_word_t) * index);
}

bool oblfr_kved_flash_sector_erase(kved_flash_sector_t sec, void *drv_arg)
{
	uint32_t addr = get_sector_addr(sec, 0, drv_arg);

	if (bflb_flash_erase(addr, oblfr_kved_flash_sector_size(sec, drv_arg)) != 0) {
		LOG_E("Erase Sector %d Failed\r\n", sec);
		return false;
	}
	return true;
}

void oblfr_kved_flash_header_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data, void *drv_arg)
{
	uint32_t addr = get_sector_addr(sec, index, drv_arg);
	if (bflb_flash_write(addr, (uint8_t*)&data, sizeof(kved_word_t)) != 0) {
		LOG_E("Write Sector %d Failed\r\n", sec);
		return;
	}
}

kved_word_t oblfr_kved_flash_header_read(kved_flash_sector_t sec, uint16_t index, void *drv_arg)
{
	uint32_t addr = get_sector_addr(sec, index, drv_arg);
	kved_word_t data;
	if (bflb_flash_read(addr, (uint8_t*)&data, sizeof(kved_word_t)) != 0) {
		LOG_E("Read Sector %d Failed\r\n", sec);
		return 0;
	}
	return data;
}

void oblfr_kved_flash_data_write(kved_flash_sector_t sec, uint16_t index, void *data, uint16_t len, void *drv_arg)
{
	uint32_t addr = get_sector_addr(sec, index, drv_arg);
	if (bflb_flash_write(addr, (uint8_t*)data, len) != 0) {
		LOG_E("Write Sector %d Failed\r\n", sec);
		return;
	}
}

void oblfr_kved_flash_data_read(kved_flash_sector_t sec, uint16_t index, void *data, uint16_t len, void *drv_arg)
{
	uint32_t addr = get_sector_addr(sec, index, drv_arg);
	if (bflb_flash_read(addr, data, len) != 0) {
		LOG_E("Read Sector %d Failed\r\n", sec);
		return;
	}
	return;
}

uint32_t oblfr_kved_flash_sector_size(kved_flash_sector_t sec, void *drv_arg)
{
	oblfr_kved_flash_driver_t *flash_drv = (oblfr_kved_flash_driver_t *)drv_arg;
	// sector sizes must be equal
	if (sec == KVED_FLASH_STRING_SECTOR_A || sec == KVED_FLASH_STRING_SECTOR_B) {
				/* No of strings times string size              +  indexes               and 2 magic bytes                */
		return ( flash_drv->max_entries * KVED_MAX_STRING_SIZE) + ((flash_drv->max_entries + 2) * sizeof(kved_word_t));
	} else if (sec == KVED_FLASH_SECTOR_A || sec == KVED_FLASH_SECTOR_B)
		return (flash_drv->flash_sector_size);
	return 0;
}

bool oblfr_kved_flash_init(void *drv_arg)
{
	oblfr_kved_flash_driver_t *flash_drv = (oblfr_kved_flash_driver_t *)drv_arg;
    spi_flash_cfg_type flashCfg;
    uint8_t *pFlashCfg = NULL;
    uint32_t flashCfgLen = 0;

    bflb_flash_get_cfg(&pFlashCfg, &flashCfgLen);
    arch_memcpy((void *)&flashCfg, pFlashCfg, flashCfgLen);
	flash_drv->flash_sector_size = (flashCfg.sector_size * 1024);
	if (flash_drv->max_entries == 0)
		flash_drv->max_entries = (flash_drv->flash_sector_size) / (sizeof(kved_word_t) * 2);
	else {
		if (flash_drv->max_entries > (flash_drv->flash_sector_size) / (sizeof(kved_word_t) * 2)) {
			LOG_E("Max Entries %d is greater than sector size %d\r\n", flash_drv->max_entries, flash_drv->flash_sector_size);
			return false;
		}
	}

	LOG_I("Initializing KVED Flash Driver\r\n");
	LOG_I("Flash Sector Size: %d\r\n", flash_drv->flash_sector_size);
	LOG_I("Number of entries: %d - Max String Size %d\r\n", flash_drv->max_entries, KVED_MAX_STRING_SIZE);
	LOG_I("Index Size: %dKB\r\n", oblfr_kved_flash_sector_size(KVED_FLASH_SECTOR_A, drv_arg)/1024);
	LOG_I("String Index Size: %dKB\r\n", oblfr_kved_flash_sector_size(KVED_FLASH_STRING_SECTOR_A, drv_arg)/1024);
	LOG_I("Total KVED Partition Size: %dKB\r\n", ((oblfr_kved_flash_sector_size(KVED_FLASH_SECTOR_A, drv_arg) * 2) + (oblfr_kved_flash_sector_size(KVED_FLASH_STRING_SECTOR_A, drv_arg) * 2))/1024);
	LOG_I("Start 0x%x - End 0x%x\r\n", flash_drv->flash_addr, flash_drv->flash_addr + ((oblfr_kved_flash_sector_size(KVED_FLASH_SECTOR_A, drv_arg) * 2) + (oblfr_kved_flash_sector_size(KVED_FLASH_STRING_SECTOR_A, drv_arg) * 2)));
	return true;
}

uint16_t oblfr_kved_flash_max_entries(void *drv_arg)
{
	oblfr_kved_flash_driver_t *flash_drv = (oblfr_kved_flash_driver_t *)drv_arg;
	return flash_drv->max_entries;
}

static kved_flash_driver_t oblfr_kved_flash_driver = {
	.init = oblfr_kved_flash_init,
	.sector_erase = oblfr_kved_flash_sector_erase,
	.header_write = oblfr_kved_flash_header_write,
	.header_read = oblfr_kved_flash_header_read,
	.data_read = oblfr_kved_flash_data_read,
	.data_write = oblfr_kved_flash_data_write,
	.sector_size = oblfr_kved_flash_sector_size,
	.max_entries = oblfr_kved_flash_max_entries,
};

kved_flash_driver_t *oblfr_kved_flash_configure(oblfr_kved_flash_driver_t *cfg) {
	
	oblfr_kved_flash_driver.drv_arg = cfg;


	return &oblfr_kved_flash_driver;
}

void oblfr_kved_flash_close(kved_flash_driver_t *driver) {

}