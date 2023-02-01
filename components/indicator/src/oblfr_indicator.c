// Copyright 2020-2021 Espressif Systems (Shanghai) CO LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdbool.h>
#include <string.h>
#include <sys/queue.h>
#include "FreeRTOS.h"
#include "timers.h"
#include "semphr.h"
#include "bflb_gpio.h"

#include "oblfr_indicator.h"
#define DBG_TAG "LED"
#include "log.h"


#define LED_INDICATOR_CHECK(a, str, ret) if(!(a)) { \
        LOG_E("%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str); \
        return (ret); \
    }

#define LED_INDICATOR_CHECK_GOTO(a, str, lable) if(!(a)) { \
        LOG_E("%s:%d (%s):%s", __FILE__, __LINE__, __FUNCTION__, str); \
        goto lable; \
    }

#define NULL_ACTIVE_BLINK -1

/*********************************** Config Blink List in Different Conditions ***********************************/
/**
 * @brief connecting to AP (or Cloud)
 * 
 */
static const blink_step_t connecting[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 200},
    {LED_BLINK_HOLD, LED_STATE_OFF, 800},
    {LED_BLINK_LOOP, 0, 0},
};

/**
 * @brief connected to AP (or Cloud) succeed
 * 
 */
static const blink_step_t connected[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 1000},
    {LED_BLINK_LOOP, 0, 0},
};

/**
 * @brief reconnecting to AP (or Cloud), if lose connection 
 * 
 */
static const blink_step_t reconnecting[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 100},
    {LED_BLINK_HOLD, LED_STATE_OFF, 200},
    {LED_BLINK_LOOP, 0, 0},
}; //offline

/**
 * @brief updating software
 * 
 */
static const blink_step_t updating[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 50},
    {LED_BLINK_HOLD, LED_STATE_OFF, 100},
    {LED_BLINK_HOLD, LED_STATE_ON, 50},
    {LED_BLINK_HOLD, LED_STATE_OFF, 800},
    {LED_BLINK_LOOP, 0, 0},
};

/**
 * @brief restoring factory settings
 * 
 */
static const blink_step_t factory_reset[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 200},
    {LED_BLINK_HOLD, LED_STATE_OFF, 200},
    {LED_BLINK_LOOP, 0, 0},
};

/**
 * @brief provisioning
 * 
 */
static const blink_step_t provisioning[] = {
    {LED_BLINK_HOLD, LED_STATE_ON, 500},
    {LED_BLINK_HOLD, LED_STATE_OFF, 500},
    {LED_BLINK_LOOP, 0, 0},
};

/**
 * @brief provision done
 * 
 */
static const blink_step_t provisioned[] = {
    {LED_BLINK_HOLD, LED_STATE_OFF, 1000},
    {LED_BLINK_STOP, 0, 0},
};

/**
 * @brief led indicator blink lists, the index like BLINK_FACTORY_RESET defined the priority of the blink
 * 
 */
blink_step_t const * oblfr_indicator_blink_lists[] = {
    [BLINK_FACTORY_RESET] = factory_reset,
    [BLINK_UPDATING] = updating,
    [BLINK_CONNECTED] = connected,
    [BLINK_PROVISIONED] = provisioned,
    [BLINK_RECONNECTING] = reconnecting,
    [BLINK_CONNECTING] = connecting,
    [BLINK_PROVISIONING] = provisioning,
    [BLINK_MAX] = NULL,
};

/* Led blink_steps handling machine implementation */
#define BLINK_LIST_NUM (sizeof(oblfr_indicator_blink_lists)/sizeof(oblfr_indicator_blink_lists[0]))

/**
 * @brief led indicator object
 * 
 */
typedef struct {
    bool off_level; /*!< gpio level during led turn off */
    int io_num; /*!< gpio number of the led indicator */
    oblfr_indicator_mode_t mode; /*!< led work mode, eg. gpio or pwm mode */
    int active_blink; /*!< active blink list*/
    int *p_blink_steps; /*!< stage of each blink list */
    SemaphoreHandle_t mutex; /*!< mutex to achive thread-safe */
    TimerHandle_t h_timer; /*!< led timmer handle, invalid if works in pwm mode */
}_oblfr_indicator_t;

typedef struct _oblfr_indicator_slist_t{
    SLIST_ENTRY(_oblfr_indicator_slist_t) next;
    _oblfr_indicator_t *p_oblfr_indicator;
}_oblfr_indicator_slist_t;

