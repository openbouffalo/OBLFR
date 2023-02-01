/*
kved (key/value embedded database), a simple key/value database
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "oblfr_kved_file.h"


#if 0
#define DBG_TAG "MAIN"
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

typedef struct file_driver_s {
	char filename[13];
	FILE *file;
} file_driver_t;


static uint16_t get_sector_addr(kved_flash_sector_t sec) {
	switch (sec) {
		case KVED_FLASH_SECTOR_A:
			return 0;
		case KVED_FLASH_SECTOR_B:
			return FLASH_SECTOR_SIZE;
		case KVED_FLASH_STRING_SECTOR_A:
			return FLASH_SECTOR_SIZE * 2;
		case KVED_FLASH_STRING_SECTOR_B:
			return FLASH_SECTOR_SIZE * 2 + FLASH_STR_SECTOR_SIZE;
		default:
			return 0;
	}
}


static uint16_t get_index_address(uint16_t index)
{
	return sizeof(kved_word_t) * index;
}

bool oblfr_kved_file_sector_erase(kved_flash_sector_t sec, void *drv_arg)
{
	file_driver_t *file_driver = (file_driver_t *)drv_arg;
	if (file_driver == NULL || file_driver->file == NULL) {
		LOG_E("File not open\r\n");
		return false;
	}
	uint8_t data = 0xff;
	switch (sec) {
		case KVED_FLASH_SECTOR_A:
		case KVED_FLASH_SECTOR_B:
			LOG_T("Erase Index Sector %d size: %d\r\n", get_sector_addr(sec), FLASH_SECTOR_SIZE);
			fseek(file_driver->file, get_sector_addr(sec), SEEK_SET);
			for (int i = 0; i < FLASH_SECTOR_SIZE; i++)
				fwrite(&data, 1, 1, file_driver->file);
			break;
		case KVED_FLASH_STRING_SECTOR_A:
		case KVED_FLASH_STRING_SECTOR_B:
			LOG_T("Erase String Sector %d size: %ld\r\n", get_sector_addr(sec), FLASH_STR_SECTOR_SIZE);
			fseek(file_driver->file, get_sector_addr(sec), SEEK_SET);
			for (int i = 0; i < FLASH_STR_SECTOR_SIZE; i++)
				fwrite(&data, 1, 1, file_driver->file);
			break;
	}
	return true;
}

void oblfr_kved_file_header_write(kved_flash_sector_t sec, uint16_t index, kved_word_t data, void *drv_arg)
{
	file_driver_t *file_driver = (file_driver_t *)drv_arg;
	if (file_driver == NULL || file_driver->file == NULL) {
		LOG_E("File not open\r\n");
		return;
	}
	uint16_t addr = get_sector_addr(sec) + get_index_address(index);
	fseek(file_driver->file, addr, SEEK_SET);
	fwrite(&data, sizeof(kved_word_t), 1, file_driver->file);
}

kved_word_t oblfr_kved_file_header_read(kved_flash_sector_t sec, uint16_t index, void *drv_arg)
{
	file_driver_t *file_driver = (file_driver_t *)drv_arg;
	if (file_driver == NULL || file_driver->file == NULL) {
		LOG_E("File not open\r\n");
		return 0;
	}
	uint16_t addr = get_sector_addr(sec) + get_index_address(index);
	static kved_word_t data;
	fseek(file_driver->file, addr, SEEK_SET);
	fread(&data, sizeof(kved_word_t), 1, file_driver->file);
	return data;
}

void oblfr_kved_file_data_write(kved_flash_sector_t sec, uint16_t index, void *data, uint16_t len, void *drv_arg)
{
	file_driver_t *file_driver = (file_driver_t *)drv_arg;
	if (file_driver == NULL || file_driver->file == NULL) {
		LOG_E("File not open\r\n");
		return;
	}
	uint16_t addr = get_sector_addr(sec) + get_index_address(index);
	fseek(file_driver->file, addr, SEEK_SET);
	fwrite(data, 1, len, file_driver->file);
}

void oblfr_kved_file_data_read(kved_flash_sector_t sec, uint16_t index, void *data, uint16_t len, void *drv_arg)
{
	file_driver_t *file_driver = (file_driver_t *)drv_arg;
	if (file_driver->file == NULL) {
		LOG_E("File not open\r\n");
		return;
	}
	uint16_t addr = get_sector_addr(sec) + get_index_address(index);
	fseek(file_driver->file, addr, SEEK_SET);
	fread(data, 1, len, file_driver->file);
}

uint32_t oblfr_kved_file_sector_size(kved_flash_sector_t sec, void *drv_arg)
{
	// sector sizes must be equal
	if (sec == KVED_FLASH_STRING_SECTOR_A || sec == KVED_FLASH_STRING_SECTOR_B)
		return FLASH_STR_SECTOR_SIZE;
	else if (sec == KVED_FLASH_SECTOR_A || sec == KVED_FLASH_SECTOR_B)
		return FLASH_SECTOR_SIZE;
}

void oblfr_kved_file_init(void *drv_arg)
{
	LOG_T("oblfr_kved_file_init()\r\n");
}

static kved_flash_driver_t oblfr_kved_file_driver = {
	.init = oblfr_kved_file_init,
	.sector_erase = oblfr_kved_file_sector_erase,
	.header_write = oblfr_kved_file_header_write,
	.header_read = oblfr_kved_file_header_read,
	.data_read = oblfr_kved_file_data_read,
	.data_write = oblfr_kved_file_data_write,
	.sector_size = oblfr_kved_file_sector_size,
};

kved_flash_driver_t *oblfr_kved_file_configure() {
	struct stat st = {0};
	file_driver_t *file_driver = malloc(sizeof(file_driver_t));
	snprintf(file_driver->filename, sizeof(file_driver->filename), "kved.bin");
	if (stat(file_driver->filename, &st) == -1) {
		/* New File */
		LOG_T("Creating new file %s\r\n", file_driver->filename);
		file_driver->file = fopen(file_driver->filename, "wb+");
	} else {
		/* existing file */
		LOG_T("Opened file %s\r\n", file_driver->filename);
		file_driver->file = fopen(file_driver->filename, "rb+");
	}
	if (file_driver->file == NULL) {
		LOG_E("Failed to open file %s\r\n", file_driver->filename);
		return NULL;
	}
	oblfr_kved_file_driver.drv_arg = file_driver;
	return &oblfr_kved_file_driver;
}

void oblfr_kved_file_close(kved_flash_driver_t *driver) {
	file_driver_t *file_driver = (file_driver_t *)driver->drv_arg;
	if (file_driver->file != NULL) {
		fclose(file_driver->file);
		file_driver->file = NULL;
	}
	free(file_driver);
	driver->drv_arg = NULL;
}