#ifndef __NETWORK_H__
#define __NETWORK_H__
/**
 * @file network.h
 * @brief Network Bridge functions between M0 and other peripherals
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

#ifdef __cplusplus
extern "C" {
#endif

#include <lwip/netif.h>

#define NET_MTU 1492

/* our Mac Addresses are assigned as follows: (we increment the last byte)
 * 1. Linux (D0) via rpmsg-net driver - Base address
 * 2. Bridge (M0) - Base address + 1 
 * 3. USB RNDIS (local) - Base address + 2
 * 4. USB RNDIS (remote) - Base address + 3
 * 4. Wifi - TBD. 
 */

uint8_t *network_get_mac();
oblfr_err_t network_init();
oblfr_err_t net_add_port(struct netif *netif);
struct netif *net_get_port(uint8_t port);
uint8_t net_get_port_count();

#ifdef __cplusplus
}
#endif
#endif