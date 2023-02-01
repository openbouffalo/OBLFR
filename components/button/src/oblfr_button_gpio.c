// Copyright 2020 Espressif Systems (Shanghai) Co. Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "bflb_gpio.h"
#include "oblfr_button_gpio.h"

#define DBG_TAG "BTN"
#include "log.h"


#define GPIO_BTN_CHECK(a, str, ret_val)                          \
    if (!(a))                                                     \
    {                                                             \
        LOG_E("%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

oblfr_err_t oblfr_button_gpio_init(const oblfr_button_gpio_config_t *config)
{
    GPIO_BTN_CHECK(NULL != config, "Pointer of config is invalid", OBLFR_ERR_INVALID);
    /* GPIO_BTN_CHECK(GPIO_IS_VALID_GPIO(config->gpio_num), "GPIO number error", OBLFR_ERR_INVALID); */

    LOG_D("oblfr_button_gpio_init %d\r\n", config->gpio_num);

    struct bflb_device_s *gpio = bflb_device_get_by_name("gpio");

    uint32_t cfg = 0;
    if (config->active_level) 
        cfg = GPIO_INPUT | GPIO_PULLDOWN | GPIO_DRV_1;
    else 
        cfg = GPIO_INPUT | GPIO_PULLUP | GPIO_DRV_1;
    bflb_gpio_init(gpio, config->gpio_num, cfg);

    return OBLFR_OK;
}

oblfr_err_t oblfr_button_gpio_deinit(int gpio_num)
{
    LOG_D("oblfr_button_gpio_deinit %d\r\n", gpio_num);

    struct bflb_device_s *gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_deinit(gpio, gpio_num);
    return OBLFR_OK;
}

uint8_t oblfr_button_gpio_get_key_level(void *gpio_num)
{
    struct bflb_device_s *gpio = bflb_device_get_by_name("gpio");
    return bflb_gpio_read(gpio, (uint32_t)gpio_num);
}
