/**
 * @file main.c
 * @brief
 *
 * Copyright (c) 2023 Justin Hammond
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
#include "oblfr_rpmsg.h"
#include "sdkconfig.h"

#define DBG_TAG "MAIN"
#include <log.h>

void tty_cb(void *data, size_t len, void *priv)
{
    LOG_I("tty_cb: %s\r\n", data);
}

void raw_cb(void *data, size_t len, void *priv)
{
    LOG_I("raw_cb: %s\r\n", data);
}

void haha_cb(void *data, size_t len, void *priv)
{
    LOG_I("haha_cb: %s\r\n", data);
}

int app_main(void)
{
    //    board_init();

    LOG_I("Starting Mailbox Handlers\r\n");

    if (oblfr_mailbox_init() != OBLFR_OK)
    {
        LOG_E("oblfr_mailbox_init failed\r\n");
        while (1)
        {
            ;
        }
    }
    LOG_I("Running...\r\n");
    if (init_rpmsg(NULL) != OBLFR_OK)
    {
        LOG_E("init_rpmsg failed\r\n");
        while (1)
        {
            ;
        }
    }

    oblfr_device_cfg_t endpoint = {
        .name = "rpmsg-tty",
        .cb = tty_cb,
        .priv = NULL,
    };
    oblfr_queue_entry_t *ttyhandle = oblfr_rpmsg_device_add(&endpoint);

    oblfr_device_cfg_t endpoint2 = {
        .name = "rpmsg-raw",
        .cb = raw_cb,
        .priv = NULL,
    };
    oblfr_rpmsg_device_add(&endpoint2);

    oblfr_device_cfg_t endpoint3 = {
        .name = "haha",
        .cb = haha_cb,
        .priv = NULL,
    };
    oblfr_queue_entry_t *handle = oblfr_rpmsg_device_add(&endpoint3);
    char txdata[] = "Hello from M0";
    LOG_I("You can now open and send messages from linux on /dev/ttyRPMSG0\r\n");
    while (1)
    {
        bflb_mtimer_delay_ms(6000);
        if (handle && (oblfr_rpmsg_device_remove(handle) != OBLFR_OK))
        {
            LOG_E("oblfr_rpmsg_remove_endpoint failed\r\n");
        }
        else
        {
            handle = NULL;
        }
        if (oblfr_rpmsg_device_send(ttyhandle, txdata, sizeof(txdata), OBLFR_NO_BLOCK) != OBLFR_OK)
        {
            LOG_E("oblfr_rpmsg_device_send failed\r\n");
        }
        oblfr_rpmsg_dump();
    }
}
