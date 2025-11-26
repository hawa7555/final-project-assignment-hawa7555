#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include "led_driver.h"

#define DEVICE_NAME "aitutor_led"

static int led_gpio = 23;
module_param(led_gpio, int, 0444);
MODULE_PARM_DESC(led_gpio, "Green LED GPIO");

static int red_led_gpio = 24;    // Red LED - GPIO 24
module_param(red_led_gpio, int, 0444);
MODULE_PARM_DESC(red_led_gpio, "Red LED GPIO");

static int gpio_base = 512;
module_param(gpio_base, int, 0444);
MODULE_PARM_DESC(gpio_base, "Global GPIO base");

static int start_button_gpio = 17;    // START button - GPIO 17
module_param(start_button_gpio, int, 0444);
MODULE_PARM_DESC(start_button_gpio, "START button GPIO");

static int cancel_button_gpio = 27;   // CANCEL button - GPIO 27
module_param(cancel_button_gpio, int, 0444);
MODULE_PARM_DESC(cancel_button_gpio, "CANCEL button GPIO");

static int global_gpio;          // Green: gpio_base + led_gpio
static int global_gpio_red;      // Red: gpio_base + red_led_gpio

static int global_gpio_start_btn;    // START: gpio_base + start_button_gpio
static int global_gpio_cancel_btn;   // CANCEL: gpio_base + cancel_button_gpio

static int led_state = 0;
static int red_led_state = 0;

static dev_t dev_num;
static struct cdev aitutor_cdev;

/* ---------- file operations ---------- */

static int led_open(struct inode *inode, struct file *file)
{
    pr_info("%s: device opened\n", DEVICE_NAME);
    return 0;
}

static int led_release(struct inode *inode, struct file *file)
{
    pr_info("%s: device closed\n", DEVICE_NAME);
    return 0;
}

static ssize_t led_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
    char tmp[3];
    int start_val, cancel_val;

    //if position not at start, return EOF
    if (*ppos != 0)
        return 0;

    // Read button states
    if (!gpio_is_valid(global_gpio_start_btn) || !gpio_is_valid(global_gpio_cancel_btn))
    {
        pr_err("%s: invalid button GPIOs\n", DEVICE_NAME);
        return -ENODEV;
    }

    start_val =  gpio_get_value(global_gpio_start_btn);
    cancel_val = gpio_get_value(global_gpio_cancel_btn);

    tmp[0] = start_val ? '1' : '0';
    tmp[1] = cancel_val ? '1' : '0';
    tmp[2] = '\n';

    if (count < 3)
        return -EINVAL;

    //copy tmp data to buf
    if (copy_to_user(buf, tmp, 3))
        return -EFAULT;

    *ppos += 3;
    return 3;
}

static ssize_t led_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    char ch;

    if (count == 0)
        return 0;

    if (copy_from_user(&ch, buf, 1))
        return -EFAULT;

    if (!gpio_is_valid(global_gpio)) 
    {
        pr_err("%s: Invalid LED GPIOs\n", DEVICE_NAME);
        return -ENODEV;
    }

    if (ch == '0')
    {
        gpio_set_value(global_gpio, 0);
        led_state = 0;
        pr_info("LED OFF\n");
    }
    else if (ch == '1')
    {
        gpio_set_value(global_gpio, 1);
        led_state = 1;
        pr_info("LED ON\n");
    }

    return count;
}

