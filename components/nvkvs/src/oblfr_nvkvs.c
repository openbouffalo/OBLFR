#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "oblfr_common.h"
#include "kved.h"
#include "oblfr_nvkvs.h"
#include "oblfr_kved_flash.h"
#include "oblfr_kved_memory.h"

#define DBG_TAG "NVKVS"
#include "log.h"

typedef struct oblfr_nvkvs_handle_s
{
    kved_flash_driver_t *storage_driver;
    oblfr_nvkvs_storage_t storage;
    kved_ctrl_t *kved_ctrl;
} oblfr_nvkvs_handle_t;

oblfr_nvkvs_handle_t *oblfr_nvkvs_init(const oblfr_nvkvs_cfg_t *cfg)
{
    if (cfg == NULL)
    {
        LOG_E("Invalid configuration");
        return NULL;
    }

    oblfr_nvkvs_handle_t *handle = malloc(sizeof(oblfr_nvkvs_handle_t));
    if (handle == NULL)
    {
        LOG_E("Failed to allocate memory for handle");
        return NULL;
    }

    switch (cfg->storage)
    {
#ifdef CONFIG_COMPONENT_NVKVS_FLASH_BACKEND
    case OBLFR_NVKVS_STORAGE_FLASH:
        handle->storage_driver = oblfr_kved_flash_configure(cfg->drv_cfg.flash);
        break;
#endif
#ifdef CONFIG_COMPONENT_NVKVS_MEM_BACKEND
    case OBLFR_NVKVS_STORAGE_RAM:
        handle->storage_driver = oblfr_kved_memory_configure();
        break;
#endif
    default:
        LOG_E("Invalid storage type");
        return NULL;
    }
    handle->storage = cfg->storage;

    handle->kved_ctrl = kved_init(handle->storage_driver);
    if (handle->kved_ctrl == NULL)
    {
        LOG_E("Failed to initialize KVED");
        oblfr_nvkvs_deinit(handle);
        return NULL;
    }

    return handle;
}

oblfr_err_t oblfr_nvkvs_deinit(oblfr_nvkvs_handle_t *handle)
{
    if (handle == NULL)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_deinit(handle->kved_ctrl);
    switch (handle->storage)
    {
    case OBLFR_NVKVS_STORAGE_FLASH:
        oblfr_kved_flash_close(handle->storage_driver);
        break;
    case OBLFR_NVKVS_STORAGE_RAM:
        oblfr_kved_memory_close(handle->storage_driver);
        break;
    default:
        return OBLFR_ERR_INVALID;
    }
    free(handle);

    return OBLFR_OK;
}

oblfr_err_t oblfr_nvkvs_dump(oblfr_nvkvs_handle_t *handle)
{
    if (handle == NULL)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_dump(handle->kved_ctrl);
    return OBLFR_OK;
}

