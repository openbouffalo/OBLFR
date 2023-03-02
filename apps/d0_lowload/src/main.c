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

#define DBG_TAG "LowLoad"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <rv_pmp.h>
#include <log.h>
#include <csi_core.h>
#include <bflb_uart.h>
#include <bflb_gpio.h>
#include "board.h"

extern void unlz4(const void *aSource, void *aDestination, uint32_t FileLen);

#define VM_LINUX_DST_ADDR 0x50000000
#define OPENSBI_DST_ADDR 0x3EF80000
#define DTB_DST_ADDR 0x51ff8000
#define BOOT_HDR_SRC_ADDR 0x5d5ff000

static struct bflb_device_s *uart0;

#if (__riscv_xlen == 64)
/* linux pmp setting */
const pmp_config_entry_t pmp_entry_tab[8] = {
    [0] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x20000000,
        .entry_pa_length = PMP_REG_SZ_1M,
    },

    [1] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x30000000,
        .entry_pa_length = PMP_REG_SZ_1M,
    },

    [2] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_X | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x3ef80000,
        .entry_pa_length = 0x10000
    },

    [3] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_X | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x40000000,
        .entry_pa_length = PMP_REG_SZ_16K,
    },

    [4] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_X | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x50000000,
        .entry_pa_length = 0x4000000,
    },

    [5] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0x58000000,
        .entry_pa_length = 0x4000000,
    },

    [6] = {
        .entry_flag = ENTRY_FLAG_ADDR_NAPOT | ENTRY_FLAG_PERM_W | ENTRY_FLAG_PERM_R,
        .entry_pa_base = 0xe0000000,
        .entry_pa_length = 0x8000000,
    },

    [7] = {
        .entry_flag = ENTRY_FLAG_ADDR_TOR,
        .entry_pa_base = 0xffffffffff, /* 40-bit PA */
        .entry_pa_length = 0,
    },
};

#endif

typedef enum {
    VM_BOOT_SECTION_HEADER = 0,
    VM_BOOT_SECTION_DTB,
    VM_BOOT_SECTION_OPENSBI,
    VM_BOOT_SECTION_KERNEL,
    VM_BOOT_SECTION_MAX,
} vm_boot_section_t;

typedef struct  __attribute__((packed, aligned(4))) {
    uint32_t magic;
    uint32_t version;
    uint32_t sections;
    uint32_t crc;
    struct {
        uint32_t magic;
        uint32_t type;
        uint32_t offset;
        uint32_t size;
    } section[VM_BOOT_SECTION_MAX];
} vm_boot_header_t;

static char *sections[] = {
    "BootHeader",
    "dtb", 
    "OpenSBI", 
    "Kernel", 
    "Unknown"
};

static char *get_sectionstr(uint32_t type)
{
    if (type < VM_BOOT_SECTION_MAX) {
        return sections[type];
    }

    return sections[VM_BOOT_SECTION_MAX];
}

