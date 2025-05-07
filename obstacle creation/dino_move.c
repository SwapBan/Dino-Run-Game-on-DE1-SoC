#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN     8

// Memory‐mapped I/O base and offsets (tweak to match your system)
#define LW_BRIDGE_BASE 0xFF200000
#define MAP_SIZE       0x1000

#define DINO_Y_OFFSET    0x0004
#define CACTUS_X_OFFSET  0x0008
#define PTERO_X_OFFSET   0x000C
#define LAVA_X_OFFSET    0x0010

#define GROUND_Y       120
#define GRAVITY          1
#define OBSTACLE_SPEED   1    // pixels per frame
#define SCREEN_WIDTH  640    // adjust to your visible HACTIVE

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }
    void *lw_base = mmap(NULL, MAP_SIZE,
                         PROT_READ|PROT_WRITE, MAP_SHARED,
                         fd, LW_BRIDGE_BASE);
    if (lw_base == MAP_FAILED) { perror("mmap"); return 1; }

    // Pointers into FPGA registers
    volatile uint32_t *dino_y_reg    = lw_base + DINO_Y_OFFSET;
    volatile uint32_t *cactus_x_reg  = lw_base + CACTUS_X_OFFSET;
    volatile uint32_t *ptero_x_reg   = lw_base + PTERO_X_OFFSET;
    volatile uint32_t *lava_x_reg    = lw_base + LAVA_X_OFFSET;

    // USB joystick setup (same as before)…
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred;
    pad = openkeyboard(&ep);
    if (!pad) {
        fprintf(stderr, "Controller not found\n");
        return 1;
    }

    // Game state
    int y = GROUND_Y, v = 0;
    // Initialize obstacles somewhere off-screen to the right:
    *cactus_x_reg = SCREEN_WIDTH +  50;
    *ptero_x_reg  = SCREEN_WIDTH + 150;
    *lava_x_reg   = SCREEN_WIDTH + 250;

    while (1) {
        // 1) handle joystick & physics
        if (libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0)==0
         && transferred==REPORT_LEN)
        {
            uint8_t y_axis = report[4];
            if (y_axis==0x00 && y==GROUND_Y) {
                v = -12;
            }
            v += GRAVITY;
            y += v;
            if (y>GROUND_Y) { y=GROUND_Y; v=0; }
            *dino_y_reg = y;
        }

        // 2) move each obstacle left by OBSTACLE_SPEED
        uint32_t cx = *cactus_x_reg;
        uint32_t px = *ptero_x_reg;
        uint32_t lx = *lava_x_reg;

        cx = (cx > OBSTACLE_SPEED) ? cx - OBSTACLE_SPEED : SCREEN_WIDTH;
        px = (px > OBSTACLE_SPEED) ? px - OBSTACLE_SPEED : SCREEN_WIDTH;
        lx = (lx > OBSTACLE_SPEED) ? lx - OBSTACLE_SPEED : SCREEN_WIDTH;

        *cactus_x_reg = cx;
        *ptero_x_reg  = px;
        *lava_x_reg   = lx;

        // 3) control frame‐rate / speed
        usleep(10000);  // ~100 FPS; adjust to taste
    }

    // cleanup…
    libusb_close(pad);
    libusb_exit(NULL);
    munmap(lw_base, MAP_SIZE);
    close(fd);
    return 0;
}
