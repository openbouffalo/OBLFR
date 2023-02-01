/*
kved (key/value embedded database), a simple key/value database
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

/**
@file
*/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#if 1
#include <sdkconfig.h>

#ifdef CONFIG_FREERTOS
#include "FreeRTOS.h"
#include "semphr.h"
#endif

#define DBG_TAG "KVED"
#include "log.h"

#else
#define LOG_T(...) printf(__VA_ARGS__);
#define LOG_I(...) printf(__VA_ARGS__);
#define LOG_E(...) printf(__VA_ARGS__);
#endif

#include "kved.h"

#ifndef __weak
#define __weak   __attribute__((weak))
#endif

#define KVED_CHECK_ERR_GOTO(x, y) \
	do { \
		ret = x; \
		if (ret != KVED_OK) {\
			LOG_E("Error at %s:%d - %u\r\n", __FILE__, __LINE__, ret); \
			goto y; \
		} \
	} while (0)
#define KVED_CHECK_ERR_RETURN(x) \
	do { \
		kved_error_t ret = x; \
		if (ret != KVED_OK) {\
			LOG_E("Error at %s:%d - %u\r\n", __FILE__, __LINE__, ret); \
			return ret; \
		} \
	} while (0)

static kved_error_t kved_string_consistency_check(kved_ctrl_t *ctrl);
static kved_error_t kved_data_consistency_check(kved_ctrl_t *ctrl);

// must match @ref kved_data_t
/** @private */
const uint16_t kved_key_type_size[] =
	{
		1,
		1,
		2,
		2,
		2,
		2,
		4,
		KVED_MAX_KEY_SIZE,
		8,
		8,
		8,
};

/** @private */
typedef struct kved_sector_stat_s
{
	uint16_t num_free_entries;	  /**< @private */
	uint16_t num_deleted_entries; /**< @private */
	uint16_t num_used_entries;	  /**< @private */
	uint16_t num_total_entries;	  /**< @private */
} kved_sector_stat_t;

typedef struct kved_str_sector_ctrl_s
{
	uint16_t next_free_index;  /**< @private */
	uint16_t next_free_sector; /**< @private */
} kved_str_sector_ctrl_t;

typedef struct kved_str_sector_stats_s
{
	uint16_t num_free_entries;	  /**< @private */
	uint16_t num_deleted_entries; /**< @private */
	uint16_t num_used_entries;	  /**< @private */
	uint16_t num_total_entries;	  /**< @private */
} kved_str_sector_stats_t;

/** @private */
typedef struct kved_ctrl_s
{
	uint16_t first_index;			   /**< @private */
	uint16_t first_free_index;		   /**< @private */
	uint16_t last_index;			   /**< @private */
	kved_str_sector_ctrl_t str_ctrl;   /**< @private */
	kved_sector_stat_t stats;		   /**< @private */
	kved_str_sector_stats_t str_stats; /**< @private */
	kved_flash_sector_t sector;		   /**< @private */
	kved_flash_sector_t str_sector;	   /**< @private */
	bool started;					   /**< @private */
	kved_flash_driver_t *fdriver;	   /**< @private */
	uint16_t drv_max_entries;		   /**< @private */
#ifdef CONFIG_FREERTOS
	SemaphoreHandle_t mutex; 			/**< @private */
#endif
} kved_ctrl_t;

kved_error_t kved_cpu_critical_section_enter(kved_ctrl_t *ctrl)
{
#ifdef CONFIG_FREERTOS
	if (xSemaphoreTake(ctrl->mutex, portMAX_DELAY) != pdTRUE)
		return KVED_ERROR;
#endif
    return KVED_OK;
}

kved_error_t kved_cpu_critical_section_leave(kved_ctrl_t *ctrl)
{   
#ifdef CONFIG_FREERTOS
	if (xSemaphoreGive(ctrl->mutex) != pdTRUE)
		return KVED_ERROR;
#endif
    return KVED_OK;
}



static void nv_sector_stats_erase(kved_sector_stat_t *stats)
{
	stats->num_deleted_entries = 0;
	stats->num_free_entries = 0;
	stats->num_total_entries = 0;
	stats->num_used_entries = 0;
}

/* where in the String Sector does the indexes start */
static uint16_t kved_string_entry_to_header(kved_ctrl_t *ctrl, uint16_t entry)
{
	return 1 + entry;
}
/* calculate which sector the entry starts at */
static uint16_t kved_string_entry_to_start_sector(kved_ctrl_t *ctrl, uint16_t offset)
{
	/*    magic  + index headers         + magic + offset */
	return 1 + ctrl->stats.num_total_entries + 1 + offset;
}

const uint8_t *kved_data_type_label[] =
	{
		(uint8_t *)"U8",
		(uint8_t *)"I8",
		(uint8_t *)"U16",
		(uint8_t *)"I16",
		(uint8_t *)"U32",
		(uint8_t *)"I32",
		(uint8_t *)"FLT",
		(uint8_t *)"STR",
		(uint8_t *)"U64",
		(uint8_t *)"I64",
		(uint8_t *)"DBL",
};

static void kved_print(kved_word_t val)
{
	uint8_t *p = (uint8_t *)&val;

	p += KVED_FLASH_WORD_SIZE - 1;
	for (uint8_t n = 0; n < sizeof(kved_word_t); n++, p--)
		printf("%02X", *p);
}

static void kved_print_ascii(kved_word_t val, size_t size, bool reverse)
{
	uint8_t *p = (uint8_t *)&val;

	if (reverse)
		p += size - 1;

	for (uint8_t n = 0; n < size; n++)
	{
		if (isprint(*p))
		{
			printf("%c", *p);
		}
		else
		{
			printf(".");
		}

		if (reverse)
			p--;
		else
			p++;
	}
}

