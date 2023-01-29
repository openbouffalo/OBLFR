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
#include <stdint.h>
#include <stdbool.h>
#include "sdkconfig.h"
#include "FreeRTOSConfig.h"
#include <FreeRTOS.h>
#include <task.h>

#include "board.h"
#include "oblfr_system.h"
#include "oblfr_common.h"
#ifdef CONFIG_COMPONENT_TIMER
#include "oblfr_timer.h"
#endif

#define DBG_TAG "SYS"
#include "log.h"

int main(void)
{
    board_init();

#ifdef CONFIG_COMPONENT_TIMER
    oblfr_timer_init();
#endif


    if (xTaskCreate(app_main, "app_main", CONFIG_COMPONENT_SYSTEM_MAIN_TASK_STACK_SIZE, NULL, CONFIG_COMPONENT_SYSTEM_MAIN_TASK_PRIORITY, NULL) != pdPASS) {
        LOG_E("Failed to create app_main task");
        while (true) {
            ;
        }
    }

    vTaskStartScheduler();

}
