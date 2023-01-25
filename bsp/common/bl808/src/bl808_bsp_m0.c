#include <bl808_glb.h>
#include <bl808_psram_uhs.h>
#include <bl808_ef_cfg.h>
#include <bl808_uhs_phy.h>
#include <bl808_tzc_sec.h>
#include <bflb_clock.h>
#include <bflb_uart.h>
#include <bflb_flash.h>
#include <bl808_bsp_common.h>
#include "log.h"
#include <mem.h>


static struct bflb_device_s *console;
extern void bflb_uart_set_console(struct bflb_device_s *dev);

extern uint32_t __HeapBase;
extern uint32_t __HeapLimit;

void system_clock_init(void)
{
    /* wifipll/audiopll */
    GLB_Power_On_XTAL_And_PLL_CLK(GLB_XTAL_40M, GLB_PLL_WIFIPLL |
                                                    GLB_PLL_CPUPLL |
                                                    GLB_PLL_UHSPLL |
                                                    GLB_PLL_MIPIPLL);

    GLB_Set_MCU_System_CLK(GLB_MCU_SYS_CLK_WIFIPLL_320M);
    GLB_Set_DSP_System_CLK(GLB_DSP_SYS_CLK_CPUPLL_400M);
    GLB_Config_CPU_PLL(GLB_XTAL_40M, cpuPllCfg_480M);

    CPU_Set_MTimer_CLK(ENABLE, CPU_Get_MTimer_Source_Clock() / 1000 / 1000 - 1);
}

void peripheral_clock_init(void)
{
    PERIPHERAL_CLOCK_ADC_DAC_ENABLE();
    PERIPHERAL_CLOCK_SEC_ENABLE();
    PERIPHERAL_CLOCK_DMA0_ENABLE();
    PERIPHERAL_CLOCK_UART0_ENABLE();
    PERIPHERAL_CLOCK_UART1_ENABLE();
    PERIPHERAL_CLOCK_SPI0_1_ENABLE();
    PERIPHERAL_CLOCK_I2C0_ENABLE();
    PERIPHERAL_CLOCK_PWM0_ENABLE();
    PERIPHERAL_CLOCK_TIMER0_1_WDG_ENABLE();
    PERIPHERAL_CLOCK_IR_ENABLE();
    PERIPHERAL_CLOCK_I2S_ENABLE();
    PERIPHERAL_CLOCK_USB_ENABLE();
    PERIPHERAL_CLOCK_CAN_UART2_ENABLE();

    GLB_Set_ADC_CLK(ENABLE, GLB_ADC_CLK_XCLK, 4);
    GLB_Set_UART_CLK(ENABLE, HBN_UART_CLK_XCLK, 0);
    GLB_Set_DSP_UART0_CLK(ENABLE, GLB_DSP_UART_CLK_DSP_XCLK, 0);
    GLB_Set_SPI_CLK(ENABLE, GLB_SPI_CLK_MCU_MUXPLL_160M, 0);
    GLB_Set_I2C_CLK(ENABLE, GLB_I2C_CLK_XCLK, 0);
    GLB_Set_IR_CLK(ENABLE, GLB_IR_CLK_SRC_XCLK, 19);
    GLB_Set_ADC_CLK(ENABLE, GLB_ADC_CLK_XCLK, 1);
    GLB_Set_DIG_CLK_Sel(GLB_DIG_CLK_XCLK);
    GLB_Set_DIG_512K_CLK(ENABLE, ENABLE, 0x4E);
    GLB_Set_PWM1_IO_Sel(GLB_PWM1_IO_DIFF_END);
    GLB_Set_CAM_CLK(ENABLE, GLB_CAM_CLK_WIFIPLL_96M, 3);

    GLB_Set_PKA_CLK_Sel(GLB_PKA_CLK_MCU_MUXPLL_160M);

#ifdef CONFIG_BSP_CSI
    GLB_CSI_Config_MIPIPLL(2, 0x21000);
    GLB_CSI_Power_Up_MIPIPLL();
    GLB_Set_DSP_CLK(ENABLE, GLB_DSP_CLK_MUXPLL_160M, 1);
#endif
    GLB_Set_USB_CLK_From_WIFIPLL(1);
}

#ifdef CONFIG_PSRAM
#define WB_4MB_PSRAM   (1)
#define UHS_32MB_PSRAM (2)
#define UHS_64MB_PSRAM (3)
#define WB_32MB_PSRAM  (4)
#define NONE_UHS_PSRAM (-1)