static long led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int status;
    int ret = 0;

    // Check ioctl type and command number
    if (_IOC_TYPE(cmd) != LED_DRIVER_IOC_MAGIC)
    {
        pr_warn("%s: invalid ioctl magic\n", DEVICE_NAME);
        return -ENOTTY;
    }
    if (_IOC_NR(cmd) > LED_IOC_MAXNR)
    {
        pr_warn("%s: invalid ioctl number\n", DEVICE_NAME);
        return -ENOTTY;
    }

    switch (cmd)
    {
        case LED_IOC_SET_STATUS:
        // Get status value from user
        if (copy_from_user(&status, (int __user *) arg, sizeof(status)))
        {
            return -EFAULT;
        }

        // Validate status
        if (status < LED_STATUS_IDLE || status > LED_STATUS_SPEAKING)
        {
            pr_err("Invalid status\n");
            return -EINVAL;
        }

        // Set LED based on status
        if (!gpio_is_valid(global_gpio))
        {
            pr_err("Invalid gpio\n");
            return -ENODEV;
        }

        switch (status)
        {
            case LED_STATUS_IDLE:
                gpio_set_value(global_gpio, 0);      // Red OFF (GPIO 23)
                gpio_set_value(global_gpio_red, 0);  // Green OFF (GPIO 24)
                led_state = 0;
                red_led_state = 0;
                pr_info("Status IDLE - All LEDs OFF\n");
                break;

            case LED_STATUS_LISTENING:
                gpio_set_value(global_gpio, 1);      // Red ON
                gpio_set_value(global_gpio_red, 0);  // Green OFF
                led_state = 1;
                red_led_state = 0;
                pr_info("Status Listening - Red LED on\n");
                break;

            case LED_STATUS_THINKING:
                gpio_set_value(global_gpio, 0);      // Red OFF
                gpio_set_value(global_gpio_red, 1);  // Green ON
                led_state = 0;
                red_led_state = 1;
                pr_info("Status Thinking - Green LED on\n");
                break;

            case LED_STATUS_SPEAKING:
                gpio_set_value(global_gpio, 1);      // Red ON
                gpio_set_value(global_gpio_red, 0);  // Green OFF
                led_state = 1;
                red_led_state = 0;
                pr_info("Status Speaking - Red LED on\n");
                break;
        }
        break;

        case LED_IOC_GET_STATUS:
            // Return current LED state
            status = led_state;
            if (copy_to_user((int __user *)arg, &status, sizeof(status)))
            {
                return -EFAULT;
            }
            pr_info("Status Read: %d\n", status);
            break;

        default:
            return -ENOTTY;
        }

    return ret;
}

static const struct file_operations led_fops =
{
    .owner          = THIS_MODULE,
    .open           = led_open,
    .release        = led_release,
    .read           = led_read,
    .write          = led_write,
    .unlocked_ioctl = led_ioctl,
};

