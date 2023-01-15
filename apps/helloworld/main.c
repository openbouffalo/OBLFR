#include "bflb_mtimer.h"
#include "board.h"
#include "log.h"
#include "sdkconfig.h"

int main(void)
{
    board_init();
    while (1) {
        LOG_F("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_E("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_W("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_I("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_D("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        LOG_T("%s\r\n", CONFIG_EXAMPLE_HELLOWORLD);
        bflb_mtimer_delay_ms(CONFIG_EXAMPLE_INTERVAL);
    }
}