static SLIST_HEAD(_oblfr_indicator_head_t, _oblfr_indicator_slist_t) s_oblfr_indicator_slist_head = SLIST_HEAD_INITIALIZER(s_oblfr_indicator_slist_head);

static oblfr_err_t _oblfr_indicator_add_node(_oblfr_indicator_t *p_oblfr_indicator)
{
    LED_INDICATOR_CHECK(p_oblfr_indicator != NULL, "pointer can not be NULL", OBLFR_ERR_INVALID);
    _oblfr_indicator_slist_t *node = calloc(1, sizeof(_oblfr_indicator_slist_t));
    LED_INDICATOR_CHECK(node != NULL, "calloc node failed", OBLFR_ERR_NOMEM);
    node->p_oblfr_indicator = p_oblfr_indicator;
    SLIST_INSERT_HEAD(&s_oblfr_indicator_slist_head, node, next);
    return OBLFR_OK;
}

static oblfr_err_t _oblfr_indicator_remove_node(_oblfr_indicator_t *p_oblfr_indicator)
{
    LED_INDICATOR_CHECK(p_oblfr_indicator != NULL, "pointer can not be NULL", OBLFR_ERR_INVALID);
    _oblfr_indicator_slist_t *node;
    SLIST_FOREACH(node, &s_oblfr_indicator_slist_head, next) {
        if (node->p_oblfr_indicator == p_oblfr_indicator) {
            SLIST_REMOVE(&s_oblfr_indicator_slist_head, node, _oblfr_indicator_slist_t, next);
            free(node);
            break;
        }
    }
    return OBLFR_OK;
}

/**
 * @brief init a gpio to control led
 * 
 * @param io_num gpio number of the led
 * @return true init succeed
 * @return false init failed
 */
static bool _led_gpio_init(int io_num)
{
    LOG_D("led_gpio_init %d\r\n", io_num);

    struct bflb_device_s *gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_init(gpio, io_num, GPIO_OUTPUT|GPIO_PULLDOWN|GPIO_DRV_3);

    return true;
}

/**
 * @brief deinit a gpio to control led
 * 
 * @param io_num gpio number of the led
 * @return true deinit succeed
 * @return false deinit failed
 */
static bool _led_gpio_deinit(int io_num)
{
    LOG_D("led_gpio_deinit %d\r\n", io_num);

    struct bflb_device_s *gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_deinit(gpio, io_num);

    return true;
}

/**
 * @brief turn on or off of the led
 * 
 * @param io_num gpio number of the led
 * @param off_level gpio level when off, 0 if attach led positive side to esp32 gpio pin, 1 if attach led negative side
 * @param state target state
 * @return esp_err_t 
 */
static oblfr_err_t _led_set_state(int io_num, bool off_level, blink_step_state_t state)
{
    LOG_D("_led_set_state %d state %d %d\r\n", io_num, state, off_level);

    struct bflb_device_s *gpio = bflb_device_get_by_name("gpio");
    switch (state)
    {
    case LED_STATE_ON:
        if (off_level) 
            bflb_gpio_set(gpio, io_num);
        else
            bflb_gpio_reset(gpio, io_num);
        break;
    case LED_STATE_OFF:
    default :
        if (off_level) 
            bflb_gpio_reset(gpio, io_num);
        else
            bflb_gpio_set(gpio, io_num);
        break;
    }
    return OBLFR_OK;
}

/**
 * @brief switch to the first high priority incomplete blink steps
 * 
 * @param p_oblfr_indicator pointer to led indicator
 */
static void _blink_list_switch(_oblfr_indicator_t *p_oblfr_indicator)
{
    p_oblfr_indicator->active_blink = NULL_ACTIVE_BLINK; //stop active blink
    for(size_t index = 0; index < BLINK_LIST_NUM; index ++) //find the first incomplete blink
    {
        if (p_oblfr_indicator->p_blink_steps[index] != LED_BLINK_STOP)
        {
            p_oblfr_indicator->active_blink = index;
            break;
        }
    }
}

/**
 * @brief timmer callback to control led and counter steps
 * 
 * @param xTimer handle of the timmer instance
 */
