#include "oblfr_timer.h"
#include <sys/queue.h>
#include <bflb_timer.h>
#include <bflb_mtimer.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"


#define DBG_TAG "TMR"
#include "log.h"

struct oblfr_timer {
    uint64_t when;
    uint64_t period;
    oblfr_timer_cb_t callback;
    bool repeat;
    const char *name;
    LIST_ENTRY(oblfr_timer) list_entry;
};

static LIST_HEAD(oblfr_timer_list, oblfr_timer) oblfr_timer_list = LIST_HEAD_INITIALIZER(oblfr_timer_list);

static SemaphoreHandle_t oblfr_timer_list_lock = NULL;

static TaskHandle_t oblfr_timer_task_handle = NULL;
static struct bflb_device_s *timer0 = NULL;

void timer_isr(int irq, void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    bool status = bflb_timer_get_compint_status(timer0, TIMER_COMP_ID_0);
    if (status) {
        bflb_timer_compint_clear(timer0, TIMER_COMP_ID_0);
        vTaskNotifyGiveFromISR(oblfr_timer_task_handle, &xHigherPriorityTaskWoken);
    }
    if (xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

static bool oblfr_timer_lock() {
    if (xSemaphoreTake(oblfr_timer_list_lock, portMAX_DELAY) != pdPASS) {
        LOG_E("Failed to take timer list lock\r\n");
        return false;
    }
    return true;
}

static bool oblfr_timer_unlock() {
    if (xSemaphoreGive(oblfr_timer_list_lock) != pdPASS) {
        LOG_E("Failed to give timer list lock\r\n");
        return false;
    }
    return true;
}

static oblfr_err_t oblfr_recalc_next_fire() {
    if (!oblfr_timer_lock()) {
        LOG_E("oblfr_timer_process: Failed to take timer list lock\r\n");
        return OBLFR_ERR_ERROR;
    }
    oblfr_timer_t head = LIST_FIRST(&oblfr_timer_list);

    assert(head != NULL);

    uint64_t when = (head->when - bflb_mtimer_get_time_ms());
    if (when > 1000) {
        when = 1000;
    }
    if (when < 0) {
        when = 0;
    }
    /* increase 1ms so it fires after the next timer expires */
    when += 1;
    bflb_timer_set_compvalue(timer0, TIMER_COMP_ID_0, (when*1000));
    oblfr_timer_unlock();
    return OBLFR_OK;
}

static oblfr_err_t oblfr_timer_insert(struct oblfr_timer *timer) {
    struct oblfr_timer *t;
    if (!oblfr_timer_lock()) {
        return OBLFR_ERR_ERROR;
    }
    oblfr_timer_t tail;
    if (LIST_FIRST(&oblfr_timer_list) == NULL) {
        LIST_INSERT_HEAD(&oblfr_timer_list, timer, list_entry);
    } else { 
        LIST_FOREACH(t, &oblfr_timer_list, list_entry) {
            if (t->when > timer->when) {
                LIST_INSERT_BEFORE(t, timer, list_entry);
                break;
            }
            tail = t;
        }
        if (t == NULL) {
            LIST_INSERT_AFTER(tail, timer, list_entry);
        }
    }
    oblfr_timer_unlock();
    return OBLFR_OK;
}

static oblfr_err_t oblfr_timer_remove(struct oblfr_timer *timer) {
    if (timer == NULL) {
        return OBLFR_ERR_INVALID;
    }
    if (!oblfr_timer_lock()) {
        return OBLFR_ERR_ERROR;
    }
    LIST_REMOVE(timer, list_entry);
    timer->when = 0;
    oblfr_timer_unlock();
    return OBLFR_OK;
}

static void oblfr_timer_process() {
    if (!oblfr_timer_lock()) {
        LOG_E("oblfr_timer_process: Failed to take timer list lock\r\n");
        return;
    }
    struct oblfr_timer *t = LIST_FIRST(&oblfr_timer_list);
    while (t != NULL) {
        uint64_t now = bflb_mtimer_get_time_ms();
        if (t->when > now) {
            LOG_D("oblfr_timer_process: next timer is %s, %lld ms later\r\n", t->name, t->when - now);
            break;
        }
        struct oblfr_timer *next = LIST_NEXT(t, list_entry);
        LIST_REMOVE(t, list_entry);
        t->callback(t);
        if (t->repeat) {
            t->when += t->period;
            LOG_D("oblfr_timer_process: repeat timer %s, next time is %lld ms later\r\n", t->name, t->period);
            oblfr_timer_unlock();
            oblfr_timer_insert(t);
            if (!oblfr_timer_lock()) {
                LOG_E("oblfr_timer_process: Failed to take timer list lock\r\n");
                return;
            }
        } 
        t = next;
    }
    oblfr_timer_unlock();
    oblfr_recalc_next_fire();
}

static void oblfr_timer_task(void *arg) {
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        oblfr_timer_process();
    }
}

oblfr_err_t oblfr_timer_init() {

    /* timer clk = XCLK/(div + 1 )*/
    struct bflb_timer_config_s cfg0;
    cfg0.counter_mode = TIMER_COUNTER_MODE_PROLOAD; /* preload when match occur */
    cfg0.clock_source = TIMER_CLKSRC_XTAL;
#ifdef CONFIG_CHIP_BL708
    cfg0.clock_div = 31; /* for bl616/bl808/bl606p is 39, for bl702 is 31 */
#else
    cfg0.clock_div = 39; /* for bl616/bl808/bl606p is 39, for bl702 is 31 */
#endif
    cfg0.trigger_comp_id = TIMER_COMP_ID_0;
    cfg0.comp0_val = 1000000; /* this is 100ms TODO: reduce for production */

    timer0 = bflb_device_get_by_name("timer0");
    bflb_timer_init(timer0, &cfg0);
    bflb_irq_attach(timer0->irq_num, timer_isr, NULL);
    bflb_irq_enable(timer0->irq_num);

    vSemaphoreCreateBinary(oblfr_timer_list_lock);

    if (xTaskCreate(oblfr_timer_task, "oblfr_timer_task", 1024, NULL, 5, &oblfr_timer_task_handle) != pdPASS) {
        LOG_E("Create Timer Task failed\r\n");
        return OBLFR_ERR_ERROR;
    }
    bflb_timer_start(timer0);
    return OBLFR_OK;
}

oblfr_err_t oblfr_timer_deinit() {
    if (!oblfr_timer_lock()) {
        LOG_E("oblfr_timer_deinit: Failed to take timer list lock\r\n");
        return OBLFR_ERR_ERROR;
    }

    bflb_irq_disable(timer0->irq_num);
    bflb_irq_detach(timer0->irq_num);
    bflb_timer_stop(timer0);
    struct oblfr_timer *t = LIST_FIRST(&oblfr_timer_list);
    while (t != NULL) {
        struct oblfr_timer *next = LIST_NEXT(t, list_entry);
        oblfr_timer_unlock();
        oblfr_timer_stop(t);
        if (!oblfr_timer_lock()) {
            LOG_E("oblfr_timer_deinit: Failed to take timer list lock\r\n");
            return OBLFR_ERR_ERROR;
        }
        t = next;
    }
    vTaskDelete(oblfr_timer_task_handle);
    LOG_D("Timer deinit\r\n");
    oblfr_timer_unlock();
    return OBLFR_OK;
}

oblfr_err_t oblfr_timer_create(const oblfr_timer_create_args_t* create_args, oblfr_timer_t* out_handle) {
    if (create_args == NULL || create_args->callback == NULL || out_handle == NULL) {
        return OBLFR_ERR_INVALID;
    }
    oblfr_timer_t timer = (oblfr_timer_t)malloc(sizeof(struct oblfr_timer));
    if (timer == NULL) {
        return OBLFR_ERR_NOMEM;
    }
    timer->callback = create_args->callback;
    timer->name = create_args->name;
    timer->list_entry.le_next = NULL;
    *out_handle = timer;
    LOG_D("Timer %s created\r\n", timer->name);
    return OBLFR_OK;
}

oblfr_err_t oblfr_timer_start_once(oblfr_timer_t timer, uint32_t timeout_ms) {
    if (timer == NULL) {
        return OBLFR_ERR_INVALID;
    }
    if (oblfr_timer_is_active(timer)) {
        LOG_E("Timer %s is already active\r\n", timer->name);
        return OBLFR_ERR_INVALID;
    }
    timer->when = bflb_mtimer_get_time_ms() + timeout_ms;
    timer->period = 0;
    timer->repeat = false;
    oblfr_err_t ret = oblfr_timer_insert(timer);
    oblfr_recalc_next_fire();
    LOG_D("Starting one-shot timer %s - Trigger at %lld\r\n", timer->name, timeout_ms);
    return ret; 
}

oblfr_err_t oblfr_timer_start_periodic(oblfr_timer_t timer, uint32_t period_ms) {
    if (timer == NULL) {
        return OBLFR_ERR_INVALID;
    }
    if (oblfr_timer_is_active(timer)) {
        LOG_E("Timer %s is already active\r\n", timer->name);
        return OBLFR_ERR_INVALID;
    }
    timer->when = bflb_mtimer_get_time_ms() + period_ms;
    timer->period = period_ms;
    timer->repeat = true;
    oblfr_err_t ret = oblfr_timer_insert(timer);
    oblfr_recalc_next_fire();
    LOG_D("Starting periodic timer %s - Next Trigger at %lld\r\n", timer->name, timer->period);
    return ret; 
}

oblfr_err_t oblfr_timer_restart(oblfr_timer_t timer, uint32_t timeout_ms) {
    if (timer == NULL) {
        return OBLFR_ERR_INVALID;
    }
    if (timeout_ms == 0) {
        return OBLFR_ERR_INVALID;
    }
    if (!oblfr_timer_is_active(timer)) {
        return OBLFR_ERR_INVALID;
    }
    if (oblfr_timer_remove(timer) != OBLFR_OK) {
        return OBLFR_ERR_INVALID;
    }
    if (timer->repeat) {
        LOG_D("Restarting periodic timer %s\r\n", timer->name);
        return oblfr_timer_start_periodic(timer, timeout_ms);
    } else {
        LOG_D("Restarting one-shot timer %s\r\n", timer->name);
        return oblfr_timer_start_once(timer, timeout_ms);
    }
}

oblfr_err_t oblfr_timer_stop(oblfr_timer_t timer) {
    if (timer == NULL) {
        return OBLFR_ERR_INVALID;
    }
    if (oblfr_timer_remove(timer) != OBLFR_OK) {
        return OBLFR_ERR_INVALID;
    }
    LOG_D("Timer %s stopped\r\n", timer->name);
    oblfr_recalc_next_fire();
    return OBLFR_OK;
}

oblfr_err_t oblfr_timer_delete(oblfr_timer_t timer) {
    if (timer == NULL) {
        return OBLFR_ERR_INVALID;
    }
    if (oblfr_timer_is_active(timer)) {
        return OBLFR_ERR_INVALID;
    }
    LOG_D("Deleting Timer %s\r\n", timer->name);
    free(timer);
    return OBLFR_OK;
}

bool oblfr_timer_is_active(oblfr_timer_t timer) {  
    if (timer == NULL) {
        return false;
    }
    if (!oblfr_timer_lock()) {
        LOG_E("oblfr_timer_deinit: Failed to take timer list lock\r\n");
        return false;
    }    
    if (LIST_NEXT(timer, list_entry) != NULL) {
        oblfr_timer_unlock();
        return true;
    }
    oblfr_timer_unlock();
    return false;
}

void oblfr_timer_dump() {
    if (!oblfr_timer_lock()) {
        LOG_E("oblfr_timer_deinit: Failed to take timer list lock\r\n");
        return;
    }
    struct oblfr_timer *t;
    LOG_I("Timer List Dump:\r\n");
    LIST_FOREACH(t, &oblfr_timer_list, list_entry) {
        int64_t in = t->when - bflb_mtimer_get_time_ms();
        if ( in < 0)
            in = 0;
        LOG_I("\tTimer %s: in=%lld repeat=%s, period=%lld\r\n", t->name, in, t->repeat ? "true":"false", t->period);
    }
    LOG_I("End of Timer List Dump\r\n");
    oblfr_timer_unlock();
}