#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN       8

// Memory‐mapped I/O base and offsets (tweak to match your system)
#define LW_BRIDGE_BASE   0xFF200000
#define MAP_SIZE         0x1000

#define DINO_Y_OFFSET      0x0004
#define CACTUS_X_OFFSET    0x0008
#define PTERO_X_OFFSET     0x000C
#define LAVA_X_OFFSET      0x0010

#define GROUND_Y         120
#define GRAVITY            1
#define OBSTACLE_SPEED     1    // pixels per frame

// **UPDATED** to match your Verilog HACTIVE=1280
#define SCREEN_WIDTH    1280    

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open(/dev/mem)");
        return 1;
    }

    void *lw = mmap(NULL, MAP_SIZE,
                    PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, LW_BRIDGE_BASE);
    if (lw == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // Pointers into FPGA registers
    volatile uint32_t *dino_y   = (uint32_t *)((char*)lw + DINO_Y_OFFSET);
    volatile uint32_t *cactus_x = (uint32_t *)((char*)lw + CACTUS_X_OFFSET);
    volatile uint32_t *ptero_x  = (uint32_t *)((char*)lw + PTERO_X_OFFSET);
    volatile uint32_t *lava_x   = (uint32_t *)((char*)lw + LAVA_X_OFFSET);

    // USB joystick setup
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred;
    pad = openkeyboard(&ep);
    if (!pad) {
        fprintf(stderr, "Controller not found\n");
        munmap(lw, MAP_SIZE);
        close(fd);
        return 1;
    }

    // Game state
    int y = GROUND_Y, v = 0;

    // Initialize obstacles off‐screen to the right
    *cactus_x = SCREEN_WIDTH +  50;
    *ptero_x  = SCREEN_WIDTH + 150;
    *lava_x   = SCREEN_WIDTH + 250;

    while (1) {
        // 1) handle joystick & dino physics
        if (libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0) == 0
         && transferred == REPORT_LEN)
        {
            uint8_t y_axis = report[4];
            if (y_axis == 0x00 && y == GROUND_Y) {
                v = -12;
            }
            v += GRAVITY;
            y += v;
            if (y > GROUND_Y) {
                y = GROUND_Y;
                v = 0;
            }
            *dino_y = y;
        }

        // 2) slide each obstacle left by OBSTACLE_SPEED
        uint32_t cx = *cactus_x;
        uint32_t px = *ptero_x;
        uint32_t lx = *lava_x;

        cx = (cx > OBSTACLE_SPEED) ? cx - OBSTACLE_SPEED : SCREEN_WIDTH;
        px = (px > OBSTACLE_SPEED) ? px - OBSTACLE_SPEED : SCREEN_WIDTH;
        lx = (lx > OBSTACLE_SPEED) ? lx - OBSTACLE_SPEED : SCREEN_WIDTH;

        *cactus_x = cx;
        *ptero_x  = px;
        *lava_x   = lx;

        // 3) throttle loop to ~100 FPS
        usleep(10000);
    }

    // cleanup
    libusb_close(pad);
    libusb_exit(NULL);
    munmap(lw, MAP_SIZE);
    close(fd);
    return 0;
}
