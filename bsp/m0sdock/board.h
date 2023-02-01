#ifndef _BOARD_H
#define _BOARD_H

#include "bl616_bsp.h"
#include "bsp_common.h"
#include "sdkconfig.h"

void board_init(void);

#ifdef CONFIG_PINMUX_ENABLE_LED1
#define BSP_GPIO_LED1 GPIO_PIN_27
#endif
#ifdef CONFIG_PINMUX_ENABLE_LED2
#define BSP_GPIO_LED2 GPIO_PIN_28
#endif

#endif