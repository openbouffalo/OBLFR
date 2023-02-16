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
#define BFLB_IPC_DEVICE_MBOX_TX     5
#define BFLB_IPC_DEVICE_MBOX_RX     6

/* MailBox Service */
#define BFLB_IPC_MBOX_VIRTIO            1

/* Operations for MBOX_VIRTIO */
#define BFLB_IPC_MBOX_VIRTIO_OP_KICK    1

typedef void (*mbox_signal_handler_t)(uint16_t service, uint16_t op, uint32_t param, void *arg);

oblfr_err_t oblfr_mailbox_init(void);
oblfr_err_t oblfr_mailbox_dump();

oblfr_err_t oblfr_mailbox_add_signal_handler(uint16_t service, uint16_t op, mbox_signal_handler_t handler, void *arg);
oblfr_err_t oblfr_mailbox_del_signal_handler(uint16_t service, uint16_t op );
oblfr_err_t oblfr_mailbox_send_signal(uint16_t service, uint16_t op, uint32_t data);
oblfr_err_t oblfr_mailbox_mask_signal(uint16_t service, uint16_t op);
oblfr_err_t oblfr_mailbox_unmask_signal(uint16_t service, uint16_t op);

#endif