oblfr_err_t oblfr_nvkvs_compact(oblfr_nvkvs_handle_t *handle) {
    if (handle == NULL)
    {
        return OBLFR_ERR_INVALID;
    }
    if (kved_compact_database(handle->kved_ctrl) != KVED_OK) {
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}


oblfr_err_t oblfr_nvkvs_set_u8(oblfr_nvkvs_handle_t *handle, const char *key, uint8_t value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_UINT8,
        .value.u8 = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_u8(oblfr_nvkvs_handle_t *handle, const char *key, uint8_t *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_UINT8,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.u8;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_i8(oblfr_nvkvs_handle_t *handle, const char *key, int8_t value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_INT8,
        .value.i8 = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_i8(oblfr_nvkvs_handle_t *handle, const char *key, int8_t *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_INT8,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.i8;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_u16(oblfr_nvkvs_handle_t *handle, const char *key, uint16_t value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_UINT16,
        .value.u16 = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_u16(oblfr_nvkvs_handle_t *handle, const char *key, uint16_t *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_UINT16,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.u16;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_i16(oblfr_nvkvs_handle_t *handle, const char *key, int16_t value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_INT16,
        .value.i16 = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_i16(oblfr_nvkvs_handle_t *handle, const char *key, int16_t *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_INT16,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.i16;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_u32(oblfr_nvkvs_handle_t *handle, const char *key, uint32_t value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_UINT32,
        .value.u32 = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_u32(oblfr_nvkvs_handle_t *handle, const char *key, uint32_t *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_UINT32,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.u32;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_i32(oblfr_nvkvs_handle_t *handle, const char *key, int32_t value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_INT32,
        .value.i32 = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_i32(oblfr_nvkvs_handle_t *handle, const char *key, int32_t *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_INT32,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.i32;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_u64(oblfr_nvkvs_handle_t *handle, const char *key, uint64_t value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_UINT64,
        .value.u64 = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_u64(oblfr_nvkvs_handle_t *handle, const char *key, uint64_t *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_UINT64,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.u64;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_i64(oblfr_nvkvs_handle_t *handle, const char *key, int64_t value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_INT64,
        .value.i64 = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_i64(oblfr_nvkvs_handle_t *handle, const char *key, int64_t *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_INT64,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.i64;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_float(oblfr_nvkvs_handle_t *handle, const char *key, float value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_FLOAT,
        .value.flt = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_float(oblfr_nvkvs_handle_t *handle, const char *key, float *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_FLOAT,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.flt;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_double(oblfr_nvkvs_handle_t *handle, const char *key, double value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_DOUBLE,
        .value.dbl = value};
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_double(oblfr_nvkvs_handle_t *handle, const char *key, double *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_DOUBLE,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    *value = kv1.value.dbl;
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_set_string(oblfr_nvkvs_handle_t *handle, const char *key, const char *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    if (strlen(value) > CONFIG_COMPONENT_NVKVS_MAX_STRING_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_STRING,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    strncpy((char *)kv1.value.str, value, CONFIG_COMPONENT_NVKVS_MAX_STRING_SIZE);
    kved_error_t err = kved_data_write(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_write failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}
oblfr_err_t oblfr_nvkvs_get_string(oblfr_nvkvs_handle_t *handle, const char *key, char *value)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        LOG_E("key is too long");
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_STRING,
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_read(handle->kved_ctrl, &kv1);
    if (err != KVED_OK)
    {
        LOG_E("kved_data_read failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    strncpy(value, (char *)kv1.value.str, CONFIG_COMPONENT_NVKVS_MAX_STRING_SIZE);
    return OBLFR_OK;
}

oblfr_err_t oblfr_nvkvs_delete(oblfr_nvkvs_handle_t *handle, const char *key)
{
    if (strlen(key) > KVED_MAX_KEY_SIZE)
    {
        return OBLFR_ERR_INVALID;
    }
    kved_data_t kv1 = {
    };
    strncpy((char *)kv1.key, key, KVED_MAX_KEY_SIZE);
    kved_error_t err = kved_data_delete(handle->kved_ctrl, &kv1);
    if (err != KVED_OK) {
        LOG_E("kved_data_delete failed %d\r\n", err);
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}

uint16_t oblfr_nvkvs_get_size(oblfr_nvkvs_handle_t *handle) {
    return kved_total_entries_get(handle->kved_ctrl);
}

uint16_t oblfr_nvkvs_used_entries(oblfr_nvkvs_handle_t *handle) {
    return kved_used_entries_get(handle->kved_ctrl);
}

uint16_t oblfr_nvkvs_free_entries(oblfr_nvkvs_handle_t *handle) {
    return kved_free_entries_get(handle->kved_ctrl);
}

uint16_t oblfr_nvkvs_deleted_entries(oblfr_nvkvs_handle_t *handle) {
    return kved_deleted_entries_get(handle->kved_ctrl);
}

int16_t oblfr_nvkvs_iter_init(oblfr_nvkvs_handle_t *handle) {
    return kved_first_used_index_get(handle->kved_ctrl);
}

int16_t oblfr_nvkvs_iter_next(oblfr_nvkvs_handle_t *handle, int16_t iter) {
    return kved_next_used_index_get(handle->kved_ctrl, iter);
}

oblfr_err_t oblfr_nvkvs_get_item(oblfr_nvkvs_handle_t *handle, uint16_t index, oblfr_nvkvs_data_t *data) {
    kved_data_t kv1;
    if (kved_data_read_by_index(handle->kved_ctrl, index, &kv1) != KVED_OK) {
        LOG_W("kved_data_read_by_index failed\r\n");
        return OBLFR_ERR_ERROR;
    }
    data->type = kv1.type;
    strncpy(data->key, (char *)kv1.key, KVED_MAX_KEY_SIZE);
    switch (kv1.type) {
        case KVED_DATA_TYPE_INT8:
            data->value.i8 = kv1.value.i8;
            break;
        case KVED_DATA_TYPE_UINT8:
            data->value.u8 = kv1.value.u8;
            break;
        case KVED_DATA_TYPE_INT16:
            data->value.i16 = kv1.value.i16;
            break;
        case KVED_DATA_TYPE_UINT16:
            data->value.u16 = kv1.value.u16;
            break;
        case KVED_DATA_TYPE_INT32:
            data->value.i32 = kv1.value.i32;
            break;
        case KVED_DATA_TYPE_UINT32:
            data->value.u32 = kv1.value.u32;
            break;
        case KVED_DATA_TYPE_INT64:
            data->value.i64 = kv1.value.i64;
            break;
        case KVED_DATA_TYPE_UINT64:
            data->value.u64 = kv1.value.u64;
            break;
        case KVED_DATA_TYPE_FLOAT:
            data->value.flt = kv1.value.flt;
            break;
        case KVED_DATA_TYPE_DOUBLE:
            data->value.dbl = kv1.value.dbl;
            break;
        case KVED_DATA_TYPE_STRING:
            LOG_I("KVED_DATA_TYPE_STRING %s\r\n", kv1.value.str);
            strncpy((char *)data->value.str, (char *)kv1.value.str, CONFIG_COMPONENT_NVKVS_MAX_STRING_SIZE);
            break;
    }
    return OBLFR_OK;
}