static int aitutor_led_init(void)
{
    int ret;

    pr_info("%s: Initializing driver\n", DEVICE_NAME);

    global_gpio = gpio_base + led_gpio;
    global_gpio_red = gpio_base + red_led_gpio;

    // Allocate char device numbers
    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret) 
    {
        pr_err("%s: alloc_chrdev_region failed: %d\n", DEVICE_NAME, ret);
        return ret;
    }

    pr_info("%s: Allocated major number %d\n", DEVICE_NAME, MAJOR(dev_num));

    // Initialize and add cdev
    cdev_init(&aitutor_cdev, &led_fops);
    aitutor_cdev.owner = THIS_MODULE;

    ret = cdev_add(&aitutor_cdev, dev_num, 1);
    if (ret)
    {
        pr_err("cdev_add failed \n");
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    // Setup Green LED GPIO
    if (!gpio_is_valid(global_gpio))
    {
        pr_err("Invalid green GPIO\n");
        ret = -ENODEV;
        goto err_gpio;
    }

    ret = gpio_request(global_gpio, DEVICE_NAME "_green");
    if (ret) 
    {
        pr_err("gpio_request green failed\n");
        goto err_gpio;
    }

    ret = gpio_direction_output(global_gpio, 0);
    if (ret) 
    {
        pr_err("gpio_direction_output green failed");
        gpio_free(global_gpio);
        goto err_gpio;
    }

    // Setup Red LED GPIO
    if (!gpio_is_valid(global_gpio_red)) 
    {
        pr_err("Invalid RED GPIO\n");
        ret = -ENODEV;
        gpio_free(global_gpio);
        goto err_gpio;
    }

    ret = gpio_request(global_gpio_red, DEVICE_NAME "_red");
    if (ret) 
    {
        pr_err("gpio_request RED failed\n");
        gpio_free(global_gpio);
        goto err_gpio;
    }

    ret = gpio_direction_output(global_gpio_red, 0);
    if (ret) 
    {
        pr_err("gpio_direction_output RED failed");
        gpio_free(global_gpio_red);
        gpio_free(global_gpio);
        goto err_gpio;
    }

    led_state = 0;
    red_led_state = 0;

    pr_info("%s: Green GPIO %d and Red GPIO %d configured, both OFF\n",
            DEVICE_NAME, global_gpio, global_gpio_red);


    // Setup START Button GPIO
    global_gpio_start_btn = gpio_base + start_button_gpio;

    if (!gpio_is_valid(global_gpio_start_btn)) 
    {
        pr_err("Invalid Start button GPIO\n");
        ret = -ENODEV;
        gpio_free(global_gpio_red);
        gpio_free(global_gpio);
        goto err_gpio;
    }

    ret = gpio_request(global_gpio_start_btn, DEVICE_NAME "_start_btn");
    if (ret) 
    {
        pr_err("gpio_request Start button failed\n");
        gpio_free(global_gpio_red);
        gpio_free(global_gpio);
        goto err_gpio;
    }

    ret = gpio_direction_input(global_gpio_start_btn);
    if (ret) 
    {
        pr_err("gpio_direction_output Start button failed");
        gpio_free(global_gpio_start_btn);
        gpio_free(global_gpio_red);
        gpio_free(global_gpio);
        goto err_gpio;
    }

    // Setup CANCEL Button GPIO
    global_gpio_cancel_btn = gpio_base + cancel_button_gpio;

    if (!gpio_is_valid(global_gpio_cancel_btn)) 
    {
        pr_err("Invalid Cancel button GPIO\n");
        ret = -ENODEV;
        gpio_free(global_gpio_start_btn);
        gpio_free(global_gpio_red);
        gpio_free(global_gpio);
        goto err_gpio;
    }

    ret = gpio_request(global_gpio_cancel_btn, DEVICE_NAME "_cancel_btn");
    if (ret) 
    {
        pr_err("gpio_request Cancel button failed\n");
        gpio_free(global_gpio_start_btn);
        gpio_free(global_gpio_red);
        gpio_free(global_gpio);
        goto err_gpio;
    }

    ret = gpio_direction_input(global_gpio_cancel_btn);
    if (ret) 
    {
        pr_err("gpio_direction_output Cancel button failed");
        gpio_free(global_gpio_cancel_btn);
        gpio_free(global_gpio_start_btn);
        gpio_free(global_gpio_red);
        gpio_free(global_gpio);
        goto err_gpio;
    }

    pr_info("%s: Buttons configured - START: GPIO %d, CANCEL: GPIO %d\n",
            DEVICE_NAME, global_gpio_start_btn, global_gpio_cancel_btn);

    return 0;

err_gpio:
    cdev_del(&aitutor_cdev);
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

static void aitutor_led_exit(void)
{
    //Turn off both LEDs and free GPIOs
    if (gpio_is_valid(global_gpio))
    {
        gpio_set_value(global_gpio, 0);
        gpio_free(global_gpio);
    }

    if (gpio_is_valid(global_gpio_red))
    {
        gpio_set_value(global_gpio_red, 0);
        gpio_free(global_gpio_red);
    }

    if (gpio_is_valid(global_gpio_start_btn))
    {
        gpio_free(global_gpio_start_btn);
    }

    if (gpio_is_valid(global_gpio_cancel_btn))
    {
        gpio_free(global_gpio_cancel_btn);
    }

    // Remove char device
    cdev_del(&aitutor_cdev);
    unregister_chrdev_region(dev_num, 1);

    pr_info("%s: Driver unloaded\n", DEVICE_NAME);
}

module_init(aitutor_led_init);
module_exit(aitutor_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Harshal Wadhwa");
MODULE_DESCRIPTION("Simple LED driver for Raspberry Pi");
MODULE_VERSION("1.1");

//Resources: https://embetronicx.com/tutorials/linux/device-drivers/gpio-linux-device-driver-using-raspberry-pi/

