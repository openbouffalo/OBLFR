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
#include "oblfr_system.h"
#include "sdkconfig.h"
#include "config.h"
#include "shell_cmds.h"
#include "network.h"

// #include "netif/etharp.h"
// #include "lwip/init.h"
// #include "lwip/netif.h"
// #include "lwip/pbuf.h"
// #include "lwip/icmp.h"
// #include "lwip/udp.h"
// #include "lwip/opt.h"
// #include "lwip/arch.h"
// #include "lwip/api.h"
// #include "lwip/inet.h"
// #include "lwip/dns.h"
// // #include "lwip/tcp_impl.h"
// #include "lwip/tcp.h"

#include "oblfr_rpmsg.h"

#define DBG_TAG "MAIN"
#include <log.h>


oblfr_queue_entry_t *ttyhandle;


void tty_cb(void *data, size_t len, void *priv)
{
    LOG_I("tty_cb: %d\r\n", len);
    if (oblfr_rpmsg_device_send(ttyhandle, data, len, OBLFR_BLOCK) != OBLFR_OK)
    {
        LOG_W("tty_cb: send failed\r\n");
        while (1)
        {
            ;
        }
    }
}
#include "mem.h"

int free_mem()
{
    const char *Header = "total   free    alloc   mxblk   frnode  alnode  \r\n";
    struct meminfo info;
    char *mem;

    mem = malloc(64);
    bflb_mem_usage(KMEM_HEAP, &info);

    sprintf(mem, "%-8d%-8d%-8d%-8d%-8d%-8d\r\n", info.total_size, info.free_size, info.used_size, info.max_free_size,
            info.free_node, info.used_node);

    printf(Header);
    printf(mem);

    free(mem);

    return 0;
}

void vAssertCalled(void)
{
    printf("vAssertCalled\r\n");

    while (1) {
        bflb_mtimer_delay_ms(1000);
    }
}

void vApplicationTickHook(void)
{
    printf("vApplicationTickHook\r\n");
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    printf("vApplicationStackOverflowHook\r\n");

    while (1) {
        bflb_mtimer_delay_ms(1000);
    }
}

void vApplicationMallocFailedHook(void)
{
    printf("vApplicationMallocFailedHook\r\n");

    while (1) {
        bflb_mtimer_delay_ms(1000);
    }
}

void app_main(void *unused)
{
    config_init();

    LOG_I("Starting Mailbox Handlers\r\n");

    if (oblfr_mailbox_init() != OBLFR_OK)
    {
        LOG_E("oblfr_mailbox_init failed\r\n");
        while (1)
        {
            ;
        }
    }
    uint8_t hwaddr[6];

    bflb_efuse_read_mac_address_opt(0, &hwaddr, 1);
    LOG_I("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n", hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
    bflb_efuse_read_mac_address_opt(1, &hwaddr, 1);
    LOG_I("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n", hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);
    bflb_efuse_read_mac_address_opt(2, &hwaddr, 1);
    LOG_I("MAC: %02x:%02x:%02x:%02x:%02x:%02x\r\n", hwaddr[0], hwaddr[1], hwaddr[2], hwaddr[3], hwaddr[4], hwaddr[5]);

    network_init();

    if (init_rpmsg(NULL) != OBLFR_OK)
    {
        LOG_E("init_rpmsg failed\r\n");
        while (1)
        {
            vTaskDelay(100/portTICK_PERIOD_MS);
        }
    }

    rpmsg_net_init();

    usb_rndis_init();


    LOG_I("Running...Shell Enabled!\r\n");

    shell_cmd_init();
    while (1)
    {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