static kved_error_t kved_internal_dump(kved_ctrl_t *ctrl)
{
	bool first_free_printed = false;
	kved_word_t hdr = ctrl->fdriver->header_read(ctrl->sector, 0, ctrl->fdriver->drv_arg);
	kved_word_t cnt = ctrl->fdriver->header_read(ctrl->sector, 1, ctrl->fdriver->drv_arg);

#if KVED_FLASH_WORD_SIZE == 8
	printf("HDR (SEC %c)     SIGNATURE        COUNTER\r\n", (ctrl->sector == 0 ? 'A' : 'B'));
#else
	printf("HDR (SEC %c)     SIGNAT.  COUNTER\r\n", (ctrl->sector == 0 ? 'A' : 'B'));
#endif

	printf("ITEM IDX TYP SZ ");
	kved_print(hdr);
	printf(" ");
	kved_print(cnt);
	printf("\r\n");

	for (uint16_t index = ctrl->first_index; index <= ctrl->last_index && !first_free_printed; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = ctrl->fdriver->header_read(ctrl->sector, index, ctrl->fdriver->drv_arg);
		kved_word_t val = ctrl->fdriver->header_read(ctrl->sector, index + 1, ctrl->fdriver->drv_arg);

		if (key == KVED_DELETED_ENTRY)
		{
			printf("DEL  ");
		}
		else if (key == KVED_FREE_ENTRY)
		{
			if (val == KVED_FREE_ENTRY)
			{
				printf("FREE ");
				first_free_printed = true;
			}
			else
			{
				printf("ERR1 ");
			}
		}
		else
		{
			printf("USED ");
		}

		uint8_t size = KVED_HDR_MASK_SIZE(key);
		uint8_t type = KVED_HDR_MASK_TYPE(key);

		/* uint64 MAX also is equivelent to FREE_ENTRY signature! */
		if ((type != KVED_DATA_TYPE_UINT64 && val == KVED_FREE_ENTRY) || (key == KVED_DELETED_ENTRY))
		{
			printf("%03d        ", index);
		}
		else
		{
			printf("%03d %3s %02d ", index, (char *)kved_data_type_label[type], size);
		}
		kved_print(key);
		printf(" ");
		kved_print(val);
		printf(" ");
		kved_print_ascii(key, KVED_FLASH_WORD_SIZE, true);
		printf(" ");
		kved_print_ascii(val, KVED_FLASH_WORD_SIZE, type == KVED_DATA_TYPE_STRING ? false : true);
		printf("\r\n");
	}

	printf("TOTAL %d USED %d DELETED %d FREE %d\r\n\r\n",
		   ctrl->stats.num_total_entries,
		   ctrl->stats.num_used_entries,
		   ctrl->stats.num_deleted_entries,
		   ctrl->stats.num_free_entries);

	printf("STRING SECTION\r\n");
	printf("ITEM IDX SECSTART RAWSIZE SECLEN SECEND\r\n");
	first_free_printed = false;
	for (uint16_t index = 0; index <= ctrl->str_stats.num_total_entries && !first_free_printed; index++)
	{
		kved_word_t idx = ctrl->fdriver->header_read(ctrl->str_sector, kved_string_entry_to_header(ctrl, index), ctrl->fdriver->drv_arg);

		if (idx == KVED_STR_FREE_ENTRY)
		{
			printf("FREE %03d\r\n", index);
			first_free_printed = true;
		}
		else if (idx == KVED_STR_DELETED_ENTRY)
		{
			printf("DEL  %03d\r\n", index);
		}
		else
		{
			kved_word_t start = ((idx & KVED_STR_HDR_OFFSET_MSK) >> 32);
			uint16_t datalen = (idx & KVED_STR_HDR_LEN_MSK);
			uint16_t sectorlen = (datalen / KVED_FLASH_WORD_SIZE) + 1;
			uint16_t sectorend = start + sectorlen;
			printf("OCPD %03d       %2ld      %2d     %2d     %2d", index, start, datalen, sectorlen, sectorend);
			char data[16];
			ctrl->fdriver->data_read(ctrl->str_sector, kved_string_entry_to_start_sector(ctrl, start), data, datalen < 6 ? datalen : 6, ctrl->fdriver->drv_arg);
			data[datalen < 6 ? datalen : 6] = 0;
			printf("  %s%s\r\n", data, (datalen > 7 ? "..." : ""));
		}
	}
	printf("NEXT FREE IDX: %d, NEXT FREE SEC: %d\r\n", ctrl->str_ctrl.next_free_index, ctrl->str_ctrl.next_free_sector);
	printf("\r\n");
	return KVED_OK;
}

static void kved_sector_stats_read(kved_ctrl_t *ctrl)
{
	// [0,NV_HDR_SIZE] ARE NOT VALID AS ENTRY INDEXES, THEY ARE RESERVED FOR HEADER
	ctrl->first_index = KVED_HDR_SIZE_IN_WORDS;
	ctrl->last_index = (ctrl->fdriver->sector_size(ctrl->sector, ctrl->fdriver->drv_arg) / KVED_FLASH_WORD_SIZE) - KVED_HDR_SIZE_IN_WORDS;
	ctrl->first_free_index = 0;

	nv_sector_stats_erase(&ctrl->stats);

	for (uint16_t index = ctrl->first_index; index <= ctrl->last_index; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = ctrl->fdriver->header_read(ctrl->sector, index, ctrl->fdriver->drv_arg);

		if (key == KVED_DELETED_ENTRY)
		{
			ctrl->stats.num_deleted_entries++;
		}
		else if (key == KVED_FREE_ENTRY)
		{
			ctrl->stats.num_free_entries++;

			if (ctrl->first_free_index == 0)
				ctrl->first_free_index = index;
		}
		else
		{
			ctrl->stats.num_used_entries++;
		}

		ctrl->stats.num_total_entries++;
	}
	LOG_I("KVED Stats: total %d used %d deleted %d free %d\r\n",
		   ctrl->stats.num_total_entries,
		   ctrl->stats.num_used_entries,
		   ctrl->stats.num_deleted_entries,
		   ctrl->stats.num_free_entries);
}

kved_error_t kved_key_decode(kved_ctrl_t *ctrl, kved_data_t *data, kved_word_t key)
{
	data->type = KVED_HDR_MASK_TYPE(key);

	uint8_t *pkey = (uint8_t *)&key;
	pkey += KVED_MAX_KEY_SIZE;

	for (size_t p = 0; p < KVED_MAX_KEY_SIZE; p++)
		data->key[p] = *pkey--;

	return KVED_OK;
}

