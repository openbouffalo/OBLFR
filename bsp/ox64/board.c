#include <bflb_gpio.h>
#include <bflb_clock.h>
#include "bl808_board.h"
#include "log.h"


pinmux_setup_t pinmux_setup[] = {
    COMMON_PINMUX_SETUP(),
};


void board_init() {
    board_common_init(pinmux_setup, sizeof(pinmux_setup)/sizeof(pinmux_setup_t));

}

