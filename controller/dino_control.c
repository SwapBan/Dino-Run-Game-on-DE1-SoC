
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/mman.h>
#include "usbkeyboard.h"
#include <libusb-1.0/libusb.h>

#define LW_BRIDGE_BASE 0xFF200000
#define DINO_Y_OFFSET  0x0004
#define MAP_SIZE       0x1000

#define GROUND_Y 120
#define GRAVITY 1

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *lw_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, LW_BRIDGE_BASE);
    volatile uint32_t *dino_y_reg = (uint32_t *)((char *)lw_base + DINO_Y_OFFSET);

    struct usb_keyboard_packet packet;
    libusb_context *ctx;
    struct libusb_device_handle *keyboard;

    libusb_init(&ctx);
    if (usb_init_keyboard(&keyboard) == 0) {
        printf("Found controller. Listening for input...\n");
    } else {
        printf("Controller not found.\n");
        return 1;
    }

    int y = GROUND_Y;
    int v = 0;

    while (1) {
        if (usb_read_keyboard(&keyboard, &packet) == 0) {
            uint8_t y_axis = packet.report[4];

            if (y_axis == 0x00 && y == GROUND_Y) {
                v = -12; // Jump impulse
            }

            // Apply gravity and update y
            v += GRAVITY;
            y += v;
            if (y > GROUND_Y) {
                y = GROUND_Y;
                v = 0;
            }

            *dino_y_reg = y;
            usleep(5000);
        }
    }

    libusb_close(keyboard);
    libusb_exit(ctx);
    return 0;
}
