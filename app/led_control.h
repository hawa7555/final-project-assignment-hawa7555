#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include "../driver/led_driver.h"

int led_init(const char *device_path);
int led_set_status(int status);
int led_get_status(void);
void led_cleanup(void);

#endif
