#ifndef BL808_BSP_COMMON_H
#define BL808_BSP_COMMON_H
#include "sdkconfig.h"
#include "bl808_board.h"

extern void log_start(void);

void bl808_cpu_init(void);
void bl_show_log(void);
void bl_show_flashinfo(void);

#endif