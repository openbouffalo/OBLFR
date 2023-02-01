// Copyright 2023 Justin Hammond
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Modeled After the esp-idf timer implementation

#ifndef OBLFR_NVKVS_H
#define OBLFR_NVKVS_H

#include <stdint.h>
#include <stdbool.h>
#include "oblfr_common.h"
#include "kved.h"
#include "oblfr_kved_flash.h"
#include "oblfr_kved_memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NVKVS storage types
 */
typedef enum {
    OBLFR_NVKVS_STORAGE_FLASH,  /**< Flash storage - Need to provide a oblfr_kved_flash_driver_t configuration*/
    OBLFR_NVKVS_STORAGE_RAM,    /**< RAM storage */
} oblfr_nvkvs_storage_t;

/**
 * @brief NVKVS configuration
 */
typedef struct  {
    oblfr_nvkvs_storage_t storage;          /**< Storage type */
    union  {
        oblfr_kved_flash_driver_t *flash;   /**< Flash driver configuration */
    } drv_cfg;
} oblfr_nvkvs_cfg_t;

/**
 * @brief NVKVS handle
 */
typedef struct oblfr_nvkvs_handle_s oblfr_nvkvs_handle_t;

/**
 * @brief NVKVS data types
 */
typedef enum oblfr_nvkvs_data_types_e
{
	OBLFR_NVKVS_DATA_TYPE_UINT8 = 0, /**< 8 bits, unsigned */
	OBLFR_NVKVS_DATA_TYPE_INT8,      /**< 8 bits, signed */
	OBLFR_NVKVS_DATA_TYPE_UINT16,    /**< 16 bits, unsigned */
	OBLFR_NVKVS_DATA_TYPE_INT16,     /**< 16 bits, signed */
	OBLFR_NVKVS_DATA_TYPE_UINT32,    /**< 32 bits, unsigned */
	OBLFR_NVKVS_DATA_TYPE_INT32,     /**< 32 bits, com sinal */
	OBLFR_NVKVS_DATA_TYPE_FLOAT,     /**< Single precision floating point (float) */
	OBLFR_NVKVS_DATA_TYPE_STRING,    /**< String up to @ref CONFIG_COMPONENT_NVKVS_MAX_STRING_SIZE bytes, excluding terminator */
	OBLFR_NVKVS_DATA_TYPE_UINT64,    /**< 64 bits, signed */
	OBLFR_NVKVS_DATA_TYPE_INT64,     /**< 64 bits, unsigned */
	OBLFR_NVKVS_DATA_TYPE_DOUBLE,    /**< Double precision floating point (double) */
} oblfr_nvkvs_data_types_t;

/**
 * @brief NVKVS data values
 */
typedef union oblfr_nvkvs_value_u
{
	uint8_t u8;                                             /**< unsigned 8 bits value */
	int8_t i8;                                              /**< signed 8 bits value */
	uint16_t u16;                                           /**< unsigned 16 bits value */
	int16_t i16;                                            /**< signed 16 bits value */
	uint32_t u32;                                           /**< unsigned 32 bits value */
	int32_t i32;                                            /**< signed 32 bits value */
	float flt;                                              /**< single precision float */
	uint8_t str[CONFIG_COMPONENT_NVKVS_MAX_STRING_SIZE];    /**< string */
	uint64_t u64;                                           /**< unsigned 64 bits value */
	int64_t i64;                                            /**< signed 64 bits value */
	double dbl;                                             /**< double precision float */
} oblfr_nvkvs_value_t;

/**
 * @brief NVKVS data structure
 */
typedef struct oblfr_nvkvs_data_s {
    char key[8];
    oblfr_nvkvs_data_types_t type;
    oblfr_nvkvs_value_t value;
} oblfr_nvkvs_data_t;


/**
 * @brief NVKVS initialization
 * 
 * Initilize the NVKVS storage
 * @param in cfg NVKVS configuration
 * @return oblfr_nvkvs_handle_t handle for use with other NVKVS functions or NULL on error
 */
oblfr_nvkvs_handle_t *oblfr_nvkvs_init(const oblfr_nvkvs_cfg_t *cfg);

/**
 * @brief NVKVS deinitialization
 * 
 * Deinitilize the NVKVS storage
 * @param in handle NVKVS handle
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid or a invalid NVKVS Storage Driver was used
 */
oblfr_err_t oblfr_nvkvs_deinit(oblfr_nvkvs_handle_t *handle);

/**
 * @brief Dump the NVKVS storage to stdout for debugging
 * 
 * @param in handle NVKVS handle
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 */
oblfr_err_t oblfr_nvkvs_dump(oblfr_nvkvs_handle_t *handle);

/**
 * @brief Compact the NVKVS storage
 * 
 * This function will compact the NVKVS storage, removing deleted entries
 * You don't normally need to call this function, as it will automatically be compacted when the database
 * reaches capacity
 * @param in handle NVKVS handle
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the storage was not compacted
 */
oblfr_err_t oblfr_nvkvs_compact(oblfr_nvkvs_handle_t *handle);

/**
 * @brief Get the maximum number of entries the storage can hold
 * 
 * This includes Deleted entries as well
 * 
 * @param in handle NVKVS handle
 * @return  Number of entries the storage can hold
 */
uint16_t oblfr_nvkvs_get_size(oblfr_nvkvs_handle_t *handle);

/**
 * @brief Get the number of used entries in the storage
 * 
 * @param in handle NVKVS handle
 * @return  Number of used entries in the storage
 */
