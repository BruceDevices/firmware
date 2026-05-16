/**
 * @file      linux_gpio.cpp
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xinyuan Electronic Technology Co., Ltd
 * @date      2023-11-20
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

int linux_gpio_export(int pin);
int linux_gpio_unexport(int pin);
int linux_gpio_direction(int pin, int dir);
int linux_gpio_write(int pin, int value);
int linux_gpio_read(int pin);
int linux_gpio_edge(int pin, int edge);

int linux_gpio_export(int pin)
{
    char buffer[64];
    int len;
    int fd;
    fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd < 0) {
        printf("Failed to open export for writing!\n");
        return (-1);
    }
    len = snprintf(buffer, sizeof(buffer), "%d", pin);
    printf("%s,%d,%d\n", buffer, sizeof(buffer), len);
    if (write(fd, buffer, len) < 0) {
        printf("Failed to export gpio!");
        return -1;
    }
    close(fd);
    return 0;
}

int linux_gpio_unexport(int pin)
{
    char buffer[64];
    int len;
    int fd;
    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd < 0) {
        printf("Failed to open unexport for writing!\n");
        return -1;
    }
    len = snprintf(buffer, sizeof(buffer), "%d", pin);
    if (write(fd, buffer, len) < 0) {
        printf("Failed to unexport gpio!");
        return -1;
    }
    close(fd);
    return 0;
}

// dir: 0-->IN, 1-->OUT
int linux_gpio_direction(int pin, int dir)
{
    const char dir_str[] = "in\0out";
    char path[64];
    int fd;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/direction", pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("Failed to open gpio direction for writing!\n");
        return -1;
    }
    if (write(fd, &dir_str[dir == 0 ? 0 : 3], dir == 0 ? 2 : 3) < 0) {
        printf("Failed to set direction!\n");
        return -1;
    }
    close(fd);
    return 0;
}

// value: 0-->LOW, 1-->HIGH
int linux_gpio_write(int pin, int value)
{
    const char values_str[] = "01";
    char path[64];
    int fd;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("Failed to open gpio value for writing!\n");
        return -1;
    }
    if (write(fd, &values_str[value == 0 ? 0 : 1], 1) < 0) {
        printf("Failed to write value!\n");
        return -1;
    }
    close(fd);
    return 0;
}

int linux_gpio_read(int pin)
{
    char path[64];
    char value_str[3];
    int fd;
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("Failed to open gpio value for reading!\n");
        return -1;
    }
    if (read(fd, value_str, 3) < 0) {
        printf("Failed to read value!\n");
        return -1;
    }
    close(fd);
    return (atoi(value_str));
}

// 0-->none, 1-->rising, 2-->falling, 3-->both
int linux_gpio_edge(int pin, int edge)
{
    const char dir_str[] = "none\0rising\0falling\0both";
    unsigned char ptr = 0;
    char path[64];
    int fd;
    switch (edge) {
    case 0:
        ptr = 0;
        break;
    case 1:
        ptr = 5;
        break;
    case 2:
        ptr = 12;
        break;
    case 3:
        ptr = 20;
        break;
    default:
        ptr = 0;
    }
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/edge", pin);
    fd = open(path, O_WRONLY);
    if (fd < 0) {
        printf("Failed to open gpio edge for writing!\n");
        return -1;
    }
    if (write(fd, &dir_str[ptr], strlen(&dir_str[ptr])) < 0) {
        printf("Failed to set edge!\n");
        return -1;
    }
    close(fd);
    return 0;
}


