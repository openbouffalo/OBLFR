#include <stdio.h>
#include <oblfr_common.h>
#include <oblfr_rpmsg.h>
#include "network.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "lwip/snmp.h"
#include "netif/etharp.h"


#define DBG_TAG "RPMSGNET"
#include <log.h>


/* Bridge Interface */
struct netif netif_rpmsg;

oblfr_queue_entry_t *nethandle;

void rpmsg_net_status_cb(oblfr_device_cfg_t *device, device_status_t status) {
    LOG_D("rpmsg_net_status_cb: %d\r\n", status);
    if (status == DEVICE_UP) {
        netif_set_link_up(&netif_rpmsg);
    } else {
        netif_set_link_down(&netif_rpmsg);
    }
}

static err_t rpmsg_net_linkoutput(struct netif *netif, struct pbuf *p)
{
    LOG_D("%c%c - > rpmsg-net (%p)\r\n", netif->name[0], netif->name[1], p);
    LINK_STATS_INC(link.xmit);
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
    if (oblfr_rpmsg_device_send(nethandle, p->payload, p->len, OBLFR_BLOCK) != OBLFR_OK)
    {
        MIB2_STATS_NETIF_INC(netif, ifouterrors);
        LOG_W("send failed\r\n");
        return ERR_BUF;
    }

    return ERR_OK;
}

void rpmsg_net_rx(void *data, size_t len, void *priv)
{
    LOG_D("net_cb: %d %s\r\n", len, pcTaskGetTaskName(NULL));
    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL)
    {
        LOG_W("pbuf_alloc failed\r\n");
        return;
    }
    memcpy(p->payload, (uint8_t *)data, len);
    p->len = len;
    p->if_idx = netif_get_index(&netif_rpmsg);
    LINK_STATS_INC(link.recv);
    MIB2_STATS_NETIF_ADD(&netif_rpmsg, ifinoctets, p->tot_len);
    MIB2_STATS_NETIF_INC(&netif_rpmsg, ifinucastpkts);
    err_t err = netif_rpmsg.input(p, &netif_rpmsg);
    if (err != ERR_OK)
    {
        pbuf_free(p);
        MIB2_STATS_NETIF_INC(&netif_rpmsg, ifinerrors);
    }
    LOG_D("netrx %d\r\n", err);
}

err_t rpmsg_net_netif_init_cb(struct netif *netif)
{
    LOG_D("rpmsg_net_netif_init_cb\r\n");
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu = NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_UP | NETIF_FLAG_ETHERNET;
    netif->state = NULL;
    netif->name[0] = 'D';
    netif->name[1] = '0';
    netif->linkoutput = rpmsg_net_linkoutput;
    netif->output = etharp_output;
    return ERR_OK;
}


oblfr_err_t rpmsg_net_init(void)
{
    uint8_t *hwaddr = network_get_mac();
    //uint8_t hwaddr[6] = {0x00, 0x01, 0x00, 0x01, 0x08, 0xFF };

    static oblfr_device_cfg_t endpoint = {
        .name = "rpmsg-net",
        .cb = rpmsg_net_rx,
        .status_cb = rpmsg_net_status_cb,
        .priv = NULL,
    };
    nethandle = oblfr_rpmsg_device_add(&endpoint);


    netif_rpmsg.hwaddr_len = 6;
    memcpy(&netif_rpmsg.hwaddr, hwaddr, 6);
    if (netif_add(&netif_rpmsg, NULL, NULL, NULL, NULL, rpmsg_net_netif_init_cb, tcpip_input) == NULL) {
        LOG_E("netif_add failed\r\n");
        return OBLFR_ERR_INVALID;
    }

    net_add_port(&netif_rpmsg);
    //netif_set_link_up(&netif_rpmsg);
    netif_set_up(&netif_rpmsg);
    return OBLFR_OK;
}
