#include <stdbool.h>
#include <string.h>
#include <sys/queue.h>
#include <board.h>
#include <bl808.h>
#include <bl808_common.h>
#include <bl808_glb.h>
#include <ipc_reg.h>
#include <bflb_clock.h>
#include <bflb_gpio.h>
#ifdef CONFIG_FREERTOS
#include <FreeRTOS.h>
#include <semphr.h>
#endif

#include "oblfr_mailbox.h"
#include "oblfr_usb_peripheral.h"
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

typedef struct mbox_signal_s {
    uint16_t service;
    uint16_t op;
    mbox_signal_handler_t handler;
    void *arg;
    uint32_t count;
    bool masked;
    LIST_ENTRY(mbox_signal_s) list_entry;
} mbox_signal_t;

static LIST_HEAD(oblfr_mbox_signals, mbox_signal_s) oblfr_mbox_signals = LIST_HEAD_INITIALIZER(oblfr_mbox_signals);

typedef struct mbox_stats_s {
    uint32_t unhandled_irq;
    uint32_t unhandled_signals;
} mbox_stats_t;

static mbox_stats_t mbox_stats = {0};

#ifdef CONFIG_FREERTOS
static SemaphoreHandle_t oblfr_mbox_signals_lock = NULL;
#endif

static bool oblfr_mailbox_signal_lock(bool inisr) {
#ifdef CONFIG_FREERTOS
    if (inisr) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (xSemaphoreTakeFromISR(oblfr_mbox_signals_lock, &xHigherPriorityTaskWoken) != pdPASS) {
            LOG_E("Failed to take mbox signals list lock\r\n");
            return false;
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        if (xSemaphoreTake(oblfr_mbox_signals_lock, pdMS_TO_TICKS(2000)) != pdPASS) {
            LOG_E("Failed to take mbox signals list lock\r\n");
            return false;
        }
    }
#endif
    return true;
}

static bool oblfr_mailbox_signal_unlock(bool inisr) {
#ifdef CONFIG_FREERTOS
    if (inisr) {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (xSemaphoreGiveFromISR(oblfr_mbox_signals_lock, &xHigherPriorityTaskWoken) != pdPASS) {
            LOG_E("Failed to give mbox signals list lock\r\n");
            return false;
        }
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    } else {
        if (xSemaphoreGive(oblfr_mbox_signals_lock) != pdPASS) {
            LOG_E("Failed to give mbox signals list lock\r\n");
            return false;
        }
    }
#endif
    return true;
}

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