kved_word_t kved_key_encode(kved_ctrl_t *ctrl, kved_data_t *data)
{
	uint8_t *str_key = data->key;
	uint8_t key[KVED_MAX_KEY_SIZE];

	strncpy((char *)key, (char *)data->key, KVED_MAX_KEY_SIZE);

	uint8_t size = data->type == KVED_DATA_TYPE_STRING ? strnlen((char *)str_key, KVED_MAX_KEY_SIZE) : kved_key_type_size[data->type];
	uint8_t hdr = (data->type << 4) | size;

	kved_word_t encoded_key = 0;
	uint8_t *pkey = (uint8_t *)&encoded_key;
	pkey += KVED_MAX_KEY_SIZE;

	for (size_t p = 0; p < KVED_MAX_KEY_SIZE; p++)
		*pkey-- = key[p];

	*pkey = hdr;

	return encoded_key;
}

static bool kved_is_valid_key(kved_ctrl_t *ctrl, kved_word_t key)
{
	key = KVED_HDR_MASK_KEY(key);

	return (key == KVED_HDR_MASK_KEY(KVED_SIGNATURE_ENTRY(ctrl))) ||
				   (key == KVED_HDR_MASK_KEY(KVED_DELETED_ENTRY)) ||
				   (key == KVED_HDR_MASK_KEY(KVED_FREE_ENTRY))
			   ? false
			   : true;
}

static uint16_t kved_key_index_find(kved_ctrl_t *ctrl, kved_word_t key)
{
	uint16_t key_index = KVED_INDEX_NOT_FOUND;

	key = KVED_HDR_MASK_KEY(key);
	for (uint16_t index = ctrl->first_index; index <= ctrl->last_index; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key_entry = ctrl->fdriver->header_read(ctrl->sector, index, ctrl->fdriver->drv_arg);
		key_entry = KVED_HDR_MASK_KEY(key_entry);

		if (key == key_entry)
		{
			key_index = index;
			break;
		}
	}

	return key_index;
}

static kved_word_t kved_value_encode(kved_data_t *data)
{
	// if (data->type == KVED_DATA_TYPE_STRING)
	// {
	// 	/* return the string index header instead */
	// 	return data->value.u64;
	// }
	return data->value.u64;
}

static void kved_value_string_decode(kved_ctrl_t *ctrl, kved_data_t *data, kved_word_t value)
{
	/* check the read index is within our index range */
	if (value > ctrl->str_stats.num_total_entries)
	{
		LOG_E("Invalid index %ld\r\n", value);
		return;
	}
	kved_word_t index = ctrl->fdriver->header_read(ctrl->str_sector, kved_string_entry_to_header(ctrl, value), ctrl->fdriver->drv_arg);
	uint16_t offset = kved_string_entry_to_start_sector(ctrl, (index >> 32));
	uint16_t len = (index & 0xFFFFFFFF);
	/* TODO check if its within our sector space */
	if (len > KVED_MAX_STRING_SIZE)
	{
		LOG_E("Invalid len %d\r\n", len);
		return;
	}
	ctrl->fdriver->data_read(ctrl->str_sector, offset, data->value.str, len, ctrl->fdriver->drv_arg);
}

static void kved_value_decode(kved_ctrl_t *ctrl, kved_data_t *data, kved_word_t value)
{
	if (data->type == KVED_DATA_TYPE_STRING)
	{
		kved_value_string_decode(ctrl, data, value);
	}
	else
	{
		data->value.u64 = value;
	}
}

