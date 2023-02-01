#include "bflb_mtimer.h"
#include "bflb_gpio.h"
#include "board.h"
#include "oblfr_indicator.h"

#define DBG_TAG "TMR"
#include "log.h"

#if !defined(CONFIG_PINMUX_ENABLE_LED1)
#error "This Example Requires a Board with 1 LED"
#else 

void app_main(void *arg) {
    const oblfr_indicator_config_t led_cfg = { };
    oblfr_indicator_handle_t oblfr_indicator = oblfr_indicator_create(BSP_GPIO_LED1, &led_cfg);
    oblfr_err_t ret = oblfr_indicator_start(oblfr_indicator, BLINK_FACTORY_RESET);
    if (ret != OBLFR_OK) {
        LOG_E("Failed to start LED indicator\r\n");
    }
    int i = 0;

    while (true) {
        bflb_mtimer_delay_ms(5000);
        oblfr_indicator_stop(oblfr_indicator, i);
        i++;
        if (i >= BLINK_MAX) {
            i = 0;
        }
        LOG_I("Changing to Next Indicator Pattern %d\r\n", i);
        ret = oblfr_indicator_start(oblfr_indicator, i);
        if (ret != OBLFR_OK) {
            LOG_E("Failed to start LED indicator\r\n");
        }
    }
}
#endif