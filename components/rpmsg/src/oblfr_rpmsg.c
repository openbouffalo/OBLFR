
#include <bflb_l1c.h>
#include <sys/queue.h>
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>

#include "rpmsg_lite.h"
#include "rpmsg_ns.h"
#include "rpmsg_queue.h"
#include "oblfr_mailbox.h"
#include "oblfr_rpmsg.h"

#define DBG_TAG "RPMSG"
#include <log.h>

#define MAX_NUMBER_OF_ENDPOINTS 4
#define MAX_NUMBER_OF_QUEUED_MESSAGES RL_BUFFER_COUNT
#define MAX_QUEUESET_SIZE (MAX_NUMBER_OF_ENDPOINTS * MAX_NUMBER_OF_QUEUED_MESSAGES)

static struct rpmsg_lite_instance *ipc_rpmsg;
static rpmsg_ns_handle ipc_rpmsg_ns;

typedef struct oblfr_queue_entry_s
{
    rpmsg_queue_handle queue;
    oblfr_device_cfg_t *cfg;
    struct rpmsg_lite_endpoint *ept;
    uint32_t dst;
    bool valid;
    uint32_t sent;
    uint32_t received;
    LIST_ENTRY(oblfr_queue_entry_s)
    list_entry;
} oblfr_queue_entry_t;

static LIST_HEAD(oblfr_rpmsg_queues, oblfr_queue_entry_s) oblfr_rpmsg_queues = LIST_HEAD_INITIALIZER(oblfr_rpmsg_queues);

static QueueSetHandle_t xQueueSet;

#define XRAM_RINGBUF_ADDR 0x22048000

// WRAM -   0x22030000 - 160KB (0x28000)  - 0x22058000
// vring1 - 0x22048000 - 16K (0x4000)     - 0x2204C000
// vring2 - 0x2204C000 - 16K (0x4000)     - 0x22050000
// buffer - 0x22050000 - 32K (0x8000)     - 0x22058000

void oblfr_rpmsg_task(void *arg);

/* we shouldn't actually get any NS calls from Linux */
static void ipc_rpmsg_ns_callback(uint32_t new_ept, const char *new_ept_name, uint32_t flags, void *user_data)
{
    oblfr_queue_entry_t *rpmsgqueue;
    LOG_D("NS Announcement: Endpoint: %s - endpoint %d - flags %d\r", new_ept_name, new_ept, flags);
    LIST_FOREACH(rpmsgqueue, &oblfr_rpmsg_queues, list_entry)
    {
        if (strncasecmp(rpmsgqueue->cfg->name, new_ept_name, strlen(new_ept_name)) == 0)
        {
            if (rpmsgqueue->dst == RL_ADDR_ANY)
            {
                rpmsgqueue->dst = new_ept;
                LOG_D("Updated Device %s with endpoint %d - flags %d\r", new_ept_name, new_ept, flags);
                break;
            }
        }
    }
}

oblfr_err_t init_rpmsg()
{

    LOG_I("Initilizing RPMsg\r\n");

    bflb_l1c_dcache_disable();

    ipc_rpmsg = rpmsg_lite_remote_init((uintptr_t *)XRAM_RINGBUF_ADDR, RL_PLATFORM_BL808_M0_LINK_ID, RL_NO_FLAGS);
    LOG_I("rpmsg addr %lx, remaining %lx, total: %lx\r\n", ipc_rpmsg->sh_mem_base, ipc_rpmsg->sh_mem_remaining, ipc_rpmsg->sh_mem_total);
    if (ipc_rpmsg == NULL)
    {
        LOG_E("RPMSG init failed\r\n");
        return OBLFR_ERR_ERROR;
    }
    if (xTaskCreate(oblfr_rpmsg_task, "rpmsg", 1024, NULL, 5, NULL) != pdPASS)
    {
        LOG_E("Failed to create RPMSG task\r\n");
        rpmsg_lite_deinit(ipc_rpmsg);
        ipc_rpmsg = NULL;
        return OBLFR_ERR_ERROR;
    }
    return OBLFR_OK;
}