static kved_error_t kved_sector_switch(kved_ctrl_t *ctrl, kved_word_t cnt, kved_word_t upd_key, kved_data_t *upd_data)
{
	LOG_T("Switching Index and String Sectors!!\r\n");
	uint16_t next_index = KVED_HDR_SIZE_IN_WORDS;
	uint16_t total_items = 0;
	uint16_t used_items = 0;
	uint16_t str_next_free_sector = 0;
	uint16_t str_next_index = 0;
	kved_flash_sector_t next_sector = ctrl->sector == KVED_FLASH_SECTOR_A ? KVED_FLASH_SECTOR_B : KVED_FLASH_SECTOR_A;
	kved_flash_sector_t next_str_sector = ctrl->str_sector == KVED_FLASH_STRING_SECTOR_A ? KVED_FLASH_STRING_SECTOR_B : KVED_FLASH_STRING_SECTOR_A;

	ctrl->fdriver->sector_erase(next_sector, ctrl->fdriver->drv_arg);
	ctrl->fdriver->sector_erase(next_str_sector, ctrl->fdriver->drv_arg);

	upd_key = KVED_HDR_MASK_KEY(upd_key);

	for (uint16_t index = ctrl->first_index; index <= ctrl->last_index; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = ctrl->fdriver->header_read(ctrl->sector, index, ctrl->fdriver->drv_arg);

		if (kved_is_valid_key(ctrl, key))
		{
			kved_word_t val = ctrl->fdriver->header_read(ctrl->sector, index + 1, ctrl->fdriver->drv_arg);

			ctrl->fdriver->header_write(next_sector, next_index++, key, ctrl->fdriver->drv_arg);

			if (KVED_HDR_MASK_KEY(key) == upd_key)
			{
				if (KVED_HDR_MASK_TYPE(key) == KVED_DATA_TYPE_STRING)
				{
					/* write the String Index Header and actual data with the updated string */
					kved_word_t len = strlen((const char *)upd_data->value.str)+1;
					kved_word_t offset = str_next_free_sector;
					kved_word_t ptr = (offset << 32) + len;
					kved_word_t sectorlen = (len / KVED_FLASH_WORD_SIZE) + 1;
					LOG_T("Write New String IDX %d, Offset %ld, Raw Len %ld, Sector Len %ld encoded %lx\r\n", next_index, offset, len, sectorlen, ptr);
					/* write our String Data Out */
					ctrl->fdriver->data_write(next_str_sector, kved_string_entry_to_start_sector(ctrl, offset), upd_data->value.str, len, ctrl->fdriver->drv_arg);
					/* write our string index to the header */
					ctrl->fdriver->header_write(next_str_sector, kved_string_entry_to_header(ctrl, str_next_index), ptr, ctrl->fdriver->drv_arg);

					/* finally write our main header */
					ctrl->fdriver->header_write(next_sector, next_index++, str_next_index, ctrl->fdriver->drv_arg);
					str_next_index++;
					str_next_free_sector += sectorlen +1;
				}
				else
				{
					ctrl->fdriver->header_write(next_sector, next_index++, kved_value_encode(upd_data), ctrl->fdriver->drv_arg);
				}
			}
			else
			{
				if (KVED_HDR_MASK_TYPE(key) == KVED_DATA_TYPE_STRING)
				{
					/* write the String Index Header and actual data with the existing string */
					kved_data_t old_data;
					kved_value_string_decode(ctrl, &old_data, val);
					kved_word_t len = strlen((const char *)old_data.value.str)+1;
					kved_word_t offset = str_next_free_sector;
					kved_word_t ptr = (offset << 32) + len;
					kved_word_t sectorlen = (len / KVED_FLASH_WORD_SIZE) + 1;
					LOG_T("Write Old String IDX %d, Offset %ld, Raw Len %ld, Sector Len %ld encoded %lx\r\n", next_index, offset, len, sectorlen, ptr);
					/* write our String Data Out */
					ctrl->fdriver->data_write(next_str_sector, kved_string_entry_to_start_sector(ctrl, offset), old_data.value.str, len, ctrl->fdriver->drv_arg);
					/* write our string index to the header */
					ctrl->fdriver->header_write(next_str_sector, kved_string_entry_to_header(ctrl, str_next_index), ptr, ctrl->fdriver->drv_arg);

					/* finally write our main header */
					ctrl->fdriver->header_write(next_sector, next_index++, str_next_index, ctrl->fdriver->drv_arg);
					str_next_index++;
					str_next_free_sector += sectorlen +1;
				}
				else
				{
					ctrl->fdriver->header_write(next_sector, next_index++, val, ctrl->fdriver->drv_arg);
				}
			}
			used_items++;
		}

		total_items++;
	}

	kved_flash_sector_t last_sector = ctrl->sector;
	ctrl->sector = next_sector;
	ctrl->first_index = KVED_HDR_SIZE_IN_WORDS;
	ctrl->last_index = (ctrl->fdriver->sector_size(ctrl->sector, ctrl->fdriver->drv_arg) / KVED_FLASH_WORD_SIZE) - KVED_HDR_SIZE_IN_WORDS;
	ctrl->first_free_index = next_index;
	ctrl->stats.num_deleted_entries = 0;
	ctrl->stats.num_total_entries = total_items;
	ctrl->stats.num_used_entries = used_items;
	ctrl->stats.num_free_entries = total_items - used_items;
	ctrl->str_ctrl.next_free_index = str_next_index;
	ctrl->str_ctrl.next_free_sector = str_next_free_sector;
	ctrl->str_sector = next_str_sector;

	// last value is not valid since it is equal to an erased flash entry
	if ((cnt + 1) == KVED_FLASH_UINT_MAX) // last value, avoiding some #if #def related to flash size
		cnt = 0;
	else
		cnt++;

	ctrl->fdriver->header_write(next_sector, 1, cnt, ctrl->fdriver->drv_arg);
	ctrl->fdriver->header_write(next_sector, 0, KVED_SIGNATURE_ENTRY(ctrl), ctrl->fdriver->drv_arg);
	ctrl->fdriver->header_write(next_str_sector, 0, KVED_STR_SIGNATURE_ENTRY(ctrl), ctrl->fdriver->drv_arg);
	ctrl->fdriver->header_write(next_str_sector, ctrl->stats.num_total_entries + 1, KVED_STR_SIGNATURE_END(ctrl), ctrl->fdriver->drv_arg); 


	ctrl->fdriver->header_write(last_sector, 0, 0, ctrl->fdriver->drv_arg); // only invalidate header, it is faster

	KVED_CHECK_ERR_RETURN(kved_data_consistency_check(ctrl));
	return KVED_OK;
}

kved_error_t kved_compact_database(kved_ctrl_t *ctrl)
{
	kved_error_t ret = KVED_OK;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	kved_word_t cnt = ctrl->fdriver->header_read(ctrl->sector, 1, ctrl->fdriver->drv_arg);
	KVED_CHECK_ERR_GOTO(kved_sector_switch(ctrl, cnt, KVED_SIGNATURE_ENTRY(ctrl), 0), err);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));

	return ret;
}



static kved_error_t kved_internal_strdata_write(kved_ctrl_t *ctrl, kved_data_t *data)
{

	/* find our first free index */
	kved_word_t idx = ctrl->str_ctrl.next_free_index;

	/* calc the index data - offset << 32 | len encoded with null termnator */
	kved_word_t len = strlen((const char *)data->value.str)+1;
	kved_word_t offset = ctrl->str_ctrl.next_free_sector;
	kved_word_t ptr = (offset << 32) + len;
	kved_word_t sectorlen = (len / KVED_FLASH_WORD_SIZE) + 1;

	LOG_T("String IDX %ld, Offset %ld, Raw Len %ld, Sector Len %ld encoded %lx\r\n", idx, offset, len, sectorlen, ptr);

	/* write our string index to the header */
	ctrl->fdriver->header_write(ctrl->str_sector, kved_string_entry_to_header(ctrl, idx), ptr, ctrl->fdriver->drv_arg);

	/* write our String Data Out */
	ctrl->fdriver->data_write(ctrl->str_sector, kved_string_entry_to_start_sector(ctrl, offset), data->value.str, len, ctrl->fdriver->drv_arg);

	/* update our string controls with new index and free_offset */
	ctrl->str_ctrl.next_free_index++;
	ctrl->str_ctrl.next_free_sector += sectorlen + 1;
	LOG_T("New Str Ctrl Next Index: %d Next Sector %d\r\n", ctrl->str_ctrl.next_free_index, ctrl->str_ctrl.next_free_sector);
	data->value.u64 = idx;
	return KVED_OK;
}

