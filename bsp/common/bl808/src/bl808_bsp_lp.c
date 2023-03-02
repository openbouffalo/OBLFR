#include <bl808_glb.h>
#include <bflb_uart.h>
#include <bl808_bsp_common.h>
#include "log.h"
#include <mem.h>

extern uint32_t __HeapBase;
extern uint32_t __HeapLimit;

static struct bflb_device_s *console;

extern void bflb_uart_set_console(struct bflb_device_s *dev);

static void console_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");

    bflb_gpio_init(gpio, GPIO_PIN_18, 21 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_19, 21 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);

    struct bflb_uart_config_s cfg;
    cfg.baudrate = 2000000;
    cfg.data_bits = UART_DATA_BITS_8;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.parity = UART_PARITY_NONE;
    cfg.flow_ctrl = 0;
    cfg.tx_fifo_threshold = 7;
    cfg.rx_fifo_threshold = 7;

    console = bflb_device_get_by_name("uart1");

    bflb_uart_init(console, &cfg);
    bflb_uart_set_console(console);
}


void bl808_cpu_init(void)
{
    CPU_Set_MTimer_CLK(ENABLE, CPU_Get_MTimer_Source_Clock() / 1000 / 1000 - 1);

    bflb_irq_initialize();

    console_init();

    log_start();

    bl_show_log();

}




