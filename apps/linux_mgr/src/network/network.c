/**
 * @file network.c
 * @brief Network Bridge
 *
 * Copyright (c) 2023 Justin Hammond
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

#include <stdio.h>
#include <board.h>
#include <bflb_mtimer.h>
#include <bflb_efuse.h>
#include "oblfr_mailbox.h"
#include "oblfr_system.h"
#include "config.h"
#include "sdkconfig.h"

#include "netif/etharp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/icmp.h"
#include "lwip/udp.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/inet.h"
#include "lwip/dns.h"
// #include "lwip/tcp_impl.h"
#include "lwip/tcp.h"
#include "netif/bridgeif.h"
#include "netif/bridgeif.h"
#include "lwip/apps/lwiperf.h"


#define DBG_TAG "NET"
#include <log.h>

#define NET_MTU 1492
#define BRIDGE_MAX_PORTS 6
#define FB_DYNAMIC_ENTRIES 16
#define FB_STATIC_ENT 16

/* we use a bridge here, so we can link between a local LWIP instance, 
 * Linux, via rpmsg-net, usb via rndis and in the future, wifi
 */

/* Local LWIP Interface*/
struct netif netif_br;

bridgeif_initdata_t bridgeif_cfg;

struct netif *ports[BRIDGE_MAX_PORTS];
uint8_t port_count = 0;

static uint8_t hwaddr_base[6];

// static const ip_addr_t ipaddr = IPADDR4_INIT_BYTES(192, 168, 5, 2);
// static const ip_addr_t netmask = IPADDR4_INIT_BYTES(255, 255, 255, 0);
// static const ip_addr_t gateway = IPADDR4_INIT_BYTES(192, 168, 5, 1);

uint8_t *network_get_mac()
{
    return hwaddr_base;
}
 
/* actually send the data out */
static err_t netif_linkoutput(struct netif *netif, struct pbuf *p)
{
    LOG_I("linkoutput_fn\r\n");
    static int ret;

    if (ret == 0)
        return ERR_OK;
    else
        return ERR_BUF;
}



err_t netif_br_init_cb(struct netif *netif)
{
    LOG_I("netif_local_init_cb\r\n");
    LWIP_ASSERT("netif != NULL", (netif != NULL));
    netif->mtu = NET_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP | NETIF_FLAG_ETHERNET;
    netif->state = NULL;
    netif->name[0] = 'M';
    netif->name[1] = '0';
    netif->linkoutput = netif_linkoutput;
    netif->output = etharp_output;
    return ERR_OK;
}

void status_callback(struct netif *eth_if)
{
	if (netif_is_up(eth_if)) {
        LOG_I("[%c%c] UP! local interface IP is %s\r\n",
					  eth_if->name[0], eth_if->name[1],
					  ip4addr_ntoa(netif_ip4_addr(eth_if)));
		LOG_I("[%c%c] IP is ready\r\n", eth_if->name[0], eth_if->name[1]);
	} else {
		LOG_I("[%c%c] netif_is_down\r\n", eth_if->name[0], eth_if->name[1]);
	}
}

void link_callback(struct netif *eth_if)
{
	if (netif_is_link_up(eth_if)) {
		LOG_I("[%c%c] Interface UP\r\n", eth_if->name[0], eth_if->name[1]);
	} else {
	    LOG_I("[%c%c] Interface DOWN\r\n", eth_if->name[0], eth_if->name[1]);
	}
}

oblfr_err_t net_add_port(struct netif *netif) {
    netif_set_status_callback(netif, status_callback);
    netif_set_link_callback(netif, link_callback);
    bridgeif_add_port(&netif_br, netif);
    ports[port_count++] = netif;
    return OBLFR_OK;
}

struct netif *net_get_port(uint8_t port) {
    if (port > port_count)
        return NULL;
    return ports[port];
}

uint8_t net_get_port_count() {
    return port_count;
}

static void lwiperf_report(void *arg, 
    enum lwiperf_report_type report_type, 
    const ip_addr_t *local_addr, 
    u16_t local_port,
    const ip_addr_t *remote_addr,
    u16_t remote_port,
    u32_t bytes_transferred,
    u32_t ms_duration,
    u32_t bandwidth_kbitpsec) 
{
    printf("lwiperf_report: type=%d, local: %s:%d, remote: %s:%d, bytes: %d, duration: %d, kbps: %d\r\n", 
        report_type, 
        ipaddr_ntoa(local_addr), 
        local_port, 
        ipaddr_ntoa(remote_addr), 
        remote_port, 
        bytes_transferred, 
        ms_duration, 
        bandwidth_kbitpsec);
}


oblfr_err_t network_init()
{
    const struct eth_addr ethbroadcast = {{0xff, 0xff, 0xff, 0xff, 0xff, 0xff}};

    tcpip_init(NULL, NULL);

    bflb_efuse_read_mac_address_opt(0, (uint8_t*)&hwaddr_base, 1);

    /* Bridge Config */
    bridgeif_cfg.max_ports = BRIDGE_MAX_PORTS;
    bridgeif_cfg.max_fdb_dynamic_entries  = FB_DYNAMIC_ENTRIES;
    bridgeif_cfg.max_fdb_static_entries  = FB_STATIC_ENT;
    bridgeif_cfg.ethaddr.addr[0] = hwaddr_base[0];
    bridgeif_cfg.ethaddr.addr[1] = hwaddr_base[1];
    bridgeif_cfg.ethaddr.addr[2] = hwaddr_base[2];
    bridgeif_cfg.ethaddr.addr[3] = hwaddr_base[3];
    bridgeif_cfg.ethaddr.addr[4] = hwaddr_base[4];
    bridgeif_cfg.ethaddr.addr[5] = hwaddr_base[5]+1;

    net_config_t *netcfg = config_get();

    if (netif_add(&netif_br, &netcfg->ipaddr, &netcfg->netmask, &netcfg->gw, &bridgeif_cfg, bridgeif_init, ethernet_input) == NULL) {
        LOG_E("netif_add failed\r\n");
        return OBLFR_ERR_INVALID;
    }
    netif_set_status_callback(&netif_br, status_callback);
    netif_set_link_callback(&netif_br, link_callback);
    netif_set_default(&netif_br);
    netif_set_up(&netif_br);
    netif_set_link_up(&netif_br);
    bridgeif_fdb_add(&netif_br, &ethbroadcast, BR_FLOOD);
    ports[port_count++] = &netif_br;

    dns_init();
    dns_setserver(0, &netcfg->dns[0]);
    dns_setserver(1, &netcfg->dns[1]);

    lwiperf_start_tcp_server_default(lwiperf_report, NULL);
    return OBLFR_OK;
}

