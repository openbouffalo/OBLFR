#include "bflb_mtimer.h"
#include "bflb_gpio.h"
#include "board.h"
#include "log.h"
#include "sdkconfig.h"

int main(void)
{
    board_init();
    struct bflb_device_s *gpio;
    gpio = bflb_device_get_by_name("gpio");
#ifdef CONFIG_PINMUX_ENABLE_LED1
    bflb_gpio_reset(gpio, BSP_GPIO_LED1);
#endif
#ifdef CONFIG_PINMUX_ENABLE_LED2
    bflb_gpio_reset(gpio, BSP_GPIO_LED2);
#endif
#ifdef CONFIG_PINMUX_ENABLE_LED3
    bflb_gpio_reset(gpio, BSP_GPIO_LED3);
#endif
    bool led = false;
    while (1) {
        LOG_F("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_E("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_W("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_I("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_D("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_T("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
#ifdef CONFIG_PINMUX_ENABLE_LED1
        if (led == true) {
            bflb_gpio_reset(gpio, BSP_GPIO_LED1);
#ifdef CONFIG_PINMUX_ENABLE_LED3
            bflb_gpio_set(gpio, BSP_GPIO_LED3);
#endif
            led = false;
        } else { 
#ifdef CONFIG_PINMUX_ENABLE_LED3
            bflb_gpio_reset(gpio, BSP_GPIO_LED3);
#endif
            bflb_gpio_set(gpio, BSP_GPIO_LED1);
            led = true;
        }
#endif
#ifdef CONFIG_PINMUX_ENABLE_BTN1
        bool led2 = false;
        if (!bflb_gpio_read(gpio, BSP_GPIO_BTN1)) {
            LOG_I("Button 1 is pressed\r\n");
        }
#endif
#ifdef CONFIG_PINMUX_ENABLE_BTN2
        if (!bflb_gpio_read(gpio, BSP_GPIO_BTN2)) {
            LOG_I("Button 2 is pressed\r\n");
        }
#endif
        bflb_mtimer_delay_ms(CONFIG_EXAMPLE_INTERVAL);
    }
}
