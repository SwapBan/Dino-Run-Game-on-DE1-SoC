#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN     8

// Memory-mapped I/O base & offsets (in bytes)
#define LW_BRIDGE_BASE 0xFF200000
#define DINO_Y_OFFSET  0x0004       // register 1: vertical position
#define DUCKING_OFFSET (13 * 4)     // register 13: duck flag
#define JUMPING_OFFSET (14 * 4)     // register 14: jump flag
#define MAP_SIZE       0x1000

// Physics constants (as in your original)
#define GROUND_Y       100          // example: ground position
#define GRAVITY        1            // example: gravity per frame

int main(void) {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open(/dev/mem)");
        return 1;
    }

    // Map lightweight bridge into user space
    void *lw_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (lw_base == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // Pointers to each register
    volatile uint32_t *dino_y_reg = (uint32_t *)(lw_base + DINO_Y_OFFSET);
    volatile uint32_t *duck_reg   = (uint32_t *)(lw_base + DUCKING_OFFSET);
    volatile uint32_t *jump_reg   = (uint32_t *)(lw_base + JUMPING_OFFSET);

    // --- USB gamepad setup (unchanged) ---
    libusb_device_handle *pad;
    libusb_init(NULL);
    pad = libusb_open_device_with_vid_pid(NULL, USB_VID, USB_PID);
    if (!pad) {
        fprintf(stderr, "Cannot open gamepad\n");
        munmap(lw_base, MAP_SIZE);
        close(fd);
        return 1;
    }
    libusb_claim_interface(pad, 0);

    // Game state
    int y = GROUND_Y;
    int v = 0;
    uint8_t report[REPORT_LEN];
    int transferred, r;

    // Main loop
    while (1) {
        r = libusb_interrupt_transfer(pad, /*endpoint=*/0x81,
                                      report, REPORT_LEN,
                                      &transferred, /*timeout=*/0);
        if (r < 0) {
            fprintf(stderr, "USB read error: %d\n", r);
            break;
        }

        // report[4] is Y-axis: 0x00=up, 0x80=center, 0xFF=down
        uint8_t y_axis = report[4];

        // Determine actions
        bool want_jump = (y_axis == 0x00 && y == GROUND_Y);
        bool want_duck = (y_axis == 0xFF && y == GROUND_Y);

        // Jump logic: only on press (sets initial velocity)
        if (want_jump) {
            v = -12;
        }

        // Write flags every frame
        *jump_reg = want_jump ? 1 : 0;
        *duck_reg = want_duck ? 1 : 0;

        // Physics update
        v += GRAVITY;
        y += v;
        if (y > GROUND_Y) {
            y = GROUND_Y;
            v = 0;
        }

        // Update the FPGA sprite position
        *dino_y_reg = (uint32_t)y;

        // ~200 Hz update
        usleep(5000);
    }

    // Teardown
    libusb_close(pad);
    libusb_exit(NULL);
    munmap(lw_base, MAP_SIZE);
    close(fd);
    return 0;
}
