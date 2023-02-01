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

#include "oblfr_common.h"
#include "oblfr_kved_memory.h"


#if 1
#define DBG_TAG "KVED"
#include "log.h"
#else
#define LOG_T(...)
#define LOG_I(...) printf(__VA_ARGS__);
#define LOG_E(...) printf(__VA_ARGS__);
#endif
#if 0
#undef LOG_T
#define LOG_T(...) printf(__VA_ARGS__); 
#endif

#define FLASH_NUM_ENTRIES (48)
/* flash Sector uses NUM_ENTRIES as part of headers */
#define FLASH_SECTOR_SIZE (FLASH_NUM_ENTRIES * KVED_FLASH_WORD_SIZE)
/* String Sector has seperate indexes */
/*                               no of entries    *   max string size     +   no of idx's      * 2 bytes for index     +  Magic Headers */
#define FLASH_STR_SECTOR_SIZE ((FLASH_NUM_ENTRIES * KVED_MAX_STRING_SIZE) + (FLASH_NUM_ENTRIES * KVED_FLASH_WORD_SIZE) + (FLASH_NUM_ENTRIES * 2))

uint32_t *sector_address[KVED_FLASH_NUM_SECTORS];
uint32_t *data_bank0;
uint32_t *data_bank1;
uint32_t *str_bank0;
uint32_t *str_bank1;

static uint16_t get_index_address(uint16_t index)
{
	return index * 2;
}

bool oblfr_kved_memory_sector_erase(kved_flash_sector_t sec, void *drv_arg)
{
	if (sec == KVED_FLASH_SECTOR_A || sec == KVED_FLASH_SECTOR_B) {
		memset(sector_address[sec], 0xFF, FLASH_SECTOR_SIZE);
	} else if (sec == KVED_FLASH_STRING_SECTOR_A || sec == KVED_FLASH_STRING_SECTOR_B) {
		memset(sector_address[sec], 0xFF, FLASH_STR_SECTOR_SIZE);
	} else {
		return false;
	}

	return true;
}

void oblfr_kved_memory_header_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data, void *drv_arg)
{
	sector_address[sec][get_index_address(index)] = ((uint32_t)(data >> 32));
	sector_address[sec][get_index_address(index) + 1] = (data);
}

kved_word_t oblfr_kved_memory_header_read(kved_flash_sector_t sec, uint16_t index, void *drv_arg)
{
	kved_word_t data = ((kved_word_t)sector_address[sec][get_index_address(index)] << 32) | sector_address[sec][get_index_address(index) + 1];
	return data;
}

void oblfr_kved_memory_data_write(kved_flash_sector_t sec, uint16_t index, void *data, uint16_t len, void *drv_arg)
{
	for (int i = 0; i < len; i++)
	{
		sector_address[sec][get_index_address(index) + i] = ((uint32_t *)data)[i];
	}
}

void oblfr_kved_memory_data_read(kved_flash_sector_t sec, uint16_t index, void *data, uint16_t len, void *drv_arg)
{
	for (int i = 0; i < len; i++)
	{
		((uint32_t *)data)[i] = sector_address[sec][get_index_address(index) + i];
	}
}

uint32_t oblfr_kved_memory_sector_size(kved_flash_sector_t sec, void *drv_arg)
{
	// sector sizes must be equal
	if (sec == KVED_FLASH_STRING_SECTOR_A || sec == KVED_FLASH_STRING_SECTOR_B)
		return FLASH_STR_SECTOR_SIZE;
	else if (sec == KVED_FLASH_SECTOR_A || sec == KVED_FLASH_SECTOR_B)
		return FLASH_SECTOR_SIZE;
	else
		return 0;
}

bool oblfr_kved_memory_init(void *drv_arg)
{

	return true;
}


uint16_t oblfr_kved_memory_max_entries(void *drv_arg)
{
	return FLASH_NUM_ENTRIES;
}


static kved_flash_driver_t oblfr_kved_memory_driver = {
	.init = oblfr_kved_memory_init,
	.sector_erase = oblfr_kved_memory_sector_erase,
	.header_write = oblfr_kved_memory_header_write,
	.header_read = oblfr_kved_memory_header_read,
	.data_read = oblfr_kved_memory_data_read,
	.data_write = oblfr_kved_memory_data_write,
	.sector_size = oblfr_kved_memory_sector_size,
	.max_entries = oblfr_kved_memory_max_entries,
};

kved_flash_driver_t *oblfr_kved_memory_configure() {
	

	data_bank0 = malloc(FLASH_SECTOR_SIZE);
	data_bank1 = malloc(FLASH_SECTOR_SIZE);
	str_bank0 = malloc(FLASH_STR_SECTOR_SIZE);
	str_bank1 = malloc(FLASH_STR_SECTOR_SIZE);

	LOG_I("Allocated %d memory for Main Table Size - Max Entries: %d\r\n", FLASH_SECTOR_SIZE * 2, (FLASH_NUM_ENTRIES/2)-1);
	LOG_I("Allocated %ld memory for String Table Size - Max String Size: %ld\r\n", FLASH_STR_SECTOR_SIZE * 2, KVED_MAX_STRING_SIZE);
	sector_address[KVED_FLASH_SECTOR_A] = data_bank0;
	sector_address[KVED_FLASH_SECTOR_B] = data_bank1;
	sector_address[KVED_FLASH_STRING_SECTOR_A] = str_bank0;
	sector_address[KVED_FLASH_STRING_SECTOR_B] = str_bank1;

	oblfr_kved_memory_sector_erase(KVED_FLASH_SECTOR_A, sector_address);
	oblfr_kved_memory_sector_erase(KVED_FLASH_SECTOR_B, sector_address);
	oblfr_kved_memory_sector_erase(KVED_FLASH_STRING_SECTOR_A, sector_address);
	oblfr_kved_memory_sector_erase(KVED_FLASH_STRING_SECTOR_B, sector_address);

	oblfr_kved_memory_driver.drv_arg = sector_address;

	return &oblfr_kved_memory_driver;
}

void oblfr_kved_memory_close(kved_flash_driver_t *driver) {
	free(sector_address[KVED_FLASH_SECTOR_A]);
	free(sector_address[KVED_FLASH_SECTOR_B]);
	free(sector_address[KVED_FLASH_STRING_SECTOR_A]);
	free(sector_address[KVED_FLASH_STRING_SECTOR_B]);
}