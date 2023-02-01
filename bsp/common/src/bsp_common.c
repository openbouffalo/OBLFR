#include <bflb_flash.h>
#include <bflb_gpio.h>
#include <bflb_rtc.h>
#include "bsp_common.h"
#include "log.h"


#if (defined(CONFIG_LUA) || defined(CONFIG_BFLOG) || defined(CONFIG_FATFS))
static struct bflb_device_s *rtc;
#endif

void bl_show_log(void)
{
    LOG_I("\r\n");
    LOG_I("  ____                   ____               __  __      _       \r\n");
    LOG_I(" / __ \\                 |  _ \\             / _|/ _|    | |      \r\n");
    LOG_I("| |  | |_ __   ___ _ __ | |_) | ___  _   _| |_| |_ __ _| | ___  \r\n");
    LOG_I("| |  | | '_ \\ / _ \\ '_ \\|  _ < / _ \\| | | |  _|  _/ _` | |/ _ \\ \r\n");
    LOG_I("| |__| | |_) |  __/ | | | |_) | (_) | |_| | | | || (_| | | (_) |\r\n");
    LOG_I(" \\____/| .__/ \\___|_| |_|____/ \\___/ \\__,_|_| |_| \\__,_|_|\\___/ \r\n");
    LOG_I("       | |                                                      \r\n");
    LOG_I("       |_|                                                      \r\n");
    LOG_I("\r\n");
    LOG_I("Powered by BouffaloLab\r\n");
    LOG_I("Build:%s,%s\r\n", __TIME__, __DATE__);
    LOG_I("Copyright (c) 2023 OpenBouffalo team\r\n");
    LOG_I("Copyright (c) 2022 Bouffalolab team\r\n");
}

void bl_show_flashinfo(void)
{
    spi_flash_cfg_type flashCfg;
    uint8_t *pFlashCfg = NULL;
    uint32_t flashCfgLen = 0;
    uint32_t flashJedecId = 0;

    flashJedecId = bflb_flash_get_jedec_id();
    bflb_flash_get_cfg(&pFlashCfg, &flashCfgLen);
    arch_memcpy((void *)&flashCfg, pFlashCfg, flashCfgLen);
    LOG_I("=========== flash cfg ==============\r\n");
    LOG_I("jedec id   0x%06X\r\n", flashJedecId);
    LOG_I("mid            0x%02X\r\n", flashCfg.mid);
    LOG_I("iomode         0x%02X\r\n", flashCfg.io_mode);
    LOG_I("clk delay      0x%02X\r\n", flashCfg.clk_delay);
    LOG_I("clk invert     0x%02X\r\n", flashCfg.clk_invert);
    LOG_I("read reg cmd0  0x%02X\r\n", flashCfg.read_reg_cmd[0]);
    LOG_I("read reg cmd1  0x%02X\r\n", flashCfg.read_reg_cmd[1]);
    LOG_I("write reg cmd0 0x%02X\r\n", flashCfg.write_reg_cmd[0]);
    LOG_I("write reg cmd1 0x%02X\r\n", flashCfg.write_reg_cmd[1]);
    LOG_I("qe write len   0x%02X\r\n", flashCfg.qe_write_reg_len);
    LOG_I("cread support  0x%02X\r\n", flashCfg.c_read_support);
    LOG_I("cread code     0x%02X\r\n", flashCfg.c_read_mode);
    LOG_I("burst wrap cmd 0x%02X\r\n", flashCfg.burst_wrap_cmd);
    LOG_I("sector size:   0x%02X\r\n", flashCfg.sector_size);
    LOG_I("=====================================\r\n");
}

void board_common_setup_pinmux(pinmux_setup_t *pinmux_setup, uint32_t len) {
    uint32_t i;
    struct bflb_device_s *gpio;

#if (defined(CONFIG_LUA) || defined(CONFIG_BFLOG) || defined(CONFIG_FATFS))
    rtc = bflb_device_get_by_name("rtc");
#endif    

    gpio = bflb_device_get_by_name("gpio");

    for (i = 0; i < len; i++) {
        uint32_t cfg;
        if (pinmux_setup[i].mode & GPIO_ALTERNATE) {
            cfg = pinmux_setup[i].mode | pinmux_setup[i].pull | pinmux_setup[i].drive | pinmux_setup[i].func;
        } else {
            cfg = pinmux_setup[i].mode | pinmux_setup[i].pull | pinmux_setup[i].drive;
        }
        bflb_gpio_init(gpio, pinmux_setup[i].pin, cfg);
        LOG_D("pinmux_setup[%d] pin:%d, cfg:0x%08x\r\n", i, pinmux_setup[i].pin, cfg);
    }
    return;
}

#ifdef CONFIG_BFLOG
__attribute__((weak)) uint64_t bflog_clock(void)
{
    return bflb_mtimer_get_time_us();
}

__attribute__((weak)) uint32_t bflog_time(void)
{
    return BFLB_RTC_TIME2SEC(bflb_rtc_get_time(rtc));
}

__attribute__((weak)) char *bflog_thread(void)
{
    return "";
}
#endif

#ifdef CONFIG_LUA
__attribute__((weak)) clock_t luaport_clock(void)
{
    return (clock_t)bflb_mtimer_get_time_us();
}

__attribute__((weak)) time_t luaport_time(time_t *seconds)
{
    time_t t = (time_t)BFLB_RTC_TIME2SEC(bflb_rtc_get_time(rtc));
    if (seconds != NULL) {
        *seconds = t;
    }

    return t;
}
#endif

#ifdef CONFIG_FATFS
#include "bflb_timestamp.h"
__attribute__((weak)) uint32_t get_fattime(void)
{
    bflb_timestamp_t tm;

    bflb_timestamp_utc2time(BFLB_RTC_TIME2SEC(bflb_rtc_get_time(rtc)), &tm);

    return ((uint32_t)(tm.year - 1980) << 25) /* Year 2015 */
           | ((uint32_t)tm.mon << 21)         /* Month 1 */
           | ((uint32_t)tm.mday << 16)        /* Mday 1 */
           | ((uint32_t)tm.hour << 11)        /* Hour 0 */
           | ((uint32_t)tm.min << 5)          /* Min 0 */
           | ((uint32_t)tm.sec >> 1);         /* Sec 0 */
}
#endif