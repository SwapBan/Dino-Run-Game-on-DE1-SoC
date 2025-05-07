#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>               // for bool
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

// Your gamepad’s Vendor ID and Product ID from lsusb
#ifndef USB_VID
#define USB_VID 0x0079            // DragonRise Inc.
#endif
#ifndef USB_PID
#define USB_PID 0x0011            // Gamepad
#endif

#define REPORT_LEN     8

// Lightweight bridge base & register offsets (bytes)
#define LW_BRIDGE_BASE 0xFF200000
#define MAP_SIZE       0x1000

#define DINO_Y_OFFSET    0x0004     // reg 1: vertical position
#define DUCKING_OFFSET  (13 * 4)    // reg 13: duck flag
#define JUMPING_OFFSET  (14 * 4)    // reg 14: jump flag

// Physics
#define GROUND_Y        100         // adjust if needed
#define GRAVITY         1

int main(void) {
    // Open /dev/mem for MMIO
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open(/dev/mem)");
        return 1;
    }

    // Map the lightweight bridge
    void *lw_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE,
                         MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (lw_base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // Create pointers to each register
    volatile uint32_t *dino_y_reg = (uint32_t *)(lw_base + DINO_Y_OFFSET);
    volatile uint32_t *duck_reg   = (uint32_t *)(lw_base + DUCKING_OFFSET);
    volatile uint32_t *jump_reg   = (uint32_t *)(lw_base + JUMPING_OFFSET);

    // Initialize libusb and open the pad
    libusb_context *ctx = NULL;
    libusb_device_handle *pad;
    if (libusb_init(&ctx) < 0) {
        fprintf(stderr, "libusb_init failed\n");
        return 1;
    }
    pad = libusb_open_device_with_vid_pid(ctx, USB_VID, USB_PID);
    if (!pad) {
        fprintf(stderr, "Cannot open gamepad (VID=0x%04X, PID=0x%04X)\n",
                USB_VID, USB_PID);
        libusb_exit(ctx);
        return 1;
    }
    libusb_claim_interface(pad, 0);

    // Game state
    int y = GROUND_Y, v = 0;
    uint8_t report[REPORT_LEN];
    int transferred, r;

    // Main game loop
    while (1) {
        r = libusb_interrupt_transfer(pad, /*endpoint=*/0x81,
                                      report, REPORT_LEN,
                                      &transferred, /*timeout=*/0);
        if (r < 0) {
            fprintf(stderr, "USB read error: %d\n", r);
            break;
        }

        // Y-axis: 0x00 = up, 0x80 = center, 0xFF = down
        uint8_t y_axis = report[4];
        bool want_jump = (y_axis == 0x00 && y == GROUND_Y);
        bool want_duck = (y_axis == 0xFF && y == GROUND_Y);

        // Only trigger jump on press
        if (want_jump) {
            v = -12;
        }

        // Drive the FPGA flags
        *jump_reg = want_jump ? 1 : 0;
        *duck_reg = want_duck ? 1 : 0;

        // Simple gravity physics
        v += GRAVITY;
        y += v;
        if (y > GROUND_Y) {
            y = GROUND_Y;
            v = 0;
        }

        // Update sprite Y on FPGA
        *dino_y_reg = (uint32_t)y;

        // ~200 Hz refresh
        usleep(5000);
    }

    // Cleanup
    libusb_close(pad);
    libusb_exit(ctx);
    munmap(lw_base, MAP_SIZE);
    close(fd);
    return 0;
}
