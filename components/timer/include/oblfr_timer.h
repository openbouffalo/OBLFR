// Copyright 2023 Justin Hammond
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Modeled After the esp-idf timer implementation

#ifndef OBLFR_TIMER_H
#define OBLFR_TIMER_H

#include <stdint.h>
#include <stdbool.h>
#include "oblfr_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum number of timers that can be created
 */
#define OBLFR_TIMER_MAX 10

/**
 * @brief Opaque handle to a timer
 */
typedef struct oblfr_timer *oblfr_timer_t;

/**
 * @brief Timer callback function
 */
typedef void (*oblfr_timer_cb_t)(void* arg);

/**
 * @brief Timer configuration passed to esp_timer_create
 */
typedef struct {
    const char* name;               //!< Timer name, used in esp_timer_dump function
    oblfr_timer_cb_t callback;      //!< Function to call when timer expires
    void* arg;                      //!< Argument to pass to the callback
} esp_timer_create_args_t;

/**
 * @brief Initialize the timer module
 * 
 * @return oblfr_err_t
 *          - OBLFR_OK: Success
 *          - OBLFR_ERR_ERROR: If we can not create the FreeRTOS Timer Task
 */
oblfr_err_t oblfr_timer_init();

/**
 * @brief Deinitialize the timer module
 * 
 * This function will stop the timer interupt and remove all timers from the active timer
 * list. The user is responsible for calling oblfr_timer_delete for created timers.
 * 
 * @return oblfr_err_t
 *         - OBLFR_OK: Success
 *         - OBLFR_ERR_ERROR if we are unable to stop all registered timers.
 */
oblfr_err_t oblfr_timer_deinit();

/**
 * @brief Create a timer
 * 
 * @param create_args Configuration for the timer
 * @param out_handle Pointer to a variable to receive the timer handle
 * @return oblfr_err_t
 *        - OBLFR_OK: Success
 *        - OBLFR_ERR_INVALID: If create_args is NULL or out_handle is NULL or create_args->callback is NULL
 *        - OBLFR_ERR_NOMEM: If we are unable to allocate memory for the timer out_handle
 */
oblfr_err_t oblfr_timer_create(const esp_timer_create_args_t* create_args,
                           oblfr_timer_t* out_handle);

/**
 * @brief Start a one-shot timer. 
 * 
 * The Timer will be called after timeout_ms milliseconds. If the timer is already active,
 * a error will be returned.
 * 
 * @param timer Handle of the timer to start
 * @param timeout_ms Timeout in milliseconds
 * @return oblfr_err_t
 *       - OBLFR_OK: Success
 *       - OBLFR_ERR_INVALID: If timer is NULL, or it is already active
 */
oblfr_err_t oblfr_timer_start_once(oblfr_timer_t timer, uint32_t timeout_ms);

/**
 * @brief Start a periodic timer. 
 * 
 * The Timer will be called every period_ms milliseconds. If the timer is already active,
 * a error will be returned.
 * 
 * @param timer Handle of the timer to start
 * @param period_ms Period in milliseconds
 * @param oblfr_err_t
 *      - OBLFR_OK: Success
 *      - OBLFR_ERR_INVALID: If timer is NULL, or it is already active
 */
oblfr_err_t oblfr_timer_start_periodic(oblfr_timer_t timer, uint32_t period_ms);

/**
 * @brief Restart a timer.
 * 
 * If the timer is not active, a error will be returned. Otherwise the timer will be rescheduled
 * to call after timeout_ms milliseconds.
 * 
 * @param timer Handle of the timer to restart
 * @param timeout_ms Timeout in milliseconds
 * @return oblfr_err_t
 *       - OBLFR_OK: Success
 *       - OBLFR_ERR_INVALID: If timer is NULL, or it is not active, or we are unable to reschedule the timer
 */
oblfr_err_t oblfr_timer_restart(oblfr_timer_t timer, uint32_t timeout_ms);

/** 
 * @brief Stop a timer.
 * 
 * If the timer is not active, a error will be returned. Otherwise the timer will be stopped.
 * 
 * @param timer Handle of the timer to stop
 * @return oblfr_err_t
 *       - OBLFR_OK: Success
 *       - OBLFR_ERR_INVALID: If timer is NULL, or it is not active
*/
oblfr_err_t oblfr_timer_stop(oblfr_timer_t timer);

/**
 * @brief Delete a timer.
 * 
 * If the timer is active, a error will be returned. Otherwise the timer will be deleted.
 * 
 * @param timer Handle of the timer to delete
 * @return oblfr_err_t
 *       - OBLFR_OK: Success
 *       - OBLFR_ERR_INVALID: If timer is NULL, or it is active
*/
oblfr_err_t oblfr_timer_delete(oblfr_timer_t timer);

/**
 * @brief Check if a timer is active.
 * 
 * @param timer Handle of the timer to check
 * @return true if the timer is active, false otherwise
 */
bool oblfr_timer_is_active(oblfr_timer_t timer);

/** 
 * @brief Dump the Timer list to the console
 */
void oblfr_timer_dump();

#ifdef __cplusplus
}
#endif

#endif