#ifndef KVED_H
#define KVED_H
/*
kved (key/value embedded database), a simple key/value database 
implementation for microcontrollers.

Copyright (c) 2022 Marcelo Barros de Almeida <marcelobarrosalmeida@gmail.com>
*/

#pragma once

/**
@file
@defgroup KVED KVED
@brief KVED (key/value embedded databse): simple key/value persistence for embedded applications.
@{
*/
#include <stddef.h>
#include <stdbool.h>
#include "sdkconfig.h"

#define KVED_FLASH_WORD_SIZE 8
/** Maximum supported string length, per record, without termination */
#ifndef CONFIG_COMPONENT_NVKVS_MAX_STRING_SIZE
#error "KVED: CONFIG_COMPONENT_NVKVS_MAX_STRING_SIZE not defined"
#else
#define KVED_MAX_STRING_SIZE CONFIG_COMPONENT_NVKVS_MAX_STRING_SIZE
#endif
//#define KVED_DEBUG


/* kved structure

Main Index:
<- 8 bytes -><- 8 bytes ->  
+------------+------------+
| SIGNATURE  |  COUNTER   | <= HEADER ID AND NEWER COPY IDENTIFICATION
+---------+--+------------+
|KEY ENTRY|TS| KEY VALUE  | <= VALID KEY (KEY ENTRY, TYPE, SIZE AND VALUE)
+---------+--+------------+
|KEY ENTRY|TS| KEY VALUE  | <= IF VALUE IS A STRING, THEN KEY VALUE POINTS TO A IDX ENTRY IN THE STRING TABLE
+---------+--+------------+
|KEY ENTRY|TS| KEY VALUE  |
+---------+--+------------+
|000 ... 0000| KEY VALUE  |  <= ERASED KEY. WHEN A STRING KEY IS ERASED, THE INDEX IN THE STRING TABLE IS NOT ERASED!
+------------+------------+
|FFF ... FFFF|FFF ... FFFF|  <= EMPTY ENTRY
+------------+------------+
|FFF ... FFFF|FFF ... FFFF|
+------------+------------+
|FFF ... FFFF|FFF ... FFFF|
+------------+------------+

String Table:
<- 8 bytes -><- 8 bytes -><- 8 bytes -><- 8 bytes ->
+------------+------------+------------+------------+
| SIGNATURE  |  IDX 1     |  IDX x     |  SIGNATURE |
+------------+------------+------------+------------+------------+
| STRING DATA 1                        | STR DATA 2              |
+------------+------------+------------+------------+------------+
| STRING DATA 3           | EMPTY                                |
+------------+------------+------------+------------+------------+

IDX Headers are 8 bytes long and contain the offset and length of the string data.
The offset is stored in the first 4 bytes and the length in the last 4 bytes.
The offset is relative to the start of the string data are, and points to the sector that contains the start of the string
The Length is the raw length of the string, including the NULL terminator. (not the number of sectors it occupies)
The Number of IDX entries equals the total number of keys in the main index.

The string data area is made up of 8 byte entries. Strings longer flow into the next sector. 
Strings start on a 8 byte boundary. Strings must be NULL terminated
Empty space in the String Table is filled with 0xFF.

When compacting the tables, only referenced strings are copied to the new string table.

*/


typedef uint64_t kved_word_t; /**< Main KVED Header Entry Size */

#define KVED_SIGNATURE_ENTRY(x)      ((0xDEADBEEF00000000ULL + (KVED_MAX_STRING_SIZE << 16)) + x->drv_max_entries)
#define KVED_STR_SIGNATURE_ENTRY(x)  ((0xBF00BF1B00000000ULL + (KVED_MAX_STRING_SIZE << 16)) + x->drv_max_entries)
#define KVED_STR_SIGNATURE_END(x)    ((0xBEEFDEAD00000000ULL + (KVED_MAX_STRING_SIZE << 16)) + x->drv_max_entries)

//#define KVED_SIGNATURE_ENTRY  0xDEADBEEFDEADBEEFULL
#define KVED_DELETED_ENTRY    0x0000000000000000ULL
#define KVED_FREE_ENTRY       0xFFFFFFFFFFFFFFFFULL
#define KVED_HDR_ENTRY_MSK    0xFFFFFFFFFFFFFF00ULL

//#define KVED_STR_SIGNATURE_ENTRY  0xBF00BF1BDEADBEEFULL
//#define KVED_STR_SIGNATURE_END    0xDEADBEEFBF00BF1BULL
#define KVED_STR_DELETED_ENTRY    0x0000000000000000ULL
#define KVED_STR_FREE_ENTRY       0xFFFFFFFFFFFFFFFFULL
#define KVED_STR_HDR_OFFSET_MSK   0xFFFFFFFF00000000ULL
#define KVED_STR_HDR_LEN_MSK      0x00000000FFFFFFFFULL

