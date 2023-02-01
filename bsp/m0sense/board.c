#include <bflb_gpio.h>
#include <bflb_clock.h>
#include "board.h"
#include "log.h"


pinmux_setup_t pinmux_setup[] = {
    COMMON_PINMUX_SETUP()
#ifdef CONFIG_PINMUX_ENABLE_LED1
    {
        .pin = BSP_GPIO_LED1,
        .mode = GPIO_OUTPUT,
        .pull = GPIO_PULLUP,
        .drive = GPIO_DRV_1,
    },
#endif
#ifdef CONFIG_PINMUX_ENABLE_LED2
    {
        .pin = BSP_GPIO_LED2,
        .mode = GPIO_OUTPUT,
        .pull = GPIO_PULLUP,
        .drive = GPIO_DRV_1,
    },
#endif
#ifdef CONFIG_PINMUX_ENABLE_LED3
    {
        .pin = BSP_GPIO_LED3,
        .mode = GPIO_OUTPUT,
        .pull = GPIO_PULLUP,
        .drive = GPIO_DRV_1,
    },
#endif
#ifdef CONFIG_PINMUX_ENABLE_BTN1
    {
        .pin = BSP_GPIO_BTN1,
        .mode = GPIO_INPUT,
        .pull = GPIO_PULLUP,
        .drive = GPIO_DRV_1,
    },
#endif
};


void board_init() {
    board_common_init(pinmux_setup, sizeof(pinmux_setup)/sizeof(pinmux_setup_t));
    printf("board init done\r\n");
}