static kved_error_t kved_internal_data_write(kved_ctrl_t *ctrl, kved_data_t *data)
{
	bool sector_changed = false;

	if (!ctrl->started)
		return KVED_NOT_INITIALIZED;

	kved_word_t key = kved_key_encode(ctrl, data);

	if (!kved_is_valid_key(ctrl, key))
		return KVED_INVALID_KEY;

	uint16_t key_index = kved_key_index_find(ctrl, key);
	bool old_entry = key_index != KVED_INDEX_NOT_FOUND;

	// check if the value has changed or not (for existing keys)
	if (old_entry)
	{
		if (data->type == KVED_DATA_TYPE_STRING) {
			kved_word_t stored_value = ctrl->fdriver->header_read(ctrl->sector, key_index + 1, ctrl->fdriver->drv_arg);
			kved_data_t stored_data;
			kved_value_string_decode(ctrl, &stored_data, stored_value);
			if (strcmp((const char *)data->value.str, (const char *)stored_data.value.str) == 0) {
				LOG_T("IDX %d String Value has not changed, skipping write\r\n", key_index);
				return KVED_OK;
			}
		} else { 
			kved_word_t stored_value = ctrl->fdriver->header_read(ctrl->sector, key_index + 1, ctrl->fdriver->drv_arg);
			if (stored_value == kved_value_encode(data)) {
				LOG_T("IDX %d Value has not changed, skipping write\r\n", key_index);
				return KVED_OK;
			}
		}
	}
	// no space, exchanging sector do not solve this situation, you need more flash space !
	if (ctrl->stats.num_total_entries == ctrl->stats.num_used_entries) {
		LOG_E("No space in Indexs available.\r\n");
		kved_dump(ctrl);
		return KVED_TABLE_FULL;
	}

	// ok, we have space but a clean up is required before.
	// Let's do a sector switch and leave the garbage behind.
	// If we are writing using an existing entries it will
	// be their valued updated during the process.
	if (ctrl->stats.num_free_entries == 0)
	{
		kved_word_t cnt = ctrl->fdriver->header_read(ctrl->sector, 1, ctrl->fdriver->drv_arg);
		KVED_CHECK_ERR_RETURN(kved_sector_switch(ctrl, cnt, key, data));
		sector_changed = true;
	}


	// AN existing entry is moved to the new sector using the new value already, not to do anymore.
	// However we need to take care when a new entry is added or when we are updating an entry
	// in the same sector.
	bool old_entry_updated_in_the_same_sector = (!sector_changed) && old_entry;

	if (!old_entry || old_entry_updated_in_the_same_sector)
	{

		if (data->type == KVED_DATA_TYPE_STRING)
		{
			if (kved_internal_strdata_write(ctrl, data) != KVED_OK)
			{
				LOG_E("Could Not Write String Data\r\n");
				return KVED_CORRUPT_TABLE;
			}
		}

		// first data, after key
		LOG_T("Writing Index %d\r\n", ctrl->first_free_index);
		ctrl->fdriver->header_write(ctrl->sector, ctrl->first_free_index + 1, kved_value_encode(data), ctrl->fdriver->drv_arg);
		ctrl->fdriver->header_write(ctrl->sector, ctrl->first_free_index, key, ctrl->fdriver->drv_arg);

		ctrl->stats.num_free_entries--;
		ctrl->stats.num_used_entries++;
		ctrl->first_free_index += KVED_ENTRY_SIZE_IN_WORDS;

		// Existing data written in the same sector: erase the old entry
		if (old_entry_updated_in_the_same_sector)
		{
			ctrl->fdriver->header_write(ctrl->sector, key_index, KVED_DELETED_ENTRY, ctrl->fdriver->drv_arg);

			ctrl->stats.num_deleted_entries++;
			ctrl->stats.num_used_entries--;
		}
	}
#ifdef KVED_DEBUG
	KVED_CHECK_ERR_RETURN(kved_dump(ctrl));
	KVED_CHECK_ERR_RETURN(kved_data_consistency_check(ctrl));
#endif
	return KVED_OK;
}

kved_error_t kved_data_write(kved_ctrl_t *ctrl, kved_data_t *data)
{
	kved_error_t ret = KVED_OK;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	KVED_CHECK_ERR_GOTO(kved_internal_data_write(ctrl, data), err);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));

	return ret;
}

static int16_t kved_internal_first_used_index_get(kved_ctrl_t *ctrl)
{
	uint16_t first_index = KVED_INDEX_NOT_FOUND;

	for (uint16_t index = ctrl->first_index; index <= ctrl->last_index; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = ctrl->fdriver->header_read(ctrl->sector, index, ctrl->fdriver->drv_arg);

		if (kved_is_valid_key(ctrl, key))
		{
			first_index = index;
			break;
		}
	}

	return first_index;
}

int16_t kved_first_used_index_get(kved_ctrl_t *ctrl)
{
	int16_t result;
	kved_error_t ret;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	result = kved_internal_first_used_index_get(ctrl);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));
	if (ret != KVED_OK)
		return ret;
	return result;
}

static int16_t kved_internal_next_used_index_get(kved_ctrl_t *ctrl, uint16_t last_index)
{
	uint16_t next_index = KVED_INDEX_NOT_FOUND;

	for (uint16_t index = last_index + KVED_ENTRY_SIZE_IN_WORDS; index <= ctrl->last_index; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = ctrl->fdriver->header_read(ctrl->sector, index, ctrl->fdriver->drv_arg);

		if (kved_is_valid_key(ctrl, key))
		{
			next_index = index;
			break;
		}
	}

	return next_index;
}

int16_t kved_next_used_index_get(kved_ctrl_t *ctrl, uint16_t last_index)
{
	int16_t result;
	kved_error_t ret;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	result = kved_internal_next_used_index_get(ctrl, last_index);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));
	if (ret != KVED_OK)
		return ret;
	return result;
}

static kved_error_t kved_internal_data_read_by_index(kved_ctrl_t *ctrl, uint16_t index, kved_data_t *data)
{
	if ((index < ctrl->first_index) || (index > ctrl->last_index))
		return KVED_INVALID_INDEX;

	kved_word_t key = ctrl->fdriver->header_read(ctrl->sector, index, ctrl->fdriver->drv_arg);

	if (!kved_is_valid_key(ctrl, key))
		return KVED_INVALID_KEY;

	kved_word_t val = ctrl->fdriver->header_read(ctrl->sector, index + 1, ctrl->fdriver->drv_arg);
	kved_key_decode(ctrl, data, key);
	kved_value_decode(ctrl, data, val);


	return KVED_OK;
}

kved_error_t kved_data_read_by_index(kved_ctrl_t *ctrl, uint16_t index, kved_data_t *data)
{
	kved_error_t ret;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	KVED_CHECK_ERR_GOTO(kved_internal_data_read_by_index(ctrl, index, data), err);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));

	return ret;
}

static int16_t kved_internal_free_entries_get(kved_ctrl_t *ctrl)
{
	if (!ctrl->started)
		return KVED_NOT_INITIALIZED;

	return ctrl->stats.num_total_entries - ctrl->stats.num_used_entries;
}