mbox_irq_t ipc_irqs[32] = {
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

static void oblfr_mailbox_signal_handler(int irq, void *arg) 
{
    struct mbox_signal_s *s;

    uint32_t signal = BL_RD_REG(IPC0_BASE, IPC_CPU1_IPC_ILSHR);
    uint32_t data = BL_RD_REG(IPC0_BASE, IPC_CPU1_IPC_ILSLR);

    uint16_t service = (signal >> 16);
    uint16_t op = (signal & 0xFFFF);

    LOG_D("Got IPC Mailbox Service: %d, Operation: %d Data: %d\r\n", service, op, data);

    bool handled = false;
    if (!oblfr_mailbox_signal_lock(true)) {
        LOG_W("%s Can't Take Signal Lock\r\n", __FUNCTION__);
        goto err;
    }
    LIST_FOREACH(s, &oblfr_mbox_signals, list_entry) {
        if ((s->service == service) & (s->op = op)) {
            handled = true;
            if (s->masked) {
                LOG_D("Signal %d masked\r\n", signal);
                break;
            }
            __asm volatile ("fence":::"memory");
            s->handler(service, op, data, s->arg);
            s->count++;
            break;
        }
    }
    oblfr_mailbox_signal_unlock(true);

    if (!handled) {
        LOG_W("Got IPC MBOX for unknown service %d, op %d\r\n", service, op);
        mbox_stats.unhandled_signals++;
    }

err:
    /* send a EOI */
    BL_WR_REG(IPC2_BASE, IPC_CPU1_IPC_ISWR, (1 << BFLB_IPC_DEVICE_MBOX_RX));
    LOG_D("Signal Handler Done\r\n");


}

void IPC_M0_IRQHandler(int irq, void *arg)
{
    int i;
    uint32_t irqStatus = BL_RD_REG(IPC0_BASE, IPC_CPU0_IPC_IRSRR);
    for (i = 0; i < sizeof(irqStatus) * 8; i++)
    {
        if (irqStatus & (1 << i)) {
            if (i == BFLB_IPC_DEVICE_MBOX_RX) {
                oblfr_mailbox_signal_handler(irq, arg);
                break;
            } else if (i == BFLB_IPC_DEVICE_MBOX_TX) {
                LOG_D("Got IPC MBOX TX EOI\r");
                break;
            } else {
                if (ipc_irqs[i].irq == 0) {
                    LOG_W("Got IPC IRQ for unknown peripheral %d\r\n", i);
                    mbox_stats.unhandled_irq++;
                } else {
                    LOG_T("Got IPC EOI for Peripheral %s (%d - %d)\r\n", ipc_irqs[i].name, irqStatus, ipc_irqs[i].irq);
                    bflb_irq_enable(ipc_irqs[i].irq);
                    break;
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

oblfr_err_t oblfr_mailbox_add_signal_handler(uint16_t service, uint16_t op, mbox_signal_handler_t handler, void *arg)
{
    struct mbox_signal_s *s;
    if (!oblfr_mailbox_signal_lock(false)) {
        LOG_W("%s Can't Take Signal Lock\r\n", __FUNCTION__);
        return OBLFR_ERR_TIMEOUT;
    }

    LIST_FOREACH(s, &oblfr_mbox_signals, list_entry) {
        if ((s->service == service) & (s->op = op)) {
            LOG_W("Service %d Op %d already registered\r\n", service, op);
            oblfr_mailbox_signal_unlock(false);
            return OBLFR_ERR_INVALID;
        }
    }

    mbox_signal_t *newsig = malloc(sizeof(mbox_signal_t));
    if (!newsig) {
        LOG_W("Can't allocate signal\r\n");
        oblfr_mailbox_signal_unlock(false);
        return OBLFR_ERR_NOMEM;
    }

    newsig->service = service;
    newsig->op = op;
    newsig->handler = handler;
    newsig->arg = arg;
    newsig->count = 0;
    newsig->masked = false;
    newsig->list_entry.le_next = NULL;

    LIST_INSERT_HEAD(&oblfr_mbox_signals, newsig, list_entry);
    LOG_D("Added Signal Handler for Service %d, Op %d\r\n", service, op);
    oblfr_mailbox_signal_unlock(false);
    return OBLFR_OK;
}

oblfr_err_t oblfr_mailbox_del_signal_handler(uint16_t service, uint16_t op) {
    struct mbox_signal_s *s;
    if (!oblfr_mailbox_signal_lock(false)) {
        LOG_W("Can't Take Signal Lock\r\n");
        return OBLFR_ERR_TIMEOUT;
    }

    LIST_FOREACH(s, &oblfr_mbox_signals, list_entry) {
        if ((s->service == service) & (s->op == op)) {
            LOG_D("Deleting Signal Handler for Service %d, op %d \r\n", service, op);
            LIST_REMOVE(s, list_entry);
            break;
        }
    }
    oblfr_mailbox_signal_unlock(false); 
    return OBLFR_OK;   
}

oblfr_err_t oblfr_mailbox_send_signal(uint16_t service, uint16_t op, uint32_t arg)
{
    uint32_t tmpVal = (service << 16) | op;
    BL_WR_REG(IPC2_BASE, IPC_CPU0_IPC_ILSHR, tmpVal);
    BL_WR_REG(IPC2_BASE, IPC_CPU0_IPC_ILSLR, arg);

    LOG_D("Sent IPC Mailbox Singal: service %d op: %d, arg %d\r\n", service, op, arg);

    /* trigger a IPC */
    BL_WR_REG(IPC2_BASE, IPC_CPU1_IPC_ISWR, (1 << BFLB_IPC_DEVICE_MBOX_TX));

    return OBLFR_OK;
}

oblfr_err_t oblfr_mailbox_mask_signal(uint16_t service, uint16_t op)
{
    struct mbox_signal_s *s;
    if (!oblfr_mailbox_signal_lock(false)) {
        LOG_W("Can't Take Signal Lock\r\n");
        return OBLFR_ERR_TIMEOUT;
    }

    LIST_FOREACH(s, &oblfr_mbox_signals, list_entry) {
        if ((s->service == service) & (s->op == op)) {
//              s->masked = true;
            LOG_D("Masked Signal Service %d, Op %d\r\n", service, op);
            break;
        }
    } 

    oblfr_mailbox_signal_unlock(false);
    return OBLFR_OK;
}

oblfr_err_t oblfr_mailbox_unmask_signal(uint16_t service, uint16_t op)
{
    struct mbox_signal_s *s;
    if (!oblfr_mailbox_signal_lock(false)) {
        LOG_W("Can't Take Signal Lock\r\n");
        return OBLFR_ERR_TIMEOUT;
    }
    LIST_FOREACH(s, &oblfr_mbox_signals, list_entry) {
        if ((s->service == service) & (s->op == op)) {
//            s->masked = false;
            LOG_D("Unmasked Signal Service %d, Op %d\r\n", service, op);    
            break;
        }
    }

    oblfr_mailbox_signal_unlock(false);     
    return OBLFR_OK;
}

oblfr_err_t oblfr_mailbox_init()
{
    int i;

#ifdef CONFIG_FREERTOS
    oblfr_mbox_signals_lock = xSemaphoreCreateMutex();
#endif

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

#ifdef CONFIG_COMPONENT_MAILBOX_IRQFWD_USB
    if (setup_usb_peripheral() != SUCCESS) {
        LOG_E("Failed to setup USB peripheral\r\n");
        return OBLFR_ERR_ERROR;
    }
#endif
    
    return OBLFR_OK;
}

oblfr_err_t oblfr_mailbox_dump() 
{
    struct mbox_signal_s *s;
    LOG_I("Mailbox IRQ Stats:\r\n");
    for (uint8_t i = 0; i < sizeof(ipc_irqs) / sizeof(ipc_irqs[0]); i++) {
        if (ipc_irqs[i].irq != 0) {
            LOG_I("\tPeripheral %s (%d): %d\r\n", ipc_irqs[i].name, ipc_irqs[i].irq, ipc_irqs[i].count);
        }
    }
    if (!oblfr_mailbox_signal_lock(false)) {
        LOG_W("%s Can't Take Signal Lock\r\n", __FUNCTION__);
        return OBLFR_ERR_TIMEOUT;
    }
    if (LIST_FIRST(&oblfr_mbox_signals)) {
        LOG_I("Mailbox Signal Stats:\r\n");
        LIST_FOREACH(s, &oblfr_mbox_signals, list_entry) {
            LOG_I("Service %d, Op %d: %d\r\n", s->service, s->op, s->count);
        }
    }
    LOG_I("Unhandled Interupts: %d Unhandled Signals %d\r\n", mbox_stats.unhandled_irq, mbox_stats.unhandled_signals);    
    LOG_I("====================================\r\n");

    oblfr_mailbox_signal_unlock(false);
    return OBLFR_OK;
}
