#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN       8

// Lightweight HPS→FPGA bridge base and window size
#define LW_BRIDGE_BASE 0xFF200000
#define MAP_SIZE       0x1000

// Avalon‐slave register byte‐offsets (slot_index × 4)
#define DINO_Y_OFFSET    (0x0004)   // slot 1: dino_y
#define S_CAC_X_OFFSET   (6*4)      // slot 6: small cactus x
#define S_CAC_Y_OFFSET   (7*4)      // slot 7: small cactus y
#define CG_X_OFFSET      (15*4)     // slot 15: group‐cactus x
#define CG_Y_OFFSET      (16*4)     // slot 16: group‐cactus y
#define LAVA_X_OFFSET    (17*4)     // slot 17: lava x
#define LAVA_Y_OFFSET    (18*4)     // slot 18: lava y

#define GROUND_Y         120
#define GRAVITY            1
#define OBSTACLE_SPEED     1      // pixels per frame
#define SCREEN_WIDTH    640      // match your HACTIVE

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }

    void *lw = mmap(NULL, MAP_SIZE,
                    PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, LW_BRIDGE_BASE);
    if (lw == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    // Map our registers
    volatile uint32_t *dino_y = (uint32_t *)((char*)lw + DINO_Y_OFFSET);
    volatile uint32_t *sc_x   = (uint32_t *)((char*)lw + S_CAC_X_OFFSET);
    volatile uint32_t *sc_y   = (uint32_t *)((char*)lw + S_CAC_Y_OFFSET);
    volatile uint32_t *cg_x   = (uint32_t *)((char*)lw + CG_X_OFFSET);
    volatile uint32_t *cg_y   = (uint32_t *)((char*)lw + CG_Y_OFFSET);
    volatile uint32_t *lava_x = (uint32_t *)((char*)lw + LAVA_X_OFFSET);
    volatile uint32_t *lava_y = (uint32_t *)((char*)lw + LAVA_Y_OFFSET);

    // USB gamepad setup
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred;
    libusb_init(NULL);
    pad = openkeyboard(&ep);
    if (!pad) {
        fprintf(stderr, "Controller not found\n");
        return 1;
    }

    // Initialize positions
    *dino_y = GROUND_Y;
    *sc_x   = SCREEN_WIDTH +  50;  *sc_y   = GROUND_Y;
    *cg_x   = SCREEN_WIDTH + 150;  *cg_y   = GROUND_Y;
    *lava_x = SCREEN_WIDTH + 250;  *lava_y = GROUND_Y;

    // Game state
    int y = GROUND_Y, v = 0;

    while (1) {
        // 1) Read input & update dinosaur
        if (libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0) == 0
            && transferred == REPORT_LEN) {
            uint8_t y_axis = report[4];
            if (y_axis == 0x00 && y == GROUND_Y) {
                v = -12;
            }
            v += GRAVITY;
            y += v;
            if (y > GROUND_Y) { y = GROUND_Y; v = 0; }
            *dino_y = y;
        }

        // 2) Slide obstacles left, wrap at right edge
        uint32_t x_sc   = *sc_x;
        uint32_t x_cg   = *cg_x;
        uint32_t x_lava = *lava_x;

        x_sc   = (x_sc   > OBSTACLE_SPEED) ? x_sc   - OBSTACLE_SPEED : SCREEN_WIDTH;
        x_cg   = (x_cg   > OBSTACLE_SPEED) ? x_cg   - OBSTACLE_SPEED : SCREEN_WIDTH;
        x_lava = (x_lava > OBSTACLE_SPEED) ? x_lava - OBSTACLE_SPEED : SCREEN_WIDTH;

        *sc_x   = x_sc;
        *cg_x   = x_cg;
        *lava_x = x_lava;

        // 3) Frame rate control (~100 FPS)
        usleep(10000);
    }

    // never reached…
    munmap(lw, MAP_SIZE);
    close(fd);
    libusb_close(pad);
    libusb_exit(NULL);
    return 0;
}
