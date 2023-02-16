#ifndef _CONFIG_H_
#define _CONFIG_H_
/**
 * @file config.h
 * @brief Confifguration for the Linux Manager
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
#include <lwip/inet.h>
#include "oblfr_common.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t flags;
    ip_addr_t ipaddr;
    ip_addr_t netmask;
    ip_addr_t gw;
    ip_addr_t dns[2];
    char hostname[16];
} net_config_t;

oblfr_err_t config_init();
oblfr_err_t config_save();
net_config_t *config_get();

#ifdef __cplusplus
}
#endif
#endif