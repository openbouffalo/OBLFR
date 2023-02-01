#ifndef OBLFR_KVED_FLASH_H
#define OBLFR_KVED_FLASH_H


#include "kved.h"

/** 
 * @brief Flash driver configuration
*/
typedef struct oblfr_kved_flash_driver_s {
    uint32_t flash_addr;                        /**< Start Address in Flash to store the configuration */
    uint32_t flash_sector_size;                 /**< Flash Sector Size. Auto Populated */
    uint32_t max_entries;                       /**< Max number of entries in the flash. If 0, then auto calculated from Flash Sector Size. (255 for Flash Sector Size of 4096Bytes) */
} oblfr_kved_flash_driver_t;


kved_flash_driver_t *oblfr_kved_flash_configure(oblfr_kved_flash_driver_t *cfg);
void oblfr_kved_flash_close(kved_flash_driver_t *driver);

#endif // OBLFR_KVED_MEMORY_H