int uhs_psram_init(void)
{
    PSRAM_UHS_Cfg_Type psramDefaultCfg = {
        2000,
        PSRAM_MEM_SIZE_32MB,
        PSRAM_PAGE_SIZE_2KB,
        PSRAM_UHS_NORMAL_TEMP,
    };

    bflb_efuse_device_info_type chip_info;
    bflb_ef_ctrl_get_device_info(&chip_info);
    if (chip_info.psramInfo == UHS_32MB_PSRAM) {
        psramDefaultCfg.psramMemSize = PSRAM_MEM_SIZE_32MB;
    } else if (chip_info.psramInfo == UHS_64MB_PSRAM) {
        psramDefaultCfg.psramMemSize = PSRAM_MEM_SIZE_64MB;
    } else {
        return -1;
    }

    //init uhs PLL; Must open uhs pll first, and then initialize uhs psram
    GLB_Config_UHS_PLL(GLB_XTAL_40M, uhsPllCfg_2000M);
    //init uhs psram ;
    // Psram_UHS_x16_Init(Clock_Peripheral_Clock_Get(BL_PERIPHERAL_CLOCK_PSRAMA) / 1000000);
    Psram_UHS_x16_Init_Override(&psramDefaultCfg);
    Tzc_Sec_PSRAMA_Access_Release();

    // example: 2000Mbps typical cal values
    uhs_phy_cal_res->rl = 39;
    uhs_phy_cal_res->rdqs = 3;
    uhs_phy_cal_res->rdq = 0;
    uhs_phy_cal_res->wl = 13;
    uhs_phy_cal_res->wdqs = 4;
    uhs_phy_cal_res->wdq = 5;
    uhs_phy_cal_res->ck = 9;
    /* TODO: use uhs psram trim update */
    set_uhs_latency_r(uhs_phy_cal_res->rl);
    cfg_dqs_rx(uhs_phy_cal_res->rdqs);
    cfg_dq_rx(uhs_phy_cal_res->rdq);
    set_uhs_latency_w(uhs_phy_cal_res->wl);
    cfg_dq_drv(uhs_phy_cal_res->wdq);
    cfg_ck_cen_drv(uhs_phy_cal_res->wdq + 4, uhs_phy_cal_res->wdq + 1);
    cfg_dqs_drv(uhs_phy_cal_res->wdqs);
    // set_odt_en();
    mr_read_back();
    return 0;
}
#endif

void console_init()
{
    struct bflb_device_s *gpio;

    gpio = bflb_device_get_by_name("gpio");
    bflb_gpio_uart_init(gpio, GPIO_PIN_14, GPIO_UART_FUNC_UART0_TX);
    bflb_gpio_uart_init(gpio, GPIO_PIN_15, GPIO_UART_FUNC_UART0_RX);

    struct bflb_uart_config_s cfg;
    cfg.baudrate = 2000000;
    cfg.data_bits = UART_DATA_BITS_8;
    cfg.stop_bits = UART_STOP_BITS_1;
    cfg.parity = UART_PARITY_NONE;
    cfg.flow_ctrl = 0;
    cfg.tx_fifo_threshold = 7;
    cfg.rx_fifo_threshold = 7;

    console = bflb_device_get_by_name("uart0");
    bflb_uart_init(console, &cfg);
    bflb_uart_set_console(console);
}


#if defined(CONFIG_PINMUX_ENABLE_SDH)
void enable_sdh_periheral() {    
    LOG_D("Enabling SD Card\r\n");
    PERIPHERAL_CLOCK_SDH_ENABLE();
    uint32_t tmp_val;
    tmp_val = BL_RD_REG(PDS_BASE, PDS_CTL5);
    uint32_t tmp_val2 = BL_GET_REG_BITS_VAL(tmp_val, PDS_CR_PDS_GPIO_KEEP_EN);
    tmp_val2 &= ~(1 << 0);
    tmp_val = BL_SET_REG_BITS_VAL(tmp_val, PDS_CR_PDS_GPIO_KEEP_EN, tmp_val2);
    BL_WR_REG(PDS_BASE, PDS_CTL5, tmp_val);
    GLB_AHB_MCU_Software_Reset(GLB_AHB_MCU_SW_SDH);
    LOG_D("SDH peripheral clock: %d\r\n", bflb_clk_get_peripheral_clock(BFLB_DEVICE_TYPE_SDH, 0)/1000000);
}
#endif

void bl808_cpu_init(void)
{
    int ret = -1;
    uintptr_t flag;

    flag = bflb_irq_save();

    GLB_Halt_CPU(GLB_CORE_ID_D0);

    ret = bflb_flash_init();

    system_clock_init();
    peripheral_clock_init();
    bflb_irq_initialize();
    console_init();
#if defined(CONFIG_PINMUX_ENABLE_SDH)
    enable_sdh_periheral();
#endif

    size_t heap_len = ((size_t)&__HeapLimit - (size_t)&__HeapBase);
    kmem_init((void *)&__HeapBase, heap_len);

    log_start();

    bl_show_log();
    if (ret != 0) {
        LOG_E("flash init fail!!!\r\n");
    }
    bl_show_flashinfo();

    LOG_I("dynamic memory init success,heap size = %d Kbyte \r\n", ((size_t)&__HeapLimit - (size_t)&__HeapBase) / 1024);


#ifdef CONFIG_PSRAM
    if (uhs_psram_init() < 0) {
        while (1) {
        }
    }
#endif
    /* set CPU D0 boot XIP address and flash address */
    // Tzc_Sec_Set_CPU_Group(GLB_CORE_ID_D0, 1);
    // /* D0 boot from 0x58000000 */
    // GLB_Set_CPU_Reset_Address(GLB_CORE_ID_D0, 0x58000000);
    // /* D0 image offset on flash is 0x100000+0x1000(header) */
    // bflb_sf_ctrl_set_flash_image_offset(0x101000, 1, SF_CTRL_FLASH_BANK0);

    bflb_irq_restore(flag);

    /* we do not check header at 0x100000, just boot */
    GLB_Release_CPU(GLB_CORE_ID_D0);

    /* release d0 and then do can run */
    BL_WR_WORD(IPC_SYNC_ADDR1, IPC_SYNC_FLAG);
    BL_WR_WORD(IPC_SYNC_ADDR2, IPC_SYNC_FLAG);
    L1C_DCache_Clean_By_Addr(IPC_SYNC_ADDR1, 8);
}


