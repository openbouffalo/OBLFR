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


oblfr_err_t oblfr_mailbox_init(void);

oblfr_err_t oblfr_mailbox_dump();

#endif
