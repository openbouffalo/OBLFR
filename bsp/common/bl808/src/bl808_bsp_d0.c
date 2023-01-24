#include <bl808_glb.h>
//#include <bl808_psram_uhs.h>
//#include <bl808_ef_cfg.h>
//#include <bl808_uhs_phy.h>
//#include <bl808_tzc_sec.h>
//#include <bflb_clock.h>
#include <bflb_uart.h>
//#include <bflb_flash.h>
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

    bflb_gpio_init(gpio, GPIO_PIN_16, 21 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);
    bflb_gpio_init(gpio, GPIO_PIN_17, 21 | GPIO_ALTERNATE | GPIO_PULLUP | GPIO_SMT_EN | GPIO_DRV_1);

    struct bflb_uart_config_s cfg;
    cfg.baudrate = 2000000;
    cfg.data_bits = UART_DATA_BITS_8;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.parity = UART_PARITY_NONE;
    cfg.flow_ctrl = 0;
    cfg.tx_fifo_threshold = 7;
    cfg.rx_fifo_threshold = 7;

    console = bflb_device_get_by_name("uart3");

    bflb_uart_init(console, &cfg);
    bflb_uart_set_console(console);
}


void bl808_cpu_init(void)
{
    CPU_Set_MTimer_CLK(ENABLE, CPU_Get_MTimer_Source_Clock() / 1000 / 1000 - 1);

    bflb_irq_initialize();

    size_t heap_len = ((size_t)&__HeapLimit - (size_t)&__HeapBase);
    kmem_init((void *)&__HeapBase, heap_len);

    console_init();

    log_start();

    bl_show_log();

    LOG_I("dynamic memory init success,heap size = %d Kbyte \r\n", ((size_t)&__HeapLimit - (size_t)&__HeapBase) / 1024);

}