int16_t kved_free_entries_get(kved_ctrl_t *ctrl)
{
	uint16_t result;
	kved_error_t ret;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	result = kved_internal_free_entries_get(ctrl);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));
	if (ret != KVED_OK)
		return ret;
	return result;
}

static int16_t kved_internal_total_entries_get(kved_ctrl_t *ctrl)
{
	if (!ctrl->started)
		return KVED_NOT_INITIALIZED;

	return ctrl->stats.num_total_entries;
}

int16_t kved_total_entries_get(kved_ctrl_t *ctrl)
{
	int16_t result;
	kved_error_t ret;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	result = kved_internal_total_entries_get(ctrl);
	err:
	KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));
	if (ret != KVED_OK)
		return ret;
	return result;
}

static int16_t kved_internal_used_entries_get(kved_ctrl_t *ctrl)
{
	if (!ctrl->started)
		return KVED_NOT_INITIALIZED;

	return ctrl->stats.num_used_entries;
}

int16_t kved_used_entries_get(kved_ctrl_t *ctrl)
{
	int16_t result;
	kved_error_t ret;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	result = kved_internal_used_entries_get(ctrl);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));
	if (ret != KVED_OK)
		return ret;
	return result;
}

static int16_t kved_internal_deleted_entries_get(kved_ctrl_t *ctrl)
{
	if (!ctrl->started)
		return KVED_NOT_INITIALIZED;

	return ctrl->stats.num_deleted_entries;
}

int16_t kved_deleted_entries_get(kved_ctrl_t *ctrl) {
	int16_t result;
	kved_error_t ret;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	result = kved_internal_deleted_entries_get(ctrl);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));
	if (ret != KVED_OK)
		return ret;
	return result;
}

static kved_error_t kved_internal_data_read(kved_ctrl_t *ctrl, kved_data_t *data)
{
	if (!ctrl->started)
		return KVED_NOT_INITIALIZED;

	kved_word_t key = kved_key_encode(ctrl, data);

	if (!kved_is_valid_key(ctrl, key))
		return KVED_INVALID_KEY;

	uint16_t key_index = kved_key_index_find(ctrl, key);

	if (key_index == KVED_INDEX_NOT_FOUND)
		return KVED_INVALID_KEY;

	// update the type as user may not know about them before calling
	data->type = KVED_HDR_MASK_TYPE(ctrl->fdriver->header_read(ctrl->sector, key_index, ctrl->fdriver->drv_arg));

	kved_word_t value = ctrl->fdriver->header_read(ctrl->sector, key_index + 1, ctrl->fdriver->drv_arg);
	kved_value_decode(ctrl, data, value);

#ifdef KVED_DEBUG
	KVED_CHECK_ERR_RETURN(kved_data_consistency_check(ctrl));
#endif
	return KVED_OK;
}

kved_error_t kved_data_read(kved_ctrl_t *ctrl, kved_data_t *data)
{
	kved_error_t ret;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	KVED_CHECK_ERR_GOTO(kved_internal_data_read(ctrl, data), err);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));
	return ret;
}

static kved_error_t kved_internal_data_delete(kved_ctrl_t *ctrl, kved_data_t *data)
{
	if (!ctrl->started)
		return KVED_NOT_INITIALIZED;

	kved_word_t key = kved_key_encode(ctrl, data);

	if (!kved_is_valid_key(ctrl, key)) {
		LOG_W("Delete: Invalid key: %s\r\n", data->key);
		return KVED_INVALID_KEY;
	}

	uint16_t key_index = kved_key_index_find(ctrl, key);
	if (key_index == KVED_INDEX_NOT_FOUND) {
		return KVED_INVALID_KEY;
	}
	ctrl->fdriver->header_write(ctrl->sector, key_index, KVED_DELETED_ENTRY, ctrl->fdriver->drv_arg);

	ctrl->stats.num_deleted_entries++;
	ctrl->stats.num_used_entries--;

#ifdef KVED_DEBUG
	KVED_CHECK_ERR_RETURN(kved_dump(ctrl));
	KVED_CHECK_ERR_RETURN(kved_data_consistency_check(ctrl));
#endif
	return KVED_OK;
}

kved_error_t kved_data_delete(kved_ctrl_t *ctrl, kved_data_t *data)
{
	kved_error_t ret;
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	ret = kved_internal_data_delete(ctrl, data);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));
	return ret;
}


static void kved_sector_consistency_check(kved_ctrl_t *ctrl)
{
	bool invalidate_a = false;
	bool invalidate_b = false;

	kved_word_t id_sec_a = ctrl->fdriver->header_read(KVED_FLASH_SECTOR_A, 0, ctrl->fdriver->drv_arg);
	kved_word_t id_sec_b = ctrl->fdriver->header_read(KVED_FLASH_SECTOR_B, 0, ctrl->fdriver->drv_arg);

	// Two valid signatures: as the signature is the latest item to be written into the sector
	// when a formatting or copy operation is performed, probably a restart event happened just
	// after data copying and, in this case, the section with the
	// newest cnt will win and the other can be erase as the copy was done.
	// (remember: last value (0xFF..FF) is not valid as it is the same value of a erased word
	if ((id_sec_a == KVED_SIGNATURE_ENTRY(ctrl)) && (id_sec_b == KVED_SIGNATURE_ENTRY(ctrl)))
	{
		kved_word_t cnt_sec_a = ctrl->fdriver->header_read(KVED_FLASH_SECTOR_A, 1, ctrl->fdriver->drv_arg);
		kved_word_t cnt_sec_b = ctrl->fdriver->header_read(KVED_FLASH_SECTOR_B, 1, ctrl->fdriver->drv_arg);

		// the (a) counter rolled over and the
		// sector (a->b) copy was done so erase older sector (a) and use the newer (b)
		if ((cnt_sec_a == (KVED_FLASH_UINT_MAX - 1)) && (cnt_sec_b == 0))
		{
			invalidate_a = true;
		}
		// the (b) counter rolled over and the
		// sector (b->a) copy was done so erase older sector (b) and use the newer (a)
		else if ((cnt_sec_b == (KVED_FLASH_UINT_MAX - 1)) && (cnt_sec_a == 0))
		{
			invalidate_b = true;
		}
		// unexpected situation since counter can not be KVED_FLASH_UINT_MAX: we
		// will invalidate the sector. Maybe some people would like to analize it
		// and rescue some valid entries. Not implemented, anyway.
		else if ((cnt_sec_a == KVED_FLASH_UINT_MAX) || (cnt_sec_b == KVED_FLASH_UINT_MAX))
		{
			invalidate_a = cnt_sec_a == KVED_FLASH_UINT_MAX;
			invalidate_b = cnt_sec_b == KVED_FLASH_UINT_MAX;
		}

		// ok, regular situation where one counter is the newest and
		// all values are normal, just choose the newest one
		if ((invalidate_a == false) && (invalidate_b == false))
		{
			if (cnt_sec_a > cnt_sec_b)
				invalidate_b = true;
			else
				invalidate_a = true;
		}

		// Invalidate selected sectors ...
		if (invalidate_a)
			ctrl->fdriver->header_write(KVED_FLASH_SECTOR_A, 0, 0, ctrl->fdriver->drv_arg);

		if (invalidate_b)
			ctrl->fdriver->header_write(KVED_FLASH_SECTOR_B, 0, 0, ctrl->fdriver->drv_arg);
	}
}

