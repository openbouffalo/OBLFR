#include <usbd_core.h>
#include <usbd_rndis.h>
#include <lwip/init.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <lwip/snmp.h>
#include <netif/etharp.h>
#include "oblfr_common.h"
#include "network.h"

#define DBG_TAG "RNDIS"
#include <log.h>


static SemaphoreHandle_t rndis_lock = NULL;

/*!< endpoint address */
#define CDC_IN_EP  0x81
#define CDC_OUT_EP 0x02
#define CDC_INT_EP 0x83

#define USBD_VID           0xEFFF
#define USBD_PID           0xEFFF
#define USBD_MAX_POWER     100
#define USBD_LANGID_STRING 1033

/*!< config descriptor size */
#define USB_CONFIG_SIZE (9 + CDC_RNDIS_DESCRIPTOR_LEN)

/*!< global descriptor */
static const uint8_t cdc_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_RNDIS_DESCRIPTOR_INIT(0x00, CDC_INT_EP, CDC_OUT_EP, CDC_IN_EP, 0x02),
    ///////////////////////////////////////
    /// string0 descriptor
    ///////////////////////////////////////
    USB_LANGID_INIT(USBD_LANGID_STRING),
    ///////////////////////////////////////
    /// string1 descriptor
    ///////////////////////////////////////
    0x14,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ///////////////////////////////////////
    /// string2 descriptor
    ///////////////////////////////////////
    0x2A,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    'C', 0x00,                  /* wcChar0 */
    'h', 0x00,                  /* wcChar1 */
    'e', 0x00,                  /* wcChar2 */
    'r', 0x00,                  /* wcChar3 */
    'r', 0x00,                  /* wcChar4 */
    'y', 0x00,                  /* wcChar5 */
    'U', 0x00,                  /* wcChar6 */
    'S', 0x00,                  /* wcChar7 */
    'B', 0x00,                  /* wcChar8 */
    ' ', 0x00,                  /* wcChar9 */
    'R', 0x00,                  /* wcChar10 */
    'N', 0x00,                  /* wcChar11 */
    'D', 0x00,                  /* wcChar12 */
    'I', 0x00,                  /* wcChar13 */
    'S', 0x00,                  /* wcChar14 */
    ' ', 0x00,                  /* wcChar15 */
    'D', 0x00,                  /* wcChar16 */
    'E', 0x00,                  /* wcChar17 */
    'M', 0x00,                  /* wcChar18 */
    'O', 0x00,                  /* wcChar19 */
    ///////////////////////////////////////
    /// string3 descriptor
    ///////////////////////////////////////
    0x16,                       /* bLength */
    USB_DESCRIPTOR_TYPE_STRING, /* bDescriptorType */
    '2', 0x00,                  /* wcChar0 */
    '0', 0x00,                  /* wcChar1 */
    '2', 0x00,                  /* wcChar2 */
    '2', 0x00,                  /* wcChar3 */
    '1', 0x00,                  /* wcChar4 */
    '2', 0x00,                  /* wcChar5 */
    '3', 0x00,                  /* wcChar6 */
    '4', 0x00,                  /* wcChar7 */
    '5', 0x00,                  /* wcChar8 */
    '6', 0x00,                  /* wcChar9 */
#ifdef CONFIG_USB_HS
    ///////////////////////////////////////
    /// device qualifier descriptor
    ///////////////////////////////////////
    0x0a,
    USB_DESCRIPTOR_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x02,
    0x02,
    0x01,
    0x40,
    0x01,
    0x00,
#endif
    0x00
};


/* Bridge Interface */
struct netif netif_usb;

//struct eth_device rndis_dev;

void usbd_configure_done_callback(void)
{
    //LOG_I("usbd_configure_done_callback\r\n");
    netif_set_link_up(&netif_usb);
}

/* actually send the data out */
static err_t usb_netif_linkoutput(struct netif *netif, struct pbuf *p)
{
    if (xSemaphoreTake(rndis_lock, pdMS_TO_TICKS(1000)) != pdPASS) {
        LOG_E("Failed to take mbox signals list lock\r\n");
        return false;
    }
    // printf("len %d mtu %d\r\n", p->tot_len, netif->mtu);
    // if (p->tot_len >= netif->mtu ) {
    //      LOG_E("Packet too large\r\n");
    //      xSemaphoreGive(rndis_lock);
    //      return ERR_BUF;
    // }

    LOG_D("linkoutput_fn\r\n");
    static int ret;
    ret = usbd_rndis_eth_tx(p);
    LINK_STATS_INC(link.xmit);
    MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
    if (xSemaphoreGive(rndis_lock) != pdPASS) {
        LOG_E("Failed to give mbox signals list lock\r\n");
         MIB2_STATS_NETIF_INC(netif, ifouterrors);
        return false;
    }
    if (ret == 0)
        return ERR_OK;
    
    MIB2_STATS_NETIF_INC(netif, ifouterrors);
    return ERR_BUF;
}

err_t usb_netif_init_cb(struct netif *netif)
{
    LOG_D("usb_netif_init_cb\r\n");
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu = NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_UP | NETIF_FLAG_ETHERNET;
    netif->state = NULL;
    netif->name[0] = 'U';
    netif->name[1] = '1';
    netif->linkoutput = usb_netif_linkoutput;
    netif->output = etharp_output;
    return ERR_OK;
}

void usbd_rndis_data_recv(uint8_t *data, uint32_t len)
{
    LOG_D("RX Data %s %d\r\n", pcTaskGetName(NULL), len);
    err_t err = ERR_BUF;
    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p == NULL) {
        return;
    }
    memcpy(p->payload, (uint8_t *)data, len);
    p->len = len;
    p->if_idx = netif_get_index(&netif_usb);
    LINK_STATS_INC(link.recv);
    MIB2_STATS_NETIF_ADD(&netif_usb, ifinoctets, p->tot_len);
    MIB2_STATS_NETIF_INC(&netif_usb, ifinucastpkts);
    err = netif_usb.input(p, &netif_usb);

    if (err != ERR_OK)
    {
        pbuf_free(p);
        MIB2_STATS_NETIF_INC(&netif_usb, ifinerrors);
    }
    LOG_D("Done\r\n");
    return;
}


struct usbd_interface intf0;
struct usbd_interface intf1;

uint8_t *network_get_mac();
oblfr_err_t net_add_port(struct netif *netif);

void usb_rndis_init(void)
{

    rndis_lock = xSemaphoreCreateMutex();

    uint8_t *hwaddr = network_get_mac();
    hwaddr[5] += 2;

    netif_usb.hwaddr_len = 6;
    memcpy(&netif_usb.hwaddr, hwaddr, 6);
    if (netif_add(&netif_usb, NULL, NULL, NULL, NULL, usb_netif_init_cb, netif_input) == NULL) {
        LOG_E("netif_add failed\r\n");
        return;
    }
    netif_set_link_down(&netif_usb);
    netif_set_up(&netif_usb);

    net_add_port(&netif_usb);

    hwaddr[5] += 1;
    usbd_desc_register(cdc_descriptor);
    usbd_add_interface(usbd_rndis_init_intf(&intf0, CDC_OUT_EP, CDC_IN_EP, CDC_INT_EP, hwaddr));
    usbd_add_interface(usbd_rndis_init_intf(&intf1, CDC_OUT_EP, CDC_IN_EP, CDC_INT_EP, hwaddr));
    usbd_initialize();
}