#define KVED_HDR_SIZE_IN_WORDS    2 /**< kved header size */
#define KVED_ENTRY_SIZE_IN_WORDS  2 /**< kved entry size */
#define KVED_HDR_MASK_KEY(k)      ( (k) & KVED_HDR_ENTRY_MSK) /**< label entry mask */
#define KVED_HDR_MASK_TYPE(k)     (((k) & 0xF0) >> 4) /**< type entry mask */
#define KVED_HDR_MASK_SIZE(k)     (((k) & 0x0F)) /**< size entry mask */

#define KVED_FLASH_UINT_MAX  ((kved_word_t)(~0)) /**< last valid unsigned int value for current flash word */

/** Key size for data access, with terminator */
#define KVED_MAX_KEY_SIZE    (KVED_FLASH_WORD_SIZE-1) 
/** Index return value when a key is not found in the database */
#define KVED_INDEX_NOT_FOUND 0 


typedef struct kved_ctrl_s kved_ctrl_t;

/**
@brief Supported data types.
*/
typedef enum kved_data_types_e
{
	KVED_DATA_TYPE_UINT8 = 0, /**< 8 bits, unsigned */
	KVED_DATA_TYPE_INT8,      /**< 8 bits, signed */
	KVED_DATA_TYPE_UINT16,    /**< 16 bits, unsigned */
	KVED_DATA_TYPE_INT16,     /**< 16 bits, signed */
	KVED_DATA_TYPE_UINT32,    /**< 32 bits, unsigned */
	KVED_DATA_TYPE_INT32,     /**< 32 bits, com sinal */
	KVED_DATA_TYPE_FLOAT,     /**< Single precision floating point (float) */
	KVED_DATA_TYPE_STRING,    /**< String up to @ref KVED_MAX_STRING_SIZE bytes, excluding terminator */
	KVED_DATA_TYPE_UINT64,    /**< 64 bits, signed */
	KVED_DATA_TYPE_INT64,     /**< 64 bits, unsigned */
	KVED_DATA_TYPE_DOUBLE,    /**< Double precision floating point (double) */
} kved_data_types_t;

/**
@brief Union with supported data types
*/
typedef union kved_value_u
{
	uint8_t u8; /**< unsigned 8 bits value */
	int8_t i8; /**< signed 8 bits value */
	uint16_t u16; /**< unsigned 16 bits value */
	int16_t i16; /**< signed 16 bits value */
	uint32_t u32; /**< unsigned 32 bits value */
	int32_t i32; /**< signed 32 bits value */
	float flt; /**< single precision float */
	uint8_t str[KVED_MAX_STRING_SIZE]; /**< string */
	uint64_t u64; /**< unsigned 64 bits value */
	int64_t i64; /**< signed 64 bits value */
	double dbl; /**< double precision float */
} kved_value_t; 

/**
@brief Structure for accessing the database containing information about key, type and value
*/
typedef struct kved_data_s
{
	kved_value_t value;             /**< User value */                  
	uint8_t key[KVED_MAX_KEY_SIZE]; /**< String used as access key */
	kved_data_types_t type;         /**< Data type used according to @ref kved_data_types_t */
} kved_data_t;

/**
 * @brief Flash Table enumeration
 * @details This enumeration is used to identify the flash table to be used.
*/

typedef enum kved_flash_sector_e
{
	KVED_FLASH_SECTOR_A = 0, 		/**< Main Table A */
	KVED_FLASH_SECTOR_B,     		/**< Main Table B */
	KVED_FLASH_STRING_SECTOR_A, 	/**< String Table A */
	KVED_FLASH_STRING_SECTOR_B, 	/**< String Table B */
	KVED_FLASH_NUM_SECTORS,  		/**< Number of Tables */
} kved_flash_sector_t;

/**
 * @brief Flash driver structure
 * @details This structure is used Populated with the functions to read/write to flash/disk/memory
*/
typedef struct {
  bool (*sector_erase)(kved_flash_sector_t sec, void *drv_arg);												/**< Erase sector */
  void (*header_write)(kved_flash_sector_t sec, uint16_t index, kved_word_t data, void *drv_arg);			/**< Write Headers (in Main and String Table) */
  kved_word_t (*header_read)(kved_flash_sector_t sec, uint16_t index, void *drv_arg);						/**< Read header (in Main and String Table)*/
  void (*data_write)(kved_flash_sector_t sec, uint16_t index, void *data, uint16_t len, void *drv_arg);		/**< Write data in String Table */
  void (*data_read)(kved_flash_sector_t sec, uint16_t index, void *data, uint16_t len, void *drv_arg);		/**< Read data in String Table */
  uint32_t (*sector_size)(kved_flash_sector_t sec, void *drv_arg);											/**< Get Table Size */
  bool (*init)( void *drv_arg);																				/**< Init driver */
  uint16_t (*max_entries)(void *drv_arg);																	/**< Max entries in table */
  void *drv_arg;																							/**< Driver argument */		
} kved_flash_driver_t;


typedef enum kved_error_e 
{
	KVED_OK = 0,
	KVED_INVALID_HANDLE = -1,
	KVED_NOT_INITIALIZED = -2,
	KVED_INVALID_KEY = -3,
	KVED_INVALID_INDEX = -4,
	KVED_TABLE_FULL = -5,
	KVED_CORRUPT_TABLE = -6,
	KVED_ERROR = -7,
} kved_error_t;


