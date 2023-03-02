#ifndef BL808_BSP_COMMON_H
#define BL808_BSP_COMMON_H
#include "sdkconfig.h"
#include "bl808_bsp.h"

extern void log_start(void);

void bl808_cpu_init(void);

#define CONFIG_D0_FLASH_ADDR             0x100000
#define CONFIG_LP_FLASH_ADDR             0x200000

#endif