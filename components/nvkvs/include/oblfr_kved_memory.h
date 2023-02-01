#ifndef OBLFR_KVED_MEMORY_H
#define OBLFR_KVED_MEMORY_H

#include "kved.h"


kved_flash_driver_t *oblfr_kved_memory_configure();
void oblfr_kved_memory_close(kved_flash_driver_t *driver);

#endif // OBLFR_KVED_MEMORY_H