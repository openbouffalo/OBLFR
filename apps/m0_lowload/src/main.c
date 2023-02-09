/**
 * @file main.c
 * @brief
 *
 * Copyright (c) 2022 Bouffalolab team
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 */

#include <stdio.h>
#include <board.h>
#include <bflb_mtimer.h>
#include "oblfr_mailbox.h"
#include "oblfr_usbphy.h"
#include "sdkconfig.h"

#define DBG_TAG "MAIN"
#include <log.h>

int main(void)
{
    board_init();

#ifdef CONFIG_COMPONENT_USBPHY
   
    bflb_usb_phy_init();

#endif
    
    LOG_I("Starting Mailbox Handlers\r\n");

    if (oblfr_mailbox_init() != OBLFR_OK) {
        LOG_E("oblfr_mailbox_init failed\r\n");
        while (1) {
            ;
        }
    }
    LOG_I("Running...\r\n");
    while (1)
    {
        oblfr_mailbox_dump();
        bflb_mtimer_delay_ms(5000);
    }
}
