#ifndef OBLFR_KVED_FILE_H
#define OBLFR_KVED_FILE_H

#include "kved.h"


kved_flash_driver_t *oblfr_kved_file_configure();
void oblfr_kved_file_close(kved_flash_driver_t *driver);

#endif // OBLFR_KVED_FILE_H