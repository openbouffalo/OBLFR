#ifndef __OBLFR_MAILBOX_H__
#define __OBLFR_MAILBOX_H__

#include <stdint.h>
#include <stdbool.h>

#include "oblfr_common.h"
#include "sdkconfig.h"

/* Peripheral device ID */
#define BFLB_IPC_DEVICE_SDHCI		0
#define BFLB_IPC_DEVICE_UART2		1
#define BLFB_IPC_DEVICE_USB         2
#define BFLB_IPC_DEVICE_EMAC		3
#define BFLB_IPC_DEVICE_GPIO        4  
#define BFLB_IPC_DEVICE_MAILBOX     5


typedef void (*mbox_signal_handler_t)(uint32_t signal, uint32_t param, void *arg);

oblfr_err_t oblfr_mailbox_init(void);
oblfr_err_t oblfr_mailbox_dump();

oblfr_err_t oblfr_mailbox_add_signal_handler(uint32_t signal, mbox_signal_handler_t handler, void *arg);
oblfr_err_t oblfr_mailbox_del_signal_handler(uint32_t signal);
oblfr_err_t oblfr_mailbox_send_signal(uint32_t signal, uint32_t data);
oblfr_err_t oblfr_mailbox_mask_signal(uint32_t signal);
oblfr_err_t oblfr_mailbox_unmask_signal(uint32_t signal);

#endif
