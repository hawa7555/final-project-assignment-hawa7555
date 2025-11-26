#include "led_control.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

static int led_fd = -1;

int led_init(const char *device_path)
{
    led_fd = open(device_path, O_RDWR);
    if (led_fd < 0)
    {
        perror("Failed to open LED device");
        return -1;
    }
    printf("LED device initialized: %s\n", device_path);
    return 0;
}

int led_set_status(int status)
{
    if (led_fd < 0)
    {
        printf("LED device not initialized\n");
        return -1;
    }
    
    if (ioctl(led_fd, LED_IOC_SET_STATUS, &status) < 0)
    {
        perror("ioctl LED_IOC_SET_STATUS failed");
        return -1;
    }
    
    return 0;
}

int led_get_status(void)
{
    int status;
    
    if (led_fd < 0)
    {
        printf("LED device not initialized\n");
        return -1;
    }
    
    if (ioctl(led_fd, LED_IOC_GET_STATUS, &status) < 0)
    {
        perror("ioctl LED_IOC_GET_STATUS failed");
        return -1;
    }
    
    return status;
}

void led_cleanup(void)
{
    if (led_fd >= 0)
    {
        close(led_fd);
        led_fd = -1;
        printf("LED device closed\n");
    }
}
