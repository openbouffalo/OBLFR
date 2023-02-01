#ifndef _BOARD_H
#define _BOARD_H

#include "bl808_bsp.h"
#include "bflb_gpio.h"

void board_init(void);

#ifdef CONFIG_PINMUX_ENABLE_LED1
#define BSP_GPIO_LED1 GPIO_PIN_8
#endif
#ifdef CONFIG_PINMUX_ENABLE_BTN1
#define BSP_GPIO_BTN1 GPIO_PIN_22
#endif
#ifdef CONFIG_PINMUX_ENABLE_BTN2
#define BSP_GPIO_BTN2 GPIO_PIN_23
#endif


#endif