oblfr_queue_entry_t *oblfr_rpmsg_device_add(oblfr_device_cfg_t *cfg)
{
    oblfr_queue_entry_t *queue_entry = malloc(sizeof(oblfr_queue_entry_t));
    if (queue_entry == NULL)
    {
        LOG_E("Failed to allocate queue entry\r\n");
        return NULL;
    }
    queue_entry->cfg = cfg;

    queue_entry->queue = rpmsg_queue_create(ipc_rpmsg);
    if (queue_entry->queue == RL_NULL)
    {
        LOG_W("Failed to create RPMSG queue\r\n");
        rpmsg_queue_destroy(ipc_rpmsg, queue_entry->queue);
        return NULL;
    }

    /* first endpoint addr will be 0x1*/
    queue_entry->ept = rpmsg_lite_create_ept(ipc_rpmsg, RL_ADDR_ANY, rpmsg_queue_rx_cb, queue_entry->queue);
    if (queue_entry->ept == RL_NULL)
    {
        LOG_W("Failed to create RPMSG endpoint\r\n");
        rpmsg_queue_destroy(ipc_rpmsg, queue_entry->queue);
        return NULL;
    }

    LIST_INSERT_HEAD(&oblfr_rpmsg_queues, queue_entry, list_entry);
    if (xQueueAddToSet(queue_entry->queue, xQueueSet) != pdPASS)
    {
        LOG_W("Failed to add queue to set\r\n");
        rpmsg_lite_destroy_ept(ipc_rpmsg, queue_entry->ept);
        rpmsg_queue_destroy(ipc_rpmsg, queue_entry->queue);
        LIST_REMOVE(queue_entry, list_entry);
        return NULL;
    }

    LOG_D("New RPMsg Driver %s\r\n", queue_entry->cfg->name);

    /* only announce if the link is up */
    if (rpmsg_lite_is_link_up(ipc_rpmsg) == RL_TRUE)
    {
        if (rpmsg_ns_announce(ipc_rpmsg, queue_entry->ept, queue_entry->cfg->name, RL_NS_CREATE) != RL_SUCCESS)
        {
            LOG_W("Failed to announce RPMSG NS\r\n");
            rpmsg_lite_destroy_ept(ipc_rpmsg, queue_entry->ept);
            rpmsg_queue_destroy(ipc_rpmsg, queue_entry->queue);
            return NULL;
        }
        LOG_D("New Endpoint %s Announced\r\n", queue_entry->cfg->name);
    }
    queue_entry->dst = RL_ADDR_ANY;
    queue_entry->valid = true;
    queue_entry->sent = queue_entry->received = 0;
    return queue_entry;
}

