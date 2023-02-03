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
#include <board.h>
#include <bl808.h>
#include <bl808_common.h>
#include <bl808_glb.h>
#include <ipc_reg.h>
#include <bflb_clock.h>
#include <bflb_gpio.h>
#include "oblfr_mailbox.h"
#define DBG_TAG "MBOX"
#include "log.h"

#ifndef CONFIG_CHIP_BL808
#error "This component is only for BL808"
#endif


typedef struct {
    char name[16];
    uint32_t irq;
    uint32_t count;
    void (*handler)(int irq, void *arg);
} mbox_irq_t;

static void Send_IPC_IRQ(int device);

#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_SDH
static void SDHCI_IRQHandler(int irq, void *arg)
{
    LOG_T("Got SDH IRQ\r\n");
    Send_IPC_IRQ(BFLB_IPC_DEVICE_SDHCI);
}
#endif

#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_UART2
static void UART2_IRQHandler(int irq, void *arg)
{
    LOG_T("Got UART IRQ\r\n");
    Send_IPC_IRQ(BFLB_IPC_DEVICE_UART2);
}
#endif

#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_USB
static void USB_IRQHandler(int irq, void *arg)
{
    LOG_T("Got USB IRQ\r\n");
    Send_IPC_IRQ(BLFB_IPC_DEVICE_USB);
}
#endif

#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_EMAC
static void EMAC_IRQHandler(int irq, void *arg)
{
    LOG_T("Got EMAC IRQ\r\n");
    Send_IPC_IRQ(BFLB_IPC_DEVICE_EMAC);
}
#endif

#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_GPIO
static void GPIO_IRQHandler(int irq, void *arg)
{
    LOG_T("Got GPIO IRQ\r\n");
    Send_IPC_IRQ(BFLB_IPC_DEVICE_GPIO);
}
#endif

static mbox_irq_t ipc_irqs[32] = {
#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_SDH
    [BFLB_IPC_DEVICE_SDHCI] = { "SDH", SDH_IRQn, 0, SDHCI_IRQHandler},
#endif
#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_UART2
    [BFLB_IPC_DEVICE_UART2] = { "UART2", UART2_IRQn, 0, UART2_IRQHandler},
#endif
#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_USB
    [BLFB_IPC_DEVICE_USB] = { "USB", USB_IRQn, 0, USB_IRQHandler},
#endif
#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_EMAC
    [BFLB_IPC_DEVICE_EMAC] = { "EMAC", EMAC_IRQn, 0, EMAC_IRQHandler},
#endif
#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_GPIO
    [BFLB_IPC_DEVICE_GPIO] = { "GPIO", GPIO_INT0_IRQn, 0, GPIO_IRQHandler},
#endif
};


static void Send_IPC_IRQ(int device)
{
    if (ipc_irqs[device].irq == 0) {
        LOG_E("Invalid IPC IRQ %d\r\n", device);
        return;
    }
    bflb_irq_disable(ipc_irqs[device].irq);
    BL_WR_REG(IPC2_BASE, IPC_CPU1_IPC_ISWR, (1 << device));
    ipc_irqs[device].count++;
}

void IPC_M0_IRQHandler(int irq, void *arg)
{
    int i;
    uint32_t irqStatus = BL_RD_REG(IPC0_BASE, IPC_CPU0_IPC_IRSRR);
    for (i = 0; i < sizeof(irqStatus) * 8; i++)
    {
        if (irqStatus & (1 << i)) {
            if (i == BFLB_IPC_DEVICE_MAILBOX) {
                LOG_I("Got Mailbox IRQ\r\n");
            } else {
                if (ipc_irqs[i].irq == 0) {
                    LOG_W("Got IPC IRQ for unknown peripheral %d\r\n", i);
                } else {
                    LOG_T("Got IPC EOI for Peripheral %s (%d - %d)\r\n", ipc_irqs[i].name, irqStatus, ipc_irqs[i].irq);
                    //assert(irqStatus == ipc_irqs[irqStatus].irq);                
                    bflb_irq_enable(ipc_irqs[i].irq);
                }
            }
        }
    }

    BL_WR_REG(IPC0_BASE, IPC_CPU0_IPC_ICR, irqStatus);
}