/**
@brief Writes a new value to the database.
@param[in] data - information about the data to be written
@return true: recording successful.
@return false: error during the recording process.

@code

kved_data_t kv1 = {
	.type = KVED_DATA_TYPE_UINT32,
	.key = "ca1",
	.value.u32 = 0x12345678
};

kved_data_t kv2 = {
	.type = KVED_DATA_TYPE_STRING,
	.key = "ID",
	.value.str ="N01"
};

kved_data_write(&kv1);
kved_data_write(&kv2);

@endcode
*/
kved_error_t kved_data_write(kved_ctrl_t *ctrl, kved_data_t *data);

/**
@brief Retrieves a previously saved value from database.
@param[out] data - Structure where the retrieved value will be stored (type and content)
@return true: read successfully.
@return false: error during the reading process.

@code

kved_data_t kv1 = {
	.key = "ca1",
};

kved_data_t kv2 = {
	.key = "ID",
};

if(kved_data_read(&kv1))
	printf("Value: %d\n",kv1.value.u32);

if(kved_data_read(&kv2))
	printf("Value: %4s\n",kv2.value.str);

@endcode
*/
kved_error_t kved_data_read(kved_ctrl_t *ctrl, kved_data_t *data);

/**
@brief Deletes a previously saved value in the database, if it exists.
@param[in] data - Structure where the retrieved value will be stored (type and content)
@return true: deletion successful.
@return false: error during the erasuring process.

@code

kved_data_t kv1 = {
	.key = "ca1",
};

if(kved_data_delete(&kv1))
	printf("Entry erased");

@endcode
*/
kved_error_t kved_data_delete(kved_ctrl_t *ctrl, kved_data_t *data);

/**
@brief Retrieves a previously saved value from database but using an index.
This function is used in conjunction with @ref kved_first_used_index_get and @ref kved_next_used_index_get
to iterate over the database.
@param[in] index - index of the value to retrieve
@param[out] data - structure where the retrieved value will be stored (type and content)
@return true: read successfully.
@return false: error during reading process.

@code

kved_data_t kv1;

uint16_t index = kved_first_used_index_get();

printf("Database keys:\n");

while(index != KVED_INDEX_NOT_FOUND)
{
	kved_data_read_by_index(index,&data);
	printf("- Key: %s\n",kv1.key);
	index = kved_next_used_index_get(index);
}

@endcode
*/
kved_error_t kved_data_read_by_index(kved_ctrl_t *ctrl, uint16_t index, kved_data_t *data);

/**
@brief Get the first valid index in the database.
@return returns @ref KVED_INDEX_NOT_FOUND (index not found, database empty) or an index value greater than zero
*/
int16_t kved_first_used_index_get(kved_ctrl_t *ctrl);

/**
@brief Given the last index, get the next valid index from the database.
@param[in] last_index - last valid index used
@return return @ref KVED_INDEX_NOT_FOUND when the end of the database is reached or a valid index value (greater than zero)
*/
int16_t kved_next_used_index_get(kved_ctrl_t *ctrl, uint16_t last_index);

/**
@brief Returns the number of database entries (used or not)
@return Number of entries
*/
int16_t kved_total_entries_get(kved_ctrl_t *ctrl);

/**
@brief Returns the number of used database entries
@return Number of entries
*/
int16_t kved_used_entries_get(kved_ctrl_t *ctrl);

/**
@brief Returns the number available entries in the database
@return Number of entries
*/
int16_t kved_free_entries_get(kved_ctrl_t *ctrl);

/** 
 * @brief Returns the number of deleted database entries
 * @return Number of entries
 */
int16_t kved_deleted_entries_get(kved_ctrl_t *ctrl);

/** 
 * @breif compact the database
 * @return KVED_OK if success
 */
kved_error_t kved_compact_database(kved_ctrl_t *ctrl);

/**
@brief Print all values stored in the database
*/
kved_error_t kved_dump(kved_ctrl_t *ctrl);

/**
@brief Format the database, deleting all values
*/
kved_error_t kved_format(kved_ctrl_t *ctrl);

/**
@brief Given a key entry from database, decode it to data type and user key
@param[out] data - structure where data type and user key will be stored
@param[in] key - key entry
*/
kved_error_t kved_key_decode(kved_ctrl_t *ctrl, kved_data_t *data, kved_word_t key);

/**
@brief Given a data type and user key, encode it as a key entry to be written in the database
@param[in] data - user entry
@return Encoded key entry
*/
kved_word_t kved_key_encode(kved_ctrl_t *ctrl, kved_data_t *data);

/**
@brief Initialize the database. Must be called before any use.
*/
kved_ctrl_t *kved_init(kved_flash_driver_t *driver);

/**
@brief Initialize the database. Must be called before any use.
*/
void kved_deinit(kved_ctrl_t *ctrl);

/**
@}
*/
#endif