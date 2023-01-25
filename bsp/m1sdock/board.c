#include <bflb_gpio.h>
#include <bflb_clock.h>
#include "bl808_board.h"
#include "log.h"

pinmux_setup_t pinmux_setup[] = {
    COMMON_PINMUX_SETUP(),
#ifdef CONFIG_PINMUX_ENABLE_LED1
    {
        .pin = GPIO_PIN_8,
        .mode = GPIO_OUTPUT,
        .pull = GPIO_PULLDOWN,
        .drive = GPIO_DRV_1,
    },
#endif
#ifdef CONFIG_PINMUX_ENABLE_BTN1
    {
        .pin = GPIO_PIN_22,
        .mode = GPIO_INPUT,
        .pull = GPIO_PULLUP,
        .drive = GPIO_DRV_1,
    },
#endif
#ifdef CONFIG_PINMUX_ENABLE_BTN2
    {
        .pin = GPIO_PIN_23,
        .mode = GPIO_INPUT,
        .pull = GPIO_PULLUP,
        .drive = GPIO_DRV_1,
    },
#endif
};

void board_init() {
    board_common_init(pinmux_setup, sizeof(pinmux_setup)/sizeof(pinmux_setup_t));
}

