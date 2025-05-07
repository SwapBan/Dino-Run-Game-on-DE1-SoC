#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN       8

// Lightweight HPS→FPGA bridge base & window
#define LW_BRIDGE_BASE 0xFF200000
#define MAP_SIZE       0x1000

// Avalon‐slave regs (word index × 4)
#define DINO_Y_OFFSET   (1*4)
#define S_CAC_X_OFFSET  (6*4)
#define S_CAC_Y_OFFSET  (7*4)
#define CG_X_OFFSET    (15*4)
#define CG_Y_OFFSET    (16*4)
#define LAVA_X_OFFSET  (17*4)
#define LAVA_Y_OFFSET  (18*4)

#define GROUND_Y       120
#define GRAVITY          1
#define OBSTACLE_SPEED   1
#define SCREEN_WIDTH   640

int main() {
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) { perror("open"); return 1; }

    void *lw = mmap(NULL, MAP_SIZE,
                    PROT_READ|PROT_WRITE, MAP_SHARED,
                    fd, LW_BRIDGE_BASE);
    if (lw == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    // FPGA registers
    volatile uint32_t *dino_y =  (uint32_t *)( (char*)lw + DINO_Y_OFFSET );
    volatile uint32_t *sc_x   =  (uint32_t *)( (char*)lw + S_CAC_X_OFFSET );
    volatile uint32_t *sc_y   =  (uint32_t *)( (char*)lw + S_CAC_Y_OFFSET );
    volatile uint32_t *cg_x   =  (uint32_t *)( (char*)lw + CG_X_OFFSET );
    volatile uint32_t *cg_y   =  (uint32_t *)( (char*)lw + CG_Y_OFFSET );
    volatile uint32_t *lava_x =  (uint32_t *)( (char*)lw + LAVA_X_OFFSET );
    volatile uint32_t *lava_y =  (uint32_t *)( (char*)lw + LAVA_Y_OFFSET );

    // USB gamepad
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred, r;
    libusb_init(NULL);
    pad = openkeyboard(&ep);
    if (!pad) { fprintf(stderr,"Controller not found\n"); return 1; }

    // init positions
    *dino_y = GROUND_Y;
    *sc_x   = SCREEN_WIDTH +  50;  *sc_y   = GROUND_Y;
    *cg_x   = SCREEN_WIDTH + 150;  *cg_y   = GROUND_Y;
    *lava_x = SCREEN_WIDTH + 250;  *lava_y = GROUND_Y;

    int y = GROUND_Y, v = 0;

    while (1) {
        // 1) try to read a report (1ms timeout)
        r = libusb_interrupt_transfer(pad, ep, report, REPORT_LEN,
                                      &transferred, /*timeout=*/1);
        if (r == 0 && transferred == REPORT_LEN) {
            uint8_t ya = report[4];
            if (ya == 0x00 && y == GROUND_Y) v = -12;
            v += GRAVITY;
            y += v;
            if (y > GROUND_Y) { y = GROUND_Y; v = 0; }
            *dino_y = y;
        }
        // if r == LIBUSB_ERROR_TIMEOUT, just skip input this frame

        // 2) slide all obstacles left, wrap at right edge
        uint32_t xs = *sc_x,   xc = *cg_x,   xl = *lava_x;
        xs = xs > OBSTACLE_SPEED ? xs - OBSTACLE_SPEED : SCREEN_WIDTH;
        xc = xc > OBSTACLE_SPEED ? xc - OBSTACLE_SPEED : SCREEN_WIDTH;
        xl = xl > OBSTACLE_SPEED ? xl - OBSTACLE_SPEED : SCREEN_WIDTH;
        *sc_x   = xs;
        *cg_x   = xc;
        *lava_x = xl;

        // 3) throttle to ~100 FPS
        usleep(10000);
    }

    // never reached…
    munmap(lw, MAP_SIZE);
    close(fd);
    libusb_close(pad);
    libusb_exit(NULL);
    return 0;
}
