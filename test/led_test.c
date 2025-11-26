#include <stdio.h>
#include <unistd.h>
#include "../app/led_control.h"

int main(void)
{
    if (led_init("/dev/aitutor_led") < 0)
    {
        return 1;
    }

    printf("Test 1: IDLE\n");
    led_set_status(LED_STATUS_IDLE);
    sleep(2);

    printf("Test 2: LISTENING\n");
    led_set_status(LED_STATUS_LISTENING);
    sleep(2);

    printf("Test 3: THINKING\n");
    led_set_status(LED_STATUS_THINKING);
    sleep(2);

    printf("Test 4: SPEAKING\n");
    led_set_status(LED_STATUS_SPEAKING);
    sleep(2);

    printf("Test 5: Get status\n");
    int status = led_get_status();
    printf("Current status: %d\n", status);

    printf("Test 6: Back to IDLE\n");
    led_set_status(LED_STATUS_IDLE);

    led_cleanup();
    printf("\n=== Test Complete ===\n");
    return 0;
}