oblfr_err_t oblfr_rpmsg_device_remove(oblfr_queue_entry_t *device)
{
    if (device == NULL)
    {
        LOG_W("Invalid Handle\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (device->valid == false)
    {
        LOG_W("Invalid Handle\r\n");
        return OBLFR_ERR_INVALID;
    }

    oblfr_err_t ret = OBLFR_OK;
    if (rpmsg_lite_is_link_up(ipc_rpmsg) == RL_TRUE)
    {
        if (rpmsg_ns_announce(ipc_rpmsg, device->ept, device->cfg->name, RL_NS_DESTROY) != RL_SUCCESS)
        {
            LOG_W("Failed to Destroy RPMSG Nameservice for %s\r\n", device->cfg->name);
            ret = OBLFR_ERR_ERROR;
        }
    }
    LIST_REMOVE(device, list_entry);
    if (xQueueRemoveFromSet(device->queue, xQueueSet) != pdPASS)
    {
        LOG_W("Failed to remove queue from set\r\n");
        ret = OBLFR_ERR_ERROR;
    }
    if (rpmsg_lite_destroy_ept(ipc_rpmsg, device->ept) != RL_SUCCESS)
    {
        LOG_W("Failed to destroy RPMSG endpoint\r\n");
        ret = OBLFR_ERR_ERROR;
    }

    if (rpmsg_queue_destroy(ipc_rpmsg, device->queue) != RL_SUCCESS)
    {
        LOG_W("Failed to destroy RPMSG queue\r\n");
        ret = OBLFR_ERR_ERROR;
    }
    LOG_D("RPMSG Driver %s removed: %d\r\n", device->cfg->name, ret);
    device->valid = false;
    free(device);
    device = NULL;
    return ret;
}

oblfr_err_t oblfr_rpmsg_device_send(oblfr_queue_entry_t *device, void *data, size_t len, OBLFR_Timeout timeout)
{
    if (device == NULL)
    {
        LOG_W("Invalid Handle\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (device->valid == false)
    {
        LOG_W("Invalid Handle\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (len > RL_BUFFER_PAYLOAD_SIZE)
    {
        LOG_W("Message too large\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (device->dst == RL_ADDR_ANY)
    {
        LOG_W("No destination address\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (rpmsg_lite_is_link_up(ipc_rpmsg) == RL_FALSE)
    {
        LOG_W("Link is down\r\n");
        return OBLFR_ERR_ERROR;
    }
    if (rpmsg_lite_send(ipc_rpmsg, device->ept, device->dst, data, len, (uint32_t)timeout) != RL_SUCCESS)
    {
        LOG_W("Failed to send message\r\n");
        return OBLFR_ERR_ERROR;
    }
    device->sent++;
    LOG_D("Sent message to %s\r\n", device->cfg->name);
    return OBLFR_OK;
}

void *oblfr_rpmsg_device_send_buffer_alloc(oblfr_queue_entry_t *device, uint32_t *size, OBLFR_Timeout timeout)
{
    if (device == NULL)
    {
        LOG_W("Invalid Handle\r\n");
        return NULL;
    }
    if (device->valid == false)
    {
        LOG_W("Invalid Handle\r\n");
        return NULL;
    }
    if (device->dst == RL_ADDR_ANY)
    {
        LOG_W("No destination address\r\n");
        return NULL;
    }
    if (rpmsg_lite_is_link_up(ipc_rpmsg) == RL_FALSE)
    {
        LOG_W("Link is down\r\n");
        return NULL;
    }
    LOG_D("Allocating buffer for %s\r\n", device->cfg->name);
    return rpmsg_lite_alloc_tx_buffer(ipc_rpmsg, size, (uint32_t)timeout);
}
oblfr_err_t oblfr_rpmsg_device_send_buffer(oblfr_queue_entry_t *device, void *buffer, size_t len)
{
    if (device == NULL)
    {
        LOG_W("Invalid Handle\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (device->valid == false)
    {
        LOG_W("Invalid Handle\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (len > RL_BUFFER_PAYLOAD_SIZE)
    {
        LOG_W("Message too large\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (device->dst == RL_ADDR_ANY)
    {
        LOG_W("No destination address\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (rpmsg_lite_is_link_up(ipc_rpmsg) == RL_FALSE)
    {
        LOG_W("Link is down\r\n");
        return OBLFR_ERR_INVALID;
    }
    if (rpmsg_lite_send_nocopy(ipc_rpmsg, device->ept, device->dst, buffer, len) != RL_SUCCESS)
    {
        LOG_W("Failed to send message\r\n");
        return OBLFR_ERR_ERROR;
    }
    device->sent++;
    LOG_D("Sent message to %s\r\n", device->cfg->name);
    return OBLFR_OK;
}

uint32_t oblfr_rpmsg_get_mtu(void)
{
    return RL_BUFFER_PAYLOAD_SIZE;
}

bool oblfr_rpmsg_is_ready(void)
{
    return rpmsg_lite_is_link_up(ipc_rpmsg);
}

oblfr_err_t oblfr_rpmsg_dump(void)
{
    oblfr_queue_entry_t *rpmsgqueue;
    if (rpmsg_lite_is_link_up(ipc_rpmsg) == RL_FALSE)
    {
        LOG_W("Link is down\r\n");
        return OBLFR_ERR_INVALID;
    }

    LOG_I("RPMSG Driver Dump\r\n");
    LOG_I("State: %s\r\n", rpmsg_lite_is_link_up(ipc_rpmsg) == RL_TRUE ? "UP" : "DOWN");
    LOG_I("Shared Memory Total: %ld Free: %ld\r\n", ipc_rpmsg->sh_mem_total, ipc_rpmsg->sh_mem_remaining);
    LOG_I("RPMSG MTU %ld\r\n", oblfr_rpmsg_get_mtu());
    LIST_FOREACH(rpmsgqueue, &oblfr_rpmsg_queues, list_entry)
    {
        LOG_I("Endpoint: %s - Addr: %ld Sent: %ld Recv: %ld\r\n", rpmsgqueue->cfg->name, rpmsgqueue->dst, rpmsgqueue->sent, rpmsgqueue->received);
    }
    LOG_I("========================================\r\n");
    return OBLFR_OK;
}

void oblfr_process_queue(oblfr_queue_entry_t *rpmsgqueue)
{
    char *rx_msg;
    uint32_t len;
    if (rpmsg_queue_recv_nocopy(ipc_rpmsg, rpmsgqueue->queue, &rpmsgqueue->dst, &rx_msg, &len, RL_DONT_BLOCK) == RL_SUCCESS)
    {
        rpmsgqueue->cfg->cb(rx_msg, len, rpmsgqueue->cfg->priv);
        if (rpmsg_queue_nocopy_free(ipc_rpmsg, rx_msg) != RL_SUCCESS)
        {
            LOG_W("Failed to free rpmsg buffer\r\n");
        }
        rpmsgqueue->received++;
    }
}

void oblfr_rpmsg_task(void *arg)
{
    xQueueSet = xQueueCreateSet(MAX_QUEUESET_SIZE);
    oblfr_queue_entry_t *rpmsgqueue;

    LOG_I("Waiting for RPMSG link up\r\n");
    while (0 == rpmsg_lite_is_link_up(ipc_rpmsg))
    {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    LOG_I("RPMSG link up: MTU Size: %d\r\n", oblfr_rpmsg_get_mtu());

    ipc_rpmsg_ns = rpmsg_ns_bind(ipc_rpmsg, ipc_rpmsg_ns_callback, NULL);
    if (ipc_rpmsg_ns == RL_NULL)
    {
        LOG_W("Failed to bind RPMSG NS\r\n");
        rpmsg_lite_deinit(ipc_rpmsg);
        ipc_rpmsg = NULL;
        vTaskDelete(NULL);
        return;
    }
    LOG_D("RPMSG NS binded\r\n");

    /* go through any registered endpoints and annouce them again */
    LIST_FOREACH(rpmsgqueue, &oblfr_rpmsg_queues, list_entry)
    {
        /* ignore errors */
        if (rpmsg_ns_announce(ipc_rpmsg, rpmsgqueue->ept, rpmsgqueue->cfg->name, RL_NS_CREATE) != RL_SUCCESS)
        {
            LOG_W("Failed to announce device %s\r\n", rpmsgqueue->cfg->name);
        }
        else
        {
            LOG_D("Device %s announced\r\n", rpmsgqueue->cfg->name);
        }
    }

    while (1)
    {
        QueueSetMemberHandle_t xActivatedMember = xQueueSelectFromSet(xQueueSet, portMAX_DELAY);
        bool handled = false;
        LIST_FOREACH(rpmsgqueue, &oblfr_rpmsg_queues, list_entry)
        {
            if (rpmsgqueue->queue == xActivatedMember)
            {
                LOG_D("Received message on queue %s\r\n", rpmsgqueue->cfg->name);
                oblfr_process_queue(rpmsgqueue);
                handled = true;
                break;
            }
        }
        if (!handled)
        {
            LOG_E("Received message on unknown queue\r\n");
        }
    }

    return;
}