static void _blink_list_runner(TimerHandle_t xTimer)
{
    _oblfr_indicator_t * p_oblfr_indicator = (_oblfr_indicator_t *)pvTimerGetTimerID(xTimer);
    bool leave = false;
    while(!leave) {
        if (p_oblfr_indicator->active_blink == NULL_ACTIVE_BLINK)
        return;

        int active_blink = p_oblfr_indicator->active_blink;
        int active_step = p_oblfr_indicator->p_blink_steps[active_blink];
        const blink_step_t *p_blink_step = &oblfr_indicator_blink_lists[active_blink][active_step];

        p_oblfr_indicator->p_blink_steps[active_blink] += 1;

        if (pdFALSE == xSemaphoreTake(p_oblfr_indicator->mutex, pdMS_TO_TICKS(100))) {
            LOG_E("blinks runner blockTime expired, try repairing...");
            xTimerChangePeriod(p_oblfr_indicator->h_timer, pdMS_TO_TICKS(100), 0);
            xTimerStart(p_oblfr_indicator->h_timer, 0);
            break;
        }
        switch(p_blink_step->type) {
            case LED_BLINK_LOOP:
                    p_oblfr_indicator->p_blink_steps[active_blink] = 0;
                break;

            case LED_BLINK_STOP:
                    p_oblfr_indicator->p_blink_steps[active_blink] = LED_BLINK_STOP;
                    _blink_list_switch(p_oblfr_indicator);
                break;

            case LED_BLINK_HOLD:
                    _led_set_state(p_oblfr_indicator->io_num, p_oblfr_indicator->off_level, p_blink_step->on_off);
                    if (p_blink_step->hold_time_ms == 0)
                    break;
                    xTimerChangePeriod(p_oblfr_indicator->h_timer, pdMS_TO_TICKS(p_blink_step->hold_time_ms), 0);
                    xTimerStart(p_oblfr_indicator->h_timer, 0);
                    leave=true;
                break;

            default:
                    assert(false && "invalid state");
                break;
        }
        xSemaphoreGive(p_oblfr_indicator->mutex);
    }
}

oblfr_indicator_handle_t oblfr_indicator_create(int io_num, const oblfr_indicator_config_t* config)
{
    LED_INDICATOR_CHECK(config != NULL, "invalid config pointer", NULL);
    char timmer_name[16] = {'\0'};
    snprintf(timmer_name, sizeof(timmer_name) - 1, "%s%02x", "led_tmr_", io_num);
    _oblfr_indicator_t *p_oblfr_indicator = (_oblfr_indicator_t *)calloc(1, sizeof(_oblfr_indicator_t));
    LED_INDICATOR_CHECK(p_oblfr_indicator != NULL, "calloc indicator memory failed", NULL);
    p_oblfr_indicator->off_level = config->off_level;
    p_oblfr_indicator->io_num = io_num;
    p_oblfr_indicator->mode = config->mode;
    p_oblfr_indicator->active_blink = NULL_ACTIVE_BLINK;
    p_oblfr_indicator->p_blink_steps = (int *)calloc(BLINK_LIST_NUM, sizeof(int));
    LED_INDICATOR_CHECK_GOTO(p_oblfr_indicator->p_blink_steps != NULL, "calloc blink_steps memory failed", cleanup_indicator);
    p_oblfr_indicator->mutex = xSemaphoreCreateMutex();
    LED_INDICATOR_CHECK_GOTO(p_oblfr_indicator->mutex != NULL, "create mutex failed", cleanup_indicator_blinkstep);
    
    for(size_t j = 0; j < BLINK_LIST_NUM; j++) {
        *(p_oblfr_indicator->p_blink_steps + j) = LED_BLINK_STOP;
    }

    switch (p_oblfr_indicator->mode)
    {
        case LED_GPIO_MODE:        /**< blink with max brightness*/
            {
            bool ininted = _led_gpio_init(p_oblfr_indicator->io_num);
            LED_INDICATOR_CHECK_GOTO(ininted != false, "init led gpio failed", cleanup_all);
            p_oblfr_indicator->h_timer = xTimerCreate(timmer_name, (pdMS_TO_TICKS(100)), pdFALSE, (void *)p_oblfr_indicator, _blink_list_runner);
            LED_INDICATOR_CHECK_GOTO(p_oblfr_indicator->h_timer != NULL, "led timmer create failed", cleanup_all);
            }
            break;

        default:
            LED_INDICATOR_CHECK_GOTO(false, "mode not supported", cleanup_all);
            break;
    }

    _oblfr_indicator_add_node(p_oblfr_indicator);
    return (oblfr_indicator_handle_t)p_oblfr_indicator;

cleanup_indicator:
    free(p_oblfr_indicator);
    return NULL;
cleanup_indicator_blinkstep:
    free(p_oblfr_indicator->p_blink_steps);
    free(p_oblfr_indicator);
    return NULL;
cleanup_all:
    vSemaphoreDelete(p_oblfr_indicator->mutex);
    free(p_oblfr_indicator->p_blink_steps);
    free(p_oblfr_indicator);
    return NULL;
}

