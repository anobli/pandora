

#include <zephyr/kernel.h>
#include <esphome/components/action.h>

void esphome_delay(uint64_t delay_us)
{
        k_sleep(K_USEC(delay_us));
}
