#include <lwip/opt.h>
#include <lwip/init.h>
#include <lwip/mem.h>
#include <lwip/icmp.h>
#include <lwip/netif.h>
#include <lwip/sys.h>
#include <lwip/inet.h>
#include <lwip/inet_chksum.h>
#include <lwip/ip.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>

#include "oblfr_common.h"
#include "oblfr_nvkvs.h"
#include "config.h"

#define DBG_TAG "CONFIG"
#include "log.h"

oblfr_nvkvs_handle_t *handle;

static net_config_t net_config;

static oblfr_err_t config_start_nvkvs() {
    if (handle != NULL) {
        return OBLFR_OK;
    }
    
    oblfr_kved_flash_driver_t kved_flash_drv = {
        .flash_addr = 0x7F0000,
    };

    oblfr_nvkvs_cfg_t nvkvs_cfg = {
        .storage = OBLFR_NVKVS_STORAGE_FLASH,
        .drv_cfg.flash = &kved_flash_drv,
    };
    handle = oblfr_nvkvs_init(&nvkvs_cfg);
    return OBLFR_OK;
}

oblfr_err_t config_init() {
    bool read_ip = true;

    config_start_nvkvs();

    if (handle == NULL) {
        LOG_E("Failed to init NVKVS\r\n");
        return OBLFR_ERR_ERROR;
    }
    if (oblfr_nvkvs_get_u32(handle, "flags", &net_config.flags) != OBLFR_OK) {
        net_config.flags = 0;
    }

    char ip_buf[16];

    if (oblfr_nvkvs_get_string(handle, "ip4", &ip_buf[0]) == OBLFR_OK) {
        LOG_I("Using IP Address %s\r\n", ip_buf);
        if (inet_aton(ip_buf, &net_config.ipaddr) == 0) {
            read_ip = false;
        }
    } else {
        read_ip = false;
    }
    if (read_ip == false ) {
        IP4_ADDR(&net_config.ipaddr, 192, 168, 5, 2);
        IP4_ADDR(&net_config.netmask, 255, 255, 255, 0);
        IP4_ADDR(&net_config.gw, 192, 168, 5, 1);
        IP4_ADDR(&net_config.dns[0], 8, 8, 8, 8);
        IP4_ADDR(&net_config.dns[1], 8, 8, 4, 4);        
    } else { 
        if (oblfr_nvkvs_get_string(handle, "nmsk", ip_buf) == OBLFR_OK) {
            if (inet_aton(ip_buf, &net_config.netmask) == 0) {
                LOG_E("Failed to parse netmask %s - Setting to /24", ip_buf);
                IP4_ADDR(&net_config.netmask, 255, 255, 255, 0);
            }
        } else {
            IP4_ADDR(&net_config.netmask, 255, 255, 255, 0);
        }
        if (oblfr_nvkvs_get_string(handle, "gw", ip_buf) == OBLFR_OK) {
            if (inet_aton(ip_buf, &net_config.gw) == 0) {
                LOG_E("Failed to parse gw %s - No Gateway Set!");
                read_ip = false;
            }
        }
        if (!ip_addr_isany_val(net_config.gw) && (oblfr_nvkvs_get_string(handle, "dns1", ip_buf) == OBLFR_OK)) {
            if (inet_aton(ip_buf, &net_config.dns[0]) == 0) {
                LOG_E("Failed to read DNS1 - Setting to 8.8.8.8");
                IP4_ADDR(&net_config.dns[0], 8, 8, 8, 8);
            }
        }
        if (!ip_addr_isany_val(net_config.gw) && (oblfr_nvkvs_get_string(handle, "dns2", ip_buf) == OBLFR_OK)) {
            if (inet_aton(ip_buf, &net_config.dns[1]) == 0) {
                LOG_E("Failed to read DNS1 - Setting to 8.8.4.4");
                IP4_ADDR(&net_config.dns[1], 8, 8, 4, 4);
            }
        }
    }
    if (!read_ip) {
        LOG_I("Using Default IP addresses\r\n");
    }
    LOG_I("Using IP Address %s\r\n", inet_ntoa(net_config.ipaddr));
    LOG_I("Using Netmask %s\r\n", inet_ntoa(net_config.netmask));
    if (ip_addr_isany_val(net_config.gw))
        LOG_E("No Gateway Set!\r\n");
    else
        LOG_I("Using Gateway %s\r\n", inet_ntoa(net_config.gw));

    LOG_I("Using DNS %s %s\r\n", inet_ntoa(net_config.dns[0]), inet_ntoa(net_config.dns[1]));

    return OBLFR_OK;
}

oblfr_err_t config_save() {

    config_start_nvkvs();

    if (handle == NULL) {
        LOG_E("Failed to init NVKVS\r\n");
        return OBLFR_ERR_ERROR;
    }
    if (oblfr_nvkvs_set_u32(handle, "flags", net_config.flags) != OBLFR_OK) {
        LOG_E("Failed to write flags\r\n");
    }
    if (oblfr_nvkvs_set_string(handle, "ip4", inet_ntoa(net_config.ipaddr)) != OBLFR_OK) {
        LOG_E("Failed to write ip4\r\n");
    }
    if (oblfr_nvkvs_set_string(handle, "nmsk", inet_ntoa(net_config.netmask)) != OBLFR_OK) {
        LOG_E("Failed to write nmsk\r\n");
    }
    if (oblfr_nvkvs_set_string(handle, "gw", inet_ntoa(net_config.gw)) != OBLFR_OK) {
        LOG_E("Failed to write gw\r\n");
    }
    if (oblfr_nvkvs_set_string(handle, "dns1", inet_ntoa(net_config.dns[0])) != OBLFR_OK) {
        LOG_E("Failed to write dns1\r\n");
    }
    if (oblfr_nvkvs_set_string(handle, "dns2", inet_ntoa(net_config.dns[1])) != OBLFR_OK) {
        LOG_E("Failed to write dns2\r\n");
    }

    LOG_I("Saved Config\r\n");

    return OBLFR_OK;
}

oblfr_err_t config_stop() {
    if (handle != NULL) {
        oblfr_nvkvs_deinit(handle);
        handle = NULL;
    }
    return OBLFR_OK; 
}

net_config_t *config_get() {
    return &net_config;
}