oblfr_indicator_handle_t oblfr_indicator_get_handle(int io_num)
{
    _oblfr_indicator_slist_t *node;
    SLIST_FOREACH(node, &s_oblfr_indicator_slist_head, next) {
        if (node->p_oblfr_indicator->io_num == io_num) {
            return (oblfr_indicator_handle_t)(node->p_oblfr_indicator);
        }
    }
    return NULL;
}

oblfr_err_t oblfr_indicator_delete(oblfr_indicator_handle_t* p_handle)
{
    LED_INDICATOR_CHECK(p_handle != NULL && *p_handle != NULL, "invalid p_handle", OBLFR_ERR_INVALID);
    _oblfr_indicator_t *p_oblfr_indicator = (_oblfr_indicator_t *)(*p_handle);
    xSemaphoreTake(p_oblfr_indicator->mutex, portMAX_DELAY);

    switch (p_oblfr_indicator->mode)
    {
        case LED_GPIO_MODE:
            {
            bool deinited = _led_gpio_deinit(p_oblfr_indicator->io_num);
            LED_INDICATOR_CHECK(deinited != false, "deinit led gpio failed", OBLFR_ERR_ERROR);
            BaseType_t ret = xTimerDelete(p_oblfr_indicator->h_timer, portMAX_DELAY);
            LED_INDICATOR_CHECK(ret == pdPASS, "led timmer delete failed", OBLFR_ERR_ERROR);
            }
            break;

        default:
            LED_INDICATOR_CHECK(false, "mode not supported", OBLFR_ERR_NOTSUPPORTED);
            break;
    }
    _oblfr_indicator_remove_node(p_oblfr_indicator);
    vSemaphoreDelete(p_oblfr_indicator->mutex);
    free(p_oblfr_indicator->p_blink_steps);
    free(*p_handle);
    *p_handle = NULL;
    return OBLFR_OK;
}

oblfr_err_t oblfr_indicator_start(oblfr_indicator_handle_t handle, oblfr_indicator_blink_type_t blink_type)
{
    LED_INDICATOR_CHECK(handle != NULL && blink_type >= 0 && blink_type < BLINK_MAX, "invalid p_handle", OBLFR_ERR_INVALID);
    LED_INDICATOR_CHECK(oblfr_indicator_blink_lists[blink_type] != NULL, "undefined blink_type", OBLFR_ERR_INVALID);
    _oblfr_indicator_t *p_oblfr_indicator = (_oblfr_indicator_t *)handle;
    xSemaphoreTake(p_oblfr_indicator->mutex, portMAX_DELAY);
    p_oblfr_indicator->p_blink_steps[blink_type] = 0;
    _blink_list_switch(p_oblfr_indicator);
    xSemaphoreGive(p_oblfr_indicator->mutex);

    if(p_oblfr_indicator->active_blink == blink_type) { //re-run from first step
        _blink_list_runner(p_oblfr_indicator->h_timer);
    }

    return OBLFR_OK;
}

oblfr_err_t oblfr_indicator_stop(oblfr_indicator_handle_t handle, oblfr_indicator_blink_type_t blink_type)
{
    LED_INDICATOR_CHECK(handle != NULL && blink_type >= 0 && blink_type < BLINK_MAX, "invalid p_handle", OBLFR_ERR_INVALID);
    LED_INDICATOR_CHECK(oblfr_indicator_blink_lists[blink_type] != NULL, "undefined blink_type", OBLFR_ERR_INVALID);
    _oblfr_indicator_t *p_oblfr_indicator = (_oblfr_indicator_t *)handle;
    xSemaphoreTake(p_oblfr_indicator->mutex, portMAX_DELAY);
    p_oblfr_indicator->p_blink_steps[blink_type] = LED_BLINK_STOP;
    _blink_list_switch(p_oblfr_indicator); //stop and swith to next blink steps
    xSemaphoreGive(p_oblfr_indicator->mutex);

    if(p_oblfr_indicator->active_blink == blink_type) { //re-run from first step
        _blink_list_runner(p_oblfr_indicator->h_timer);
    }

    return OBLFR_OK;
}
