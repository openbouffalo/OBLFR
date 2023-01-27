#ifndef _BOARD_H
#define _BOARD_H

#include "bl702_bsp.h"
#include "bsp_common.h"

void board_init(void);

#ifdef CONFIG_PINMUX_ENABLE_LED1
#define BSP_GPIO_LED1 GPIO_PIN_25
#endif
#ifdef CONFIG_PINMUX_ENABLE_LED2
#define BSP_GPIO_LED2 GPIO_PIN_24
#endif
#ifdef CONFIG_PINMUX_ENABLE_LED3
#define BSP_GPIO_LED3 GPIO_PIN_23
#endif
#ifdef CONFIG_PINMUX_ENABLE_BTN1
#define BSP_GPIO_BTN1 GPIO_PIN_2
#endif

#endif