#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN     8

// base of your lightweight bridge
#define LW_BRIDGE_BASE 0xFF200000 
#define MAP_SIZE       0x1000

// register indices (word-addresses) in your Avalon slave
#define REG_DINO_Y      1   //  4 bytes = offset 0x04
#define REG_CACTUS_X   15   // 15×4=0x3C
#define REG_PTERO_X    16   // 16×4=0x40 (if you ever wire it up)
#define REG_LAVA_X     17   // 17×4=0x44

#define SCREEN_WIDTH  640
#define GROUND_Y      120
#define GRAVITY         1
#define OBSTACLE_SPEED  1   // pixels per frame

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }

    // map the whole 0x1000-byte lightweight bridge
    void *base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE,
                      MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (base == MAP_FAILED) { perror("mmap"); return 1; }

    // treat it as an array of 32-bit registers
    volatile uint32_t *regs = (volatile uint32_t *)base;

    // joystick setup (exactly as before)
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred;
    pad = openkeyboard(&ep);
    if (!pad) {
        fprintf(stderr, "Controller not found\n");
        return 1;
    }

    // initialize dino + obstacles offscreen
    regs[REG_DINO_Y]    = GROUND_Y;
    regs[REG_CACTUS_X]  = SCREEN_WIDTH +  50;
    regs[REG_PTERO_X]   = SCREEN_WIDTH + 150;
    regs[REG_LAVA_X]    = SCREEN_WIDTH + 250;

    int y = GROUND_Y, v = 0;
    while (1) {
        // 1) read joystick & update jump physics
        if (libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0)==0
         && transferred==REPORT_LEN) 
        {
            if (report[4]==0x00 && y==GROUND_Y) {
                v = -12;
            }
            v += GRAVITY;
            y += v;
            if (y > GROUND_Y) { y = GROUND_Y; v = 0; }

            regs[REG_DINO_Y] = y;
        }

        // 2) slide each obstacle left by OBSTACLE_SPEED
        uint32_t cx = regs[REG_CACTUS_X];
        uint32_t px = regs[REG_PTERO_X];
        uint32_t lx = regs[REG_LAVA_X];

        regs[REG_CACTUS_X] = (cx > OBSTACLE_SPEED) ? cx - OBSTACLE_SPEED : SCREEN_WIDTH;
        regs[REG_PTERO_X]  = (px > OBSTACLE_SPEED) ? px - OBSTACLE_SPEED : SCREEN_WIDTH;
        regs[REG_LAVA_X]   = (lx > OBSTACLE_SPEED) ? lx - OBSTACLE_SPEED : SCREEN_WIDTH;

        // 3) simple frame pacing
        usleep(10000);  // ~100 FPS
    }

    // cleanup (never reached in this tight loop)
    libusb_close(pad);
    libusb_exit(NULL);
    munmap(base, MAP_SIZE);
    close(fd);
    return 0;
}
