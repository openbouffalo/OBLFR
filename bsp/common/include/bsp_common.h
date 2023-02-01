#ifndef BSP_COMMON_H
#define BSP_COMMON_H

#include "sdkconfig.h"

typedef struct  {
    uint32_t pin;
    uint32_t mode;
    uint32_t pull;
    uint32_t drive;
    uint32_t func;
} pinmux_setup_t;


void bl_show_log(void);
void bl_show_flashinfo(void);
void board_common_setup_pinmux(pinmux_setup_t *pinmux_setup, uint32_t len);

#if defined(CONFIG_JTAG_DEBUG)
#ifdef CONFIG_JTAG_DEBUG_PINS_GRP0
#define JTAG_TMS_GPIOPIN GPIO_PIN_6
#define JTAG_TDO_GPIOPIN GPIO_PIN_7
#define JTAG_TCK_GPIOPIN GPIO_PIN_12
#define JTAG_TDI_GPIOPIN GPIO_PIN_13
#elif CONFIG_JTAG_DEBUG_PINS_GRP1
#define JTAG_TMS_GPIOPIN GPIO_PIN_0
#define JTAG_TDO_GPIOPIN GPIO_PIN_1
#define JTAG_TCK_GPIOPIN GPIO_PIN_2
#define JTAG_TDI_GPIOPIN GPIO_PIN_3
#else
#error Unknown JTAG debug pins
#endif

#ifdef CONFIG_BL808
#ifdef CONFIG_JTAG_DEBUG_M0
#define JTAG_GPIO_FUNC GPIO_FUNC_JTAG_M0
#elif CONFIG_JTAG_DEBUG_D0
#define JTAG_GPIO_FUNC GPIO_FUNC_JTAG_D0
#elif CONFIG_JTAG_DEBUG_LP
#define JTAG_GPIO_FUNC GPIO_FUNC_JTAG_LP
#else
#error "Unknown CPU Selected for JTAG"
#endif
#elif defined(CONFIG_BL616)
#define JTAG_GPIO_FUNC GPIO_FUN_JTAG
#endif

#endif


#ifdef CONFIG_PINMUX_ENABLE_SDH 
#define PINMUX_ENABLE_SDH()                     \
    {                                           \
        .pin = GPIO_PIN_0,                      \
        .mode = GPIO_ALTERNATE | GPIO_SMT_EN,   \
        .pull = GPIO_PULLUP,                    \
        .drive = GPIO_DRV_2,                    \
        .func = GPIO_FUNC_SDH,                  \
    },                                          \
    {                                           \
        .pin = GPIO_PIN_1,                      \
        .mode = GPIO_ALTERNATE | GPIO_SMT_EN,   \
        .pull = GPIO_PULLUP,                    \
        .drive = GPIO_DRV_2,                    \
        .func = GPIO_FUNC_SDH,                  \
    },                                          \
    {                                           \
        .pin = GPIO_PIN_2,                      \
        .mode = GPIO_ALTERNATE | GPIO_SMT_EN,   \
        .pull = GPIO_PULLUP,                    \
        .drive = GPIO_DRV_2,                    \
        .func = GPIO_FUNC_SDH,                  \
    },                                          \
    {                                           \
        .pin = GPIO_PIN_3,                      \
        .mode = GPIO_ALTERNATE | GPIO_SMT_EN,   \
        .pull = GPIO_PULLUP,                    \
        .drive = GPIO_DRV_2,                    \
        .func = GPIO_FUNC_SDH,                  \
    },                                          \
    {                                           \
        .pin = GPIO_PIN_4,                      \
        .mode = GPIO_ALTERNATE | GPIO_SMT_EN,   \
        .pull = GPIO_PULLUP,                    \
        .drive = GPIO_DRV_2,                    \
        .func = GPIO_FUNC_SDH,                  \
    },                                          \
    {                                           \
        .pin = GPIO_PIN_5,                      \
        .mode = GPIO_ALTERNATE | GPIO_SMT_EN,   \
        .pull = GPIO_PULLUP,                    \
        .drive = GPIO_DRV_2,                    \
        .func = GPIO_FUNC_SDH,                  \
    },              
#else
#define PINMUX_ENABLE_SDH()
#endif

#ifdef CONFIG_JTAG_DEBUG
#define PINMUX_ENABLE_JTAG()                    \
    {                                           \
        .pin = JTAG_TMS_GPIOPIN,                \
        .mode = GPIO_ALTERNATE,                 \
        .func = JTAG_GPIO_FUNC,                 \
    },                                          \
    {                                           \
        .pin = JTAG_TDO_GPIOPIN,                \
        .mode = GPIO_ALTERNATE,                 \
        .func = JTAG_GPIO_FUNC,                 \
    },                                          \
    {                                           \
        .pin = JTAG_TCK_GPIOPIN,                \
        .mode = GPIO_ALTERNATE,                 \
        .func = JTAG_GPIO_FUNC,                 \
    },                                          \
    {                                           \
        .pin = JTAG_TDI_GPIOPIN,                \
        .mode = GPIO_ALTERNATE,                 \
        .func = JTAG_GPIO_FUNC,                 \
    },
#else
#define PINMUX_ENABLE_JTAG()
#endif

#define COMMON_PINMUX_SETUP()                   \
        PINMUX_ENABLE_SDH()                     \
        PINMUX_ENABLE_JTAG()


#endif