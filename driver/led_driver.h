#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#define DEVICE_NAME "aitutor_led"
#define LED_DRIVER_IOC_MAGIC 'A'

// ioctl commands
#define LED_IOC_SET_STATUS    _IOW(LED_DRIVER_IOC_MAGIC, 1, int)
#define LED_IOC_GET_STATUS    _IOR(LED_DRIVER_IOC_MAGIC, 2, int)

#define LED_IOC_MAXNR 2

// LED status values
#define LED_STATUS_IDLE       0
#define LED_STATUS_LISTENING  1
#define LED_STATUS_THINKING   2
#define LED_STATUS_SPEAKING   3 

#endif
