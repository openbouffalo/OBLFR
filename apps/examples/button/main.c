#include "bflb_mtimer.h"
#include "bflb_gpio.h"
#include "board.h"
#include "oblfr_button.h"

#define DBG_TAG "BTN"
#include "log.h"

#if !defined(CONFIG_PINMUX_ENABLE_BTN1) || !defined(CONFIG_PINMUX_ENABLE_BTN2)
#error This Example Requires a Board with 1 or 2 buttons

#else
static void _button_click_cb(void *arg, void *data)
{
    uint8_t btn = (uintptr_t)data;
    LOG_I("BTN%d: BUTTON_CLICK\r\n", btn);
}

static void _button_dblclick_up_cb(void *arg, void *data)
{
    uint8_t btn = (uintptr_t)data;
    LOG_I("BTN%d: BUTTON_DOUBLECLICK\r\n", btn);
}

static void _button_press_down_cb(void *arg, void *data)
{
    uint8_t btn = (uintptr_t)data;
    LOG_I("BTN%d: BUTTON_PRESS_DOWN\r\n", btn);
}
static void _button_press_up_cb(void *arg, void *data)
{
    uint8_t btn = (uintptr_t)data;
    LOG_I("BTN%d: BUTTON_PRESS_UP\r\n", btn);
}
static void _button_press_repeat_cb(void *arg, void *data)
{
    uint8_t btn = (uintptr_t)data;
    LOG_I("BTN%d: BUTTON_PRESS_REPEAT\r\n", btn);
}
static void _button_press_repeat_done_cb(void *arg, void *data)
{
    uint8_t btn = (uintptr_t)data;
    LOG_I("BTN%d: BUTTON_PRESS_REPEAT_DONE\r\n", btn);
}
static void _button_long_press_start_cb(void *arg, void *data)
{
    uint8_t btn = (uintptr_t)data;
    LOG_I("BTN%d: BUTTON_LONG_PRESS_START\r\n", btn);
}
static void _button_long_press_hold_cb(void *arg, void *data)
{
    uint8_t btn = (uintptr_t)data;
    LOG_I("BTN%d: BUTTON_LONG_PRESS_HOLD\r\n", btn);
}
#endif

void app_main(void *arg) {
#ifdef CONFIG_PINMUX_ENABLE_BTN1
    oblfr_button_config_t cfg1 = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 1000,
        .short_press_time = 200,
        .gpio_button_config = {
            .gpio_num  = BSP_GPIO_BTN1,
            .active_level = 0,
        },
    };
    oblfr_button_handle_t s_btn1 = oblfr_button_create(&cfg1);
    oblfr_button_register_cb(s_btn1, BUTTON_SINGLE_CLICK, _button_click_cb, (void *)1);
    oblfr_button_register_cb(s_btn1, BUTTON_DOUBLE_CLICK, _button_dblclick_up_cb, (void *)1);
    oblfr_button_register_cb(s_btn1, BUTTON_PRESS_DOWN, _button_press_down_cb, (void *)1);
    oblfr_button_register_cb(s_btn1, BUTTON_PRESS_UP, _button_press_up_cb, (void *)1);
    oblfr_button_register_cb(s_btn1, BUTTON_PRESS_REPEAT, _button_press_repeat_cb, (void *)1);
    oblfr_button_register_cb(s_btn1, BUTTON_PRESS_REPEAT_DONE, _button_press_repeat_done_cb, (void *)1);
    oblfr_button_register_cb(s_btn1, BUTTON_LONG_PRESS_START, _button_long_press_start_cb, (void *)1);
    oblfr_button_register_cb(s_btn1, BUTTON_LONG_PRESS_HOLD, _button_long_press_hold_cb, (void *)1);
#endif
#ifdef CONFIG_PINMUX_ENABLE_BTN2
    oblfr_button_config_t cfg2 = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = 1000,
        .short_press_time = 200,
        .gpio_button_config = {
            .gpio_num  = BSP_GPIO_BTN2,
            .active_level = 0,
        },
    };
    oblfr_button_handle_t s_btn2 = oblfr_button_create(&cfg2);
    oblfr_button_register_cb(s_btn2, BUTTON_SINGLE_CLICK, _button_click_cb, (void *)2);
    oblfr_button_register_cb(s_btn2, BUTTON_DOUBLE_CLICK, _button_dblclick_up_cb, (void *)2);
    oblfr_button_register_cb(s_btn2, BUTTON_PRESS_DOWN, _button_press_down_cb, (void *)2);
    oblfr_button_register_cb(s_btn2, BUTTON_PRESS_UP, _button_press_up_cb, (void *)2);
    oblfr_button_register_cb(s_btn2, BUTTON_PRESS_REPEAT, _button_press_repeat_cb, (void *)2);
    oblfr_button_register_cb(s_btn2, BUTTON_PRESS_REPEAT_DONE, _button_press_repeat_done_cb, (void *)2);
    oblfr_button_register_cb(s_btn2, BUTTON_LONG_PRESS_START, _button_long_press_start_cb, (void *)2);
    oblfr_button_register_cb(s_btn2, BUTTON_LONG_PRESS_HOLD, _button_long_press_hold_cb, (void *)2);
#endif


   while (true) {
        bflb_mtimer_delay_ms(1000);
    }
}