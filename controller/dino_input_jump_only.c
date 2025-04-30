
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <libusb-1.0/libusb.h>
#include <math.h>
#include <sys/mman.h>

#define JOY_VENDOR_ID 0x0810  // Vendor ID for DragonRise controller (example)
#define JOY_PRODUCT_ID 0xE501 // Product ID (replace with yours from lsusb if different)

#define LW_BRIDGE_BASE 0xFF200000
#define DINO_Y_OFFSET  0x1000
#define MAP_SIZE       0x10000

#define GROUND_Y 120
#define GRAVITY 1

int main() {
    // Memory map setup
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *lw_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, LW_BRIDGE_BASE);
    volatile uint32_t *dino_y_reg = (uint32_t *)((char *)lw_base + DINO_Y_OFFSET);

    // USB setup
    libusb_context *ctx = NULL;
    libusb_device_handle *handle = NULL;
    libusb_init(&ctx);
    handle = libusb_open_device_with_vid_pid(ctx, JOY_VENDOR_ID, JOY_PRODUCT_ID);
    if (!handle) {
        printf("Controller not found.\n");
        return 1;
    }

    if (libusb_kernel_driver_active(handle, 0)) {
        libusb_detach_kernel_driver(handle, 0);
    }
    libusb_claim_interface(handle, 0);

    unsigned char report[8];
    int transferred;
    int y = GROUND_Y;
    int v = 0;

    while (1) {
        int r = libusb_interrupt_transfer(handle, 0x81, report, sizeof(report), &transferred, 0);
        if (r == 0 && transferred == 8) {
            uint8_t y_axis = report[4];

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
        }
        usleep(5000); // ~200 FPS
    }

    libusb_release_interface(handle, 0);
    libusb_close(handle);
    libusb_exit(ctx);
    return 0;
}
