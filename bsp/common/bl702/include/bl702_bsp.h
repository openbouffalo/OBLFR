#ifndef BL702_BSP_H
#define BL702_BSP_H
#include <stdint.h>
#include "bsp_common.h"
#include "sdkconfig.h"

void board_common_init(pinmux_setup_t *pinmux_setup, uint32_t len);

#endif