#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_SDH
oblfr_err_t setup_sdh_peripheral() {
    LOG_D("setting up SDH peripheral\r\n");
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_init(gpio, GPIO_PIN_0, GPIO_FUNC_SDH | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);
    bflb_gpio_init(gpio, GPIO_PIN_1, GPIO_FUNC_SDH | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);
    bflb_gpio_init(gpio, GPIO_PIN_2, GPIO_FUNC_SDH | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);
    bflb_gpio_init(gpio, GPIO_PIN_3, GPIO_FUNC_SDH | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);
    bflb_gpio_init(gpio, GPIO_PIN_4, GPIO_FUNC_SDH | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);
    bflb_gpio_init(gpio, GPIO_PIN_5, GPIO_FUNC_SDH | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);

    PERIPHERAL_CLOCK_SDH_ENABLE();
    LOG_D("SDH peripheral clock: %d\r\n", bflb_clk_get_peripheral_clock(BFLB_DEVICE_TYPE_SDH, 0)/1000000);
    return OBLFR_OK;
}
#endif

#ifdef COMPONENT_MAILBOX_IRQFWD_EMAC
static oblfr_err_t setup_emac_peripheral(void)
{
    GLB_GPIO_Cfg_Type gpio_cfg;
    int pin;

    PERIPHERAL_CLOCK_EMAC_ENABLE();

    struct bflb_device_s *gpio = bflb_device_get_by_name("gpio");
    for (pin = GLB_GPIO_PIN_24; pin <= GLB_GPIO_PIN_33; pin++) {
        bflb_gpio_init(gpio, pin, GPIO_FUNC_EMAC | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_2);
    }
    return OBLFR_OK;
}
#endif

oblfr_err_t oblfr_mailbox_init()
{
    int i;

    /* setup the IPC Interupt */
    bflb_irq_attach(IPC_M0_IRQn, IPC_M0_IRQHandler, NULL);
    BL_WR_REG(IPC0_BASE, IPC_CPU0_IPC_IUSR, 0xffffffff);
    bflb_irq_enable(IPC_M0_IRQn);

    /* register our Interupt Handlers to forward over */

    for (i = 0; i < sizeof(ipc_irqs) / sizeof(ipc_irqs[0]); i++) {
        if (ipc_irqs[i].irq != 0) {
            LOG_I("Forwarding Interupt %s (%d) to D0 (%p)\r\n", ipc_irqs[i].name, ipc_irqs[i].irq, ipc_irqs[i].handler);
            bflb_irq_attach(ipc_irqs[i].irq, ipc_irqs[i].handler, NULL);
            bflb_irq_enable(ipc_irqs[i].irq);
        }
    }

#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_SDH
    if (setup_sdh_peripheral() != SUCCESS) {
        LOG_E("Failed to setup SDH peripheral\r\n");
        return OBLFR_ERR_ERROR;
    }
#endif

#ifdef COMPONENT_MAILBOX_IRQFWD_EMAC
    if (setup_emac_peripheral() != SUCCESS) {
        LOG_E("Failed to setup EMAC peripheral\r\n");
        return OBLFR_ERR_ERROR;
    }
#endif

    return OBLFR_OK;
}

oblfr_err_t oblfr_mailbox_dump() 
{
    LOG_I("Mailbox IRQ Stats:\r\n");
    for (uint8_t i = 0; i < sizeof(ipc_irqs) / sizeof(ipc_irqs[0]); i++) {
        if (ipc_irqs[i].irq != 0) {
            LOG_I("Peripheral %s (%d): %d\r\n", ipc_irqs[i].name, ipc_irqs[i].irq, ipc_irqs[i].count);
        }
    }
    LOG_I("====================================\r\n");
    return OBLFR_OK;
}