void linux_load()
{
    LOG_I("low_load start... \r\n");

    // for (int32_t i = 0; i < 10000000; i += 4) {
    //    vm_boot_header_t *header = (vm_boot_header_t *)BOOT_HDR_SRC_ADDR + i;
    //    if (header->magic == 0x4c4d5642) {
    //         LOG_I("Header at %d %p\r\n", i, header);
    //         break;
    //    }
    // }


    vm_boot_header_t *header = (vm_boot_header_t *)BOOT_HDR_SRC_ADDR;
    LOG_I("Header at 0x%08x\r\n", BOOT_HDR_SRC_ADDR);
    if (header->magic != 0x4c4d5642) {
        LOG_E("invalid boot header magic: 0x%08x\r\n", header->magic);
        return;
    }

    if (header->version != 1) {
        LOG_E("invalid boot header version: %d\r\n", header->version);
        return;
    }

    if (header->sections != 3) {
        LOG_E("invalid boot header sections: %d\r\n", header->sections);
        return;
    }

    for (int i = 0; i < header->sections; i++) {
        if (header->section[i].magic != 0x5c2381b2) {
            LOG_E("invalid boot section magic: 0x%08x\r\n", header->section[i].magic);
            continue;
        }
        LOG_I("Section %s(%d) - Start 0x%08x, Size %ld\r\n", get_sectionstr(header->section[i].type), header->section[i].type, BOOT_HDR_SRC_ADDR + header->section[i].offset, header->section[i].size);
        switch (header->section[i].type) {
            case VM_BOOT_SECTION_DTB:
                LOG_I("Copying DTB to 0x%08x...0x%08x\r\n", DTB_DST_ADDR, DTB_DST_ADDR + header->section[i].size);
                memcpy((void *)DTB_DST_ADDR, (void *)(BOOT_HDR_SRC_ADDR + (uintptr_t)header->section[i].offset), header->section[i].size);
                LOG_I("Done!\r\n");
                break;
            case VM_BOOT_SECTION_OPENSBI:
                LOG_I("Copying OpenSBI to 0x%08x...0x%08x\r\n", OPENSBI_DST_ADDR, OPENSBI_DST_ADDR + header->section[i].size);
                memcpy((void *)OPENSBI_DST_ADDR, (void *)(BOOT_HDR_SRC_ADDR + (uintptr_t)header->section[i].offset), header->section[i].size);
                LOG_I("Done!\r\n");
                break;
            case VM_BOOT_SECTION_KERNEL:
                LOG_I("Uncompressing Kernel to 0x%08x...\r\n", VM_LINUX_DST_ADDR);
                unlz4((const void *)(BOOT_HDR_SRC_ADDR + (uintptr_t)header->section[i].offset), (void *)VM_LINUX_DST_ADDR, header->section[i].size);
                LOG_I("Done!\r\n");
                break;
            default:
                LOG_E("Unhandled Section %d\r\n", header->section[i].type);
                return;
        }
        __DMB();
        __ISB();    
    }
    LOG_I("CRC: %08x\r\n", header->crc);
    csi_dcache_clean_invalid();

    return;
}

extern void bflb_uart_set_console(struct bflb_device_s *dev);

int main(void)
{
    board_init();
    LOG_I("");
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_init(gpio, GPIO_PIN_16, 21 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_17, 21 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);

    struct bflb_uart_config_s cfg;
    cfg.baudrate = 2000000;
    cfg.data_bits = UART_DATA_BITS_8;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.parity = UART_PARITY_NONE;
    cfg.flow_ctrl = 0;
    cfg.tx_fifo_threshold = 7;
    cfg.rx_fifo_threshold = 7;
    uart0 = bflb_device_get_by_name("uart3");
    bflb_uart_init(uart0, &cfg);
    bflb_uart_set_console(uart0);

    LOG_I("D0 start...\r\n");
    uint64_t start_time, stop_time;

    void (*opensbi)(int hart_id, int fdt_addr) = (void (*)(int hart_id, int fdt_addr))OPENSBI_DST_ADDR;

    start_time = bflb_mtimer_get_time_us();
    linux_load();
    stop_time = bflb_mtimer_get_time_us();
    LOG_I("load time: %ld us \r\n", (stop_time - start_time));

    __ASM volatile("csrw mcor, %0"
                   :
                   : "r"(0x30013));

    LOG_I("Setting PMP\r\n");
    pmp_status_type_e ret = rvpmp_init(pmp_entry_tab, sizeof(pmp_entry_tab) / sizeof(pmp_config_entry_t));
    if (ret != PMP_STATUS_OK) {
        LOG_E("PMP init failed: %d\r\n", ret);
        while (1) {
            /* dead? */
        }
    }
    __ISB();
    LOG_I("Booting OpenSBI at %p with DTB at 0x%08x\r\n", opensbi, DTB_DST_ADDR);
    opensbi(0, DTB_DST_ADDR);
    while (1) {
        LOG_W("Return from OpenSBI!!!\r\n");
        bflb_mtimer_delay_ms(1000);
    }
}
