#include "bflb_mtimer.h"
#include "bflb_gpio.h"
#include "board.h"
#include "oblfr_timer.h"

#define DBG_TAG "TMR"
#include "log.h"

void timer1(void *arg) {
    LOG_I("timer1\r\n");
}

void timer2(void *arg) {
    LOG_I("timer2\r\n");
}

void one_shot(void *arg) {
    LOG_I("one-shot\r\n");
}

void app_main(void *arg) {
 
    oblfr_timer_create_args_t timer1_cfg = {
        .callback = timer1,
        .arg = NULL,
        .name = "Timer1",
    };
    oblfr_timer_t timer1;
    oblfr_timer_create(&timer1_cfg, &timer1);
    oblfr_timer_start_periodic(timer1, 2000);

    oblfr_timer_create_args_t timer2_cfg = {
        .callback = timer2,
        .arg = NULL,
        .name = "Timer2",
    };
    oblfr_timer_t timer2;
    oblfr_timer_create(&timer2_cfg, &timer2);
    oblfr_timer_start_periodic(timer2, 1534);

    oblfr_timer_create_args_t timer3_cfg = {
        .callback = one_shot,
        .arg = NULL,
        .name = "One-Shot",
    };
    oblfr_timer_t timer3;
    oblfr_timer_create(&timer3_cfg, &timer3);
    oblfr_timer_start_once(timer3, 4000);
    while (true) {
        bflb_mtimer_delay_ms(1000);
    }
}