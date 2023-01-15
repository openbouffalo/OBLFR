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

#include <bl808_common.h>
#include <bflb_irq.h>
#include <bl808_ipc.h>
#include <ipc_reg.h>
#include <sdh_reg.h>
#include <log.h>
#include <board.h>
#include "irq-forward.h"
#include "sdkconfig.h"

static uint32_t ipc_irqs[32] = {
#ifdef CONFIG_LL_IRQFWD_SDH
    [BFLB_IPC_DEVICE_SDHCI] = SDH_IRQn,
#endif
#ifdef CONFIG_LL_IRQFWD_UART2
    [BFLB_IPC_DEVICE_UART2] = UART2_IRQn,
#endif
#ifdef CONFIG_LL_IRQFWD_USB
    [BLFB_IPC_DEVICE_USB] = USB_IRQn,
#endif
    0,
};

static void Send_IPC_IRQ(int device)
{
    bflb_irq_disable(ipc_irqs[device]);
    BL_WR_REG(IPC2_BASE, IPC_CPU1_IPC_ISWR, (1 << device));
}

#ifdef CONFIG_LL_IRQFWD_SDH
void SDH_MMC1_IRQHandler(int irq, void *arg)
{
    Send_IPC_IRQ(BFLB_IPC_DEVICE_SDHCI);
}
#endif

#ifdef CONFIG_LL_IRQFWD_UART2
void UART2_IRQHandler(int irq, void *arg)
{
    Send_IPC_IRQ(BFLB_IPC_DEVICE_UART2);
}
#endif

#ifdef CONFIG_LL_IRQFWD_USB
void USB_IRQHandler(int irq, void *arg)
{
    Send_IPC_IRQ(BLFB_IPC_DEVICE_USB);
}
#endif

void IPC_M0_IRQHandler(int irq, void *arg)
{
    int i;
    uint32_t irqStatus = IPC_M0_Get_Int_Raw_Status();

    for (i = 0; i < sizeof(irqStatus) * 8; i++)
    {
        if (irqStatus & (1 << i))
            bflb_irq_enable(ipc_irqs[i]);
    }

    BL_WR_REG(IPC0_BASE, IPC_CPU0_IPC_ICR, irqStatus);
}

int main(void)
{
    board_init();

    LOG_I("M0 CPU start...\r\n");

    LOG_I("registering IPC interrupt handler\r\n");

    bflb_irq_attach(IPC_M0_IRQn, IPC_M0_IRQHandler, NULL);
    IPC_M0_Int_Unmask_By_Word(0xffffffff);
    bflb_irq_enable(IPC_M0_IRQn);

#ifdef CONFIG_LL_IRQFWD_SDH
    LOG_I("registering SDH interrupt handler\r\n");

    bflb_irq_attach(SDH_IRQn, SDH_MMC1_IRQHandler, NULL);
    bflb_irq_enable(SDH_IRQn);
#endif

#ifdef CONFIG_LL_IRQFWD_UART2
    LOG_I("registering UART2 interrupt handler\r\n");

    bflb_irq_attach(UART2_IRQn, UART2_IRQHandler, NULL);
    bflb_irq_enable(UART2_IRQn);
#endif

#ifdef CONFIG_LL_IRQFWD_USB
    LOG_I("registering USB interrupt handler\r\n");

    bflb_irq_attach(USB_IRQn, UART2_IRQHandler, NULL);
    bflb_irq_enable(USB_IRQn);
#endif

    while (1)
    {
        {
            static int x = 0;
            if ((x++ % 999999999) == 0)
            {
                {
                    // dump_ipc(IPC0_BASE);
                    // dump_ipc(IPC1_BASE);
                    LOG_I(".\n");
                }
            }
        }
        int dummy;
        /* In lieu of a halt instruction, induce a long-latency stall. */
        __asm__ __volatile__("div %0, %0, zero"
                             : "=r"(dummy));
    }
}