static kved_error_t kved_string_consistency_check(kved_ctrl_t *ctrl)
{
	/* the first X bytes after the Signature are pointers to the actual stored strings/data */
	/* first check if the End Signature is present */
	LOG_D("Checking String Table Consistency\r\n");
	if (KVED_STR_SIGNATURE_ENTRY(ctrl) != ctrl->fdriver->header_read(ctrl->str_sector, 0, ctrl->fdriver->drv_arg))
	{
		LOG_E("No Valid Start Signature found in String Sector\r\n");
		return KVED_CORRUPT_TABLE;
	}
	if (KVED_STR_SIGNATURE_END(ctrl) != ctrl->fdriver->header_read(ctrl->str_sector, ctrl->stats.num_total_entries + 1, ctrl->fdriver->drv_arg))
	{
		LOG_E("No Valid End Signature found in String Sector\r\n");
		return KVED_CORRUPT_TABLE;
	}
	memset(&ctrl->str_stats, 0, sizeof(ctrl->str_stats));
	bool free_entry_set = false;
	for (uint16_t i = 0; i < ctrl->stats.num_total_entries; i++)
	{
		kved_word_t ptr = ctrl->fdriver->header_read(ctrl->str_sector, kved_string_entry_to_header(ctrl, i), ctrl->fdriver->drv_arg);
		if (ptr == KVED_STR_FREE_ENTRY)
		{
			if (free_entry_set == false)
			{
				ctrl->str_ctrl.next_free_index = i;
				free_entry_set = true;
			}
			ctrl->str_stats.num_free_entries++;
		}
		else if (ptr == KVED_STR_DELETED_ENTRY)
		{
			ctrl->str_stats.num_deleted_entries++;
		}
		else
		{
			// kved_word_t start = ((ptr & KVED_STR_HDR_OFFSET_MSK) >> 32);
			// uint16_t datalen = (ptr & KVED_STR_HDR_LEN_MSK);
			// uint16_t sectorlen = (datalen / KVED_FLASH_WORD_SIZE) + 1;
			// uint16_t sectorend = start + sectorlen;
			// LOG_I("Index %d Start: %lu RawLen: %u SectorLen: %u SectorEnd: %u\r\n", i, start, datalen, sectorlen, sectorend);
			ctrl->str_stats.num_used_entries++;
		}
	}
	ctrl->str_stats.num_total_entries = ctrl->str_stats.num_free_entries + ctrl->str_stats.num_deleted_entries + ctrl->str_stats.num_used_entries;
	if (ctrl->str_stats.num_total_entries != ctrl->stats.num_total_entries)
	{
		LOG_E("String totals not matching Sector Totals!\r\n");
		return KVED_CORRUPT_TABLE;
	}
	LOG_T("KVED String Sector Stats: Total %d, Free %d, Deleted %d, Used %d\r\n", ctrl->str_stats.num_total_entries, ctrl->str_stats.num_free_entries, ctrl->str_stats.num_deleted_entries, ctrl->str_stats.num_used_entries);
	LOG_T("KVED String Control First Free Index: %d Next Free Sector: %d\r\n", ctrl->str_ctrl.next_free_index, ctrl->str_ctrl.next_free_sector);
	return KVED_OK;
}

static kved_error_t kved_data_consistency_check(kved_ctrl_t *ctrl)
{
	if (kved_string_consistency_check(ctrl) != KVED_OK) {
		LOG_E("String Consistency Check Failed!\r\n");
		return KVED_CORRUPT_TABLE;
	}
	LOG_T("Checking Data Consistency\r\n");
	for (uint16_t index = ctrl->first_index; index <= ctrl->last_index; index += KVED_ENTRY_SIZE_IN_WORDS)
	{
		kved_word_t key = ctrl->fdriver->header_read(ctrl->sector, index, ctrl->fdriver->drv_arg);
		kved_word_t val = ctrl->fdriver->header_read(ctrl->sector, index + 1, ctrl->fdriver->drv_arg);
#ifdef KVED_DEBUG
		if (kved_is_valid_key(ctrl, key))
			LOG_T("Checking Index %d Key: %lx Val: %lx\r\n", index, key, val);
#endif
		// As we write value first and key after and we may be powered off during this operation
		// it is necessary to fix cases where only the value was written. In such situation
		// a key with only FFs may exist and this entry will be deleted.
		// If the power down occurs when key is been written it is not possible to
		// detect if the operation was finished or not. But this case need to be solved
		// by application, removing any unknown or unexpected key (application knows its owns keys, kved not).
		if ((key == KVED_FLASH_UINT_MAX) && (val != KVED_FLASH_UINT_MAX))
		{
			ctrl->fdriver->header_write(ctrl->sector, index, 0, ctrl->fdriver->drv_arg);
			ctrl->stats.num_deleted_entries++;
			ctrl->stats.num_free_entries--;
			LOG_W("Deleted Entry at Index %d\r\n", index);
			continue;
		}

		// Second possible situation: new data is written in the same section and the
		// old value is not erased due some power down.
		// So, it is required to check for duplicated keys AHEAD (always)
		// and erase the old entry.
		if (kved_is_valid_key(ctrl, key))
		{
			LOG_T("Looking for duplicated keys\r\n");
			for (uint16_t dup_key_index = index + KVED_ENTRY_SIZE_IN_WORDS; dup_key_index <= ctrl->last_index; dup_key_index += KVED_ENTRY_SIZE_IN_WORDS)
			{
				kved_word_t dup_key = ctrl->fdriver->header_read(ctrl->sector, dup_key_index, ctrl->fdriver->drv_arg);
				if (kved_is_valid_key(ctrl, dup_key))
				{
					if (KVED_HDR_MASK_KEY(dup_key) == KVED_HDR_MASK_KEY(key))
					{
						LOG_W("Duplicated Key Found at Index %d\r\n", dup_key_index);
						ctrl->fdriver->header_write(ctrl->sector, index, 0, ctrl->fdriver->drv_arg);
						ctrl->stats.num_deleted_entries++;
						ctrl->stats.num_used_entries--;
						break;
					}
				}
			}
		}
	}
	LOG_T("Done\r\n");
	return KVED_OK;
}

