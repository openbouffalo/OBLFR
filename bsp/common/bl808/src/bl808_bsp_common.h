#ifndef BL808_BSP_COMMON_H
#define BL808_BSP_COMMON_H
#include "sdkconfig.h"
#include "bl808_board.h"

extern void log_start(void);

void bl808_cpu_init(void);
void bl_show_log(void);
void bl_show_flashinfo(void);


#if defined(CONFIG_JTAG_DEBUG)
#ifdef CONFIG_JTAG_DEBUG_PINS_GRP0
#define JTAG_TMS_GPIOPIN GPIO_PIN_6
#define JTAG_TDO_GPIOPIN GPIO_PIN_7
#define JTAG_TCK_GPIOPIN GPIO_PIN_12
#define JTAG_TDI_GPIOPIN GPIO_PIN_13
#else
#error Unknown JTAG debug pins
#endif
#endif

#endif