uint16_t oblfr_nvkvs_used_entries(oblfr_nvkvs_handle_t *handle);

/** 
 * @brief Get the number of free entries in the storage.
 * 
 * (includes deleted entries as these can be recycled by a database compact)
 * 
 * @param in handle NVKVS handle
 * @return  Number of free entries in the storage
*/
uint16_t oblfr_nvkvs_free_entries(oblfr_nvkvs_handle_t *handle);

/**
 * @brief Get the number of deleted entries in the storage
 * 
 * @param in handle NVKVS handle
 * @return  Number of deleted entries in the storage
*/
uint16_t oblfr_nvkvs_deleted_entries(oblfr_nvkvs_handle_t *handle);

/**
 * @brief obtain a index to the first entry in the database
 * 
 * @param in handle NVKVS handle
 * @return  index to the first entry in the database
 *          0 if the database is empty
 */
int16_t oblfr_nvkvs_iter_init(oblfr_nvkvs_handle_t *handle);

/**
 * @brief obtain a index to the next entry in the database
 * 
 * @param in handle NVKVS handle
 * @param in iter index to the current entry in the database
 * @return  index to the next entry in the database
 *         0 if the database is empty
 */
int16_t oblfr_nvkvs_iter_next(oblfr_nvkvs_handle_t *handle, int16_t iter);

/**
 * @brief Get the data for a given index
 * 
 * @param in handle NVKVS handle
 * @param in index index to the entry in the database
 * @param out data pointer to a data structure to store the data
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_ERROR if the index was invalid
 */
oblfr_err_t oblfr_nvkvs_get_item(oblfr_nvkvs_handle_t *handle, uint16_t index, oblfr_nvkvs_data_t *data);

/**
 * @brief Save a uint8_t value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_u8(oblfr_nvkvs_handle_t *handle, const char *key, uint8_t value);

/** 
 * @brief Get a uint8_t value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a uint8_t to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_u8(oblfr_nvkvs_handle_t *handle, const char *key, uint8_t *value);

/**
 * @brief Save a int8_t value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_i8(oblfr_nvkvs_handle_t *handle, const char *key, int8_t value);

/**
 * @brief Get a int8_t value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a int8_t to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_i8(oblfr_nvkvs_handle_t *handle, const char *key, int8_t *value);

/**
 * @brief Save a uint16_t value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_u16(oblfr_nvkvs_handle_t *handle, const char *key, uint16_t value);

/**
 * @brief Get a uint16_t value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a uint16_t to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_u16(oblfr_nvkvs_handle_t *handle, const char *key, uint16_t *value);

/**
 * @brief Save a int16_t value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_i16(oblfr_nvkvs_handle_t *handle, const char *key, int16_t value);

/** 
 * @brief Get a int16_t value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a int16_t to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_i16(oblfr_nvkvs_handle_t *handle, const char *key, int16_t *value);

/**
 * @brief Save a uint32_t value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_u32(oblfr_nvkvs_handle_t *handle, const char *key, uint32_t value);

/**
 * @brief Get a uint32_t value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a uint32_t to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_u32(oblfr_nvkvs_handle_t *handle, const char *key, uint32_t *value);

/**
 * @brief Save a int32_t value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_i32(oblfr_nvkvs_handle_t *handle, const char *key, int32_t value);

/**
 * @brief Get a int32_t value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a int32_t to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_i32(oblfr_nvkvs_handle_t *handle, const char *key, int32_t *value);

/**
 * @brief Save a uint64_t value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_u64(oblfr_nvkvs_handle_t *handle, const char *key, uint64_t value);

/**
 * @brief Get a uint64_t value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a uint64_t to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved 
 */
oblfr_err_t oblfr_nvkvs_get_u64(oblfr_nvkvs_handle_t *handle, const char *key, uint64_t *value);

/**
 * @brief Save a int64_t value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_i64(oblfr_nvkvs_handle_t *handle, const char *key, int64_t value);

/**
 * @brief Get a int64_t value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a int64_t to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_i64(oblfr_nvkvs_handle_t *handle, const char *key, int64_t *value);

/**
 * @brief Save a float value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_float(oblfr_nvkvs_handle_t *handle, const char *key, float value);

/**
 * @brief Get a float value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a float to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_float(oblfr_nvkvs_handle_t *handle, const char *key, float *value);

/**
 * @brief Save a double value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_double(oblfr_nvkvs_handle_t *handle, const char *key, double value);

/**
 * @brief Get a double value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a double to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_double(oblfr_nvkvs_handle_t *handle, const char *key, double *value);

/**
 * @brief Save a string value to the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to store the value under
 * @param in value value to store
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be stored
 */
oblfr_err_t oblfr_nvkvs_set_string(oblfr_nvkvs_handle_t *handle, const char *key, const char *value);

/**
 * @brief Get a string value from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to retrieve the value from
 * @param out value pointer to a string to store the value
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the value could not be retrieved
 */
oblfr_err_t oblfr_nvkvs_get_string(oblfr_nvkvs_handle_t *handle, const char *key, char *value);

/**
 * @brief Delete a key from the database
 * 
 * @param in handle NVKVS handle
 * @param in key key to delete
 * @return  OBLFR_OK on success
 *          OBLFR_ERR_INVALID if the handle was invalid
 *          OBLFR_ERR_ERROR if the key could not be deleted
 *          OBLFR_ERR_NOT_FOUND if the key was not found
 */
oblfr_err_t oblfr_nvkvs_delete(oblfr_nvkvs_handle_t *handle, const char *key);

#ifdef __cplusplus
}
#endif

#endif