kved_error_t kved_dump(kved_ctrl_t *ctrl)
{
	kved_error_t ret; 
	KVED_CHECK_ERR_GOTO(kved_cpu_critical_section_enter(ctrl), err);
	KVED_CHECK_ERR_GOTO(kved_internal_dump(ctrl), err);
	err:
		KVED_CHECK_ERR_RETURN(kved_cpu_critical_section_leave(ctrl));
	return ret;
}

kved_ctrl_t *kved_init(kved_flash_driver_t *driver)
{
	LOG_I("Starting KVED Backend\r\n");
	kved_ctrl_t *ctrl = (kved_ctrl_t*)malloc(sizeof(kved_ctrl_t));
	bzero(ctrl, sizeof(kved_ctrl_t));

#ifdef CONFIG_FREERTOS
	ctrl->mutex = xSemaphoreCreateMutex();
#endif	
	ctrl->fdriver = driver;

	if (ctrl->fdriver->init(ctrl->fdriver->drv_arg) == false) {
		LOG_E("Flash Driver Init Failed!\r\n");
		return NULL;
	}

	ctrl->drv_max_entries = ctrl->fdriver->max_entries(ctrl->fdriver->drv_arg);

	if (ctrl->drv_max_entries <= 0) {
		LOG_E("Flash Driver Max Entries Invalid!\r\n");
		return NULL;
	}

	kved_sector_consistency_check(ctrl);

	kved_word_t id_sec_a = ctrl->fdriver->header_read(KVED_FLASH_SECTOR_A, 0, ctrl->fdriver->drv_arg);
	kved_word_t id_sec_b = ctrl->fdriver->header_read(KVED_FLASH_SECTOR_B, 0, ctrl->fdriver->drv_arg);

	if (id_sec_a == KVED_SIGNATURE_ENTRY(ctrl))
	{
		LOG_I("Using Index Sector A\r\n");
		ctrl->sector = KVED_FLASH_SECTOR_A;
	}
	else if (id_sec_b == KVED_SIGNATURE_ENTRY(ctrl))
	{
		LOG_I("Using Index Sector B\r\n");
		ctrl->sector = KVED_FLASH_SECTOR_B;
	}
	else
	{
		LOG_I("No Valid Index Sector Found, Formating...\r\n");
		ctrl->sector = KVED_FLASH_SECTOR_A;
		ctrl->fdriver->sector_erase(ctrl->sector, ctrl->fdriver->drv_arg);
		ctrl->fdriver->header_write(ctrl->sector, 1, 0, ctrl->fdriver->drv_arg); // first cnt, after ID
		ctrl->fdriver->header_write(ctrl->sector, 0, KVED_SIGNATURE_ENTRY(ctrl), ctrl->fdriver->drv_arg);
	}

	kved_sector_stats_read(ctrl);

	kved_word_t id_str_sec_a = ctrl->fdriver->header_read(KVED_FLASH_STRING_SECTOR_A, 0, ctrl->fdriver->drv_arg);
	kved_word_t end_str_sec_a = ctrl->fdriver->header_read(KVED_FLASH_STRING_SECTOR_A, ctrl->stats.num_total_entries + 1, ctrl->fdriver->drv_arg);
	kved_word_t id_str_sec_b = ctrl->fdriver->header_read(KVED_FLASH_STRING_SECTOR_B, 0, ctrl->fdriver->drv_arg);
	kved_word_t end_str_sec_b = ctrl->fdriver->header_read(KVED_FLASH_STRING_SECTOR_B, ctrl->stats.num_total_entries + 1, ctrl->fdriver->drv_arg);

	if (id_str_sec_a == KVED_STR_SIGNATURE_ENTRY(ctrl) && end_str_sec_a == KVED_STR_SIGNATURE_END(ctrl))
	{
		LOG_I("Using String Sector A\r\n");
		ctrl->str_sector = KVED_FLASH_STRING_SECTOR_A;
	}
	else if (id_str_sec_b == KVED_STR_SIGNATURE_ENTRY(ctrl) && end_str_sec_b == KVED_STR_SIGNATURE_END(ctrl))
	{
		LOG_I("Using String Sector B\r\n");
		ctrl->str_sector = KVED_FLASH_STRING_SECTOR_B;
	}
	else
	{
		LOG_I("No Valid String Sector Found, Formating...\r\n");
		ctrl->str_sector = KVED_FLASH_STRING_SECTOR_A;
		ctrl->fdriver->sector_erase(ctrl->str_sector, ctrl->fdriver->drv_arg);
		ctrl->fdriver->header_write(ctrl->str_sector, 0, KVED_STR_SIGNATURE_ENTRY(ctrl), ctrl->fdriver->drv_arg);
		/* After Signature we have X entries as indexes to the strings/raw data then a Signature for the end of this index table */
		ctrl->fdriver->header_write(ctrl->str_sector, ctrl->stats.num_total_entries + 1, KVED_STR_SIGNATURE_END(ctrl), ctrl->fdriver->drv_arg);
	}
	LOG_I("Checking Data Consistency...\r\n");
	if (kved_data_consistency_check(ctrl) != KVED_OK)
	{
		LOG_E("KVED: Data consistency check failed!\r\n");
		free(ctrl);
		return NULL;
	}

	ctrl->started = true;

	kved_dump(ctrl);
	LOG_I("Started KVED Backend\r\n");
	return ctrl;
}

void kved_deinit(kved_ctrl_t *ctrl) {
	if (ctrl == NULL) {
		return;
	}
	free(ctrl);
	ctrl = NULL;
}