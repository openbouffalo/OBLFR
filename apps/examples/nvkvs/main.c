#include <bflb_mtimer.h>
#include "board.h"
#include "FreeRTOS.h"
#include "task.h"
#include "oblfr_nvkvs.h"

#define DBG_TAG "NVKVSDEMO"
#include "log.h"


#ifdef KVED_DEBUG
#define KVED_RUN_DUMP(x) \
    oblfr_nvkvs_dump(x) 
#else
#define KVED_RUN_DUMP(x)
#endif

#define CHECK_ERR(x) \
    { \
        oblfr_err_t err = x; \
        if (err != OBLFR_OK) { \
            LOG_E("Error %x\r\n", err); \
        } \
    }

void app_main(void *arg) {
    oblfr_kved_flash_driver_t kved_flash_drv = {
        .flash_addr = 0xF0000,
    };

    oblfr_nvkvs_cfg_t nvkvs_cfg = {
        .storage = OBLFR_NVKVS_STORAGE_FLASH,
//        .storage = OBLFR_NVKVS_STORAGE_RAM,
        .drv_cfg.flash = &kved_flash_drv,
    };

    oblfr_nvkvs_handle_t *handle = oblfr_nvkvs_init(&nvkvs_cfg);

    if (handle == NULL) {
        LOG_E("Failed to init NVKVS\r\n");
        return;
    }

	// uint8_t
	{
        LOG_I("Test U8\r\n");
        uint8_t v = 34;
        CHECK_ERR(oblfr_nvkvs_set_u8(handle, "U8", v));
        uint8_t v2;
        CHECK_ERR(oblfr_nvkvs_get_u8(handle, "U8", &v2));
        LOG_I("U8: %d = %d\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
	// int8_t
	{
        LOG_I("Test I8\r\n");
        int8_t v = 21;
        CHECK_ERR(oblfr_nvkvs_set_i8(handle, "I8", v));
        int8_t v2;
        CHECK_ERR(oblfr_nvkvs_get_i8(handle, "I8", &v2));
        LOG_I("i8: %d = %d\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
	// uint16_t
	{
        LOG_I("Test U16\r\n");
        uint16_t v = 5525;
        CHECK_ERR(oblfr_nvkvs_set_u16(handle, "U16", v));
        uint16_t v2;
        CHECK_ERR(oblfr_nvkvs_get_u16(handle, "U16", &v2));
        LOG_I("U16: %d = %d\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
	// int16_t
	{
        LOG_I("Test I16\r\n");
        int16_t v = 4432;
        CHECK_ERR(oblfr_nvkvs_set_i16(handle, "I16", v));
        int16_t v2;
        CHECK_ERR(oblfr_nvkvs_get_i16(handle, "I16", &v2));
        LOG_I("I16: %d = %d\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
	// uint32_t
	{
        LOG_I("Test U32\r\n");
        uint32_t v = 75333;
        CHECK_ERR(oblfr_nvkvs_set_u32(handle, "U32", v));
        uint32_t v2;
        CHECK_ERR(oblfr_nvkvs_get_u32(handle, "U32", &v2));
        LOG_I("U32: %d = %d\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
	// int32_t
	{
        LOG_I("Test I32\r\n");
        int32_t v = 86232;
        CHECK_ERR(oblfr_nvkvs_set_i32(handle, "I32", v));
        int32_t v2;
        CHECK_ERR(oblfr_nvkvs_get_i32(handle, "I32", &v2));
        LOG_I("I32: %d = %d\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
	// uint64_t
	{
        LOG_I("Test U64\r\n");
        uint64_t v = 664375333;
        CHECK_ERR(oblfr_nvkvs_set_u64(handle, "U64", v));
        uint64_t v2;
        CHECK_ERR(oblfr_nvkvs_get_u64(handle, "U64", &v2));
        LOG_I("U64: %ld = %ld\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
	// int64_t
	{
        LOG_I("Test I64\r\n");
        int64_t v = 223223434;
        CHECK_ERR(oblfr_nvkvs_set_i64(handle, "I64", v));
        int64_t v2;
        CHECK_ERR(oblfr_nvkvs_get_i64(handle, "I64", &v2));
        LOG_I("I64: %ld = %ld\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
	// FLOAT
	{
        LOG_I("Test FLOAT\r\n");
        float v = 3.14159265;
        CHECK_ERR(oblfr_nvkvs_set_float(handle, "FLOAT", v));
        float v2;
        CHECK_ERR(oblfr_nvkvs_get_float(handle, "FLOAT", &v2));
        LOG_I("FLOAT: %f = %f\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
    // DOUBLE
    {
        LOG_I("Test DOUBLE\r\n");
        double v = 3.14159265;
        CHECK_ERR(oblfr_nvkvs_set_double(handle, "DOUBLE", v));
        double v2;
        CHECK_ERR(oblfr_nvkvs_get_double(handle, "DOUBLE", &v2));
        LOG_I("DOUBLE: %f = %f\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }
    // STRING
    {
        LOG_I("Test STRING\r\n");
        char *v = "Hello World, this is a really long string";
        CHECK_ERR(oblfr_nvkvs_set_string(handle, "STRING", v));
        char v2[128];
        CHECK_ERR(oblfr_nvkvs_get_string(handle, "STRING", (char *)&v2));
        LOG_I("STRING: %s = %s\r\n", v, v2);
        KVED_RUN_DUMP(ctrl);
    }

    /* force a Index Table Rollover */
    {
        LOG_I("Force a Index Table Rollover.. This might take a while\r\n");
        for (uint16_t i = 0; i < 300; i++) {

            char key[16];
            sprintf(key, "KEY");
            CHECK_ERR(oblfr_nvkvs_set_u16(handle, key, i));
            uint16_t v;
            CHECK_ERR(oblfr_nvkvs_get_u16(handle, key, &v));
            if (v != i) {
                LOG_E("Error: %d != %d\r\n", v, i);
            }
        }
        LOG_I("Done\r\n");
        //oblfr_nvkvs_dump(handle);
    }
    /* Delete a entry */
    {
        LOG_I("Delete a entry\r\n");
        char key[16] = "KEY";
        CHECK_ERR(oblfr_nvkvs_delete(handle, key));
        uint16_t v2;
        int err = oblfr_nvkvs_get_u16(handle, key, &v2);
        if (err == OBLFR_ERR_ERROR) {
            LOG_I("Entry deleted\r\n");
        } else {
            LOG_E("Error: %d\r\n", err);
        }
    }
    // iterate over all keys 
    {
        LOG_I("Iterate over all keys\r\n");
        uint16_t iter = oblfr_nvkvs_iter_init(handle);
        while (iter != 0) {
            oblfr_nvkvs_data_t data;
            oblfr_nvkvs_get_item(handle, iter, &data);
            LOG_I("Key: %s\r\n", data.key);
            switch (data.type) {
                case OBLFR_NVKVS_DATA_TYPE_UINT8:
                    LOG_I("\tU8: %d\r\n", data.value.u8);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_INT8:
                    LOG_I("\tI8: %d\r\n", data.value.i8);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_UINT16:
                    LOG_I("\tU16: %d\r\n", data.value.u16);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_INT16:
                    LOG_I("\tI16: %d\r\n", data.value.i16);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_UINT32:
                    LOG_I("\tU32: %d\r\n", data.value.u32);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_INT32:
                    LOG_I("\tI32: %d\r\n", data.value.i32);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_UINT64:
                    LOG_I("\tU64: %ld\r\n", data.value.u64);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_INT64:
                    LOG_I("\tI64: %ld\r\n", data.value.i64);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_FLOAT:
                    LOG_I("\tFLOAT: %f\r\n", data.value.flt);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_DOUBLE:
                    LOG_I("\tDOUBLE: %f\r\n", data.value.dbl);
                    break;
                case OBLFR_NVKVS_DATA_TYPE_STRING:
                    LOG_I("\tSTRING: %s\r\n", data.value.str);
                    break;
            }
            iter = oblfr_nvkvs_iter_next(handle, iter);
        }
        LOG_I("Done\r\n");
    }
    /* some statistics */
    {
        LOG_I("Statistics\r\n");
        LOG_I("Total Size: %d\r\n", oblfr_nvkvs_get_size(handle));
        LOG_I("Used Size: %d\r\n", oblfr_nvkvs_used_entries(handle));
        LOG_I("Free Size: %d\r\n", oblfr_nvkvs_free_entries(handle));
        LOG_I("Deleted Entries: %d\r\n", oblfr_nvkvs_deleted_entries(handle));
    }
    /* compact */
    {
        LOG_I("Compact\r\n");
        vTaskDelay(100);
        oblfr_nvkvs_dump(handle);
        vTaskDelay(100);
        oblfr_nvkvs_compact(handle);
        vTaskDelay(100);
        oblfr_nvkvs_dump(handle);
    }
    {
        LOG_I("Final Statistics\r\n");
        LOG_I("Total Size: %d\r\n", oblfr_nvkvs_get_size(handle));
        LOG_I("Used Size: %d\r\n", oblfr_nvkvs_used_entries(handle));
        LOG_I("Free Size: %d\r\n", oblfr_nvkvs_free_entries(handle));
        LOG_I("Deleted Entries: %d\r\n", oblfr_nvkvs_deleted_entries(handle));
    }

   while (true) {
        bflb_mtimer_delay_ms(1000);
        LOG_I("Complete!\r\n");
    }
}