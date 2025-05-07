#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN      8

// HPS→FPGA “lightweight” bridge base & window
#define LW_BRIDGE_BASE  0xFF200000
#define MAP_SIZE        0x1000

// Avalon-slave registers (word-index ×4 bytes)
#define DINO_Y_OFFSET   (1*4)
#define SC_X_OFFSET     (6*4)
#define SC_Y_OFFSET     (7*4)
#define CG_X_OFFSET    (15*4)
#define CG_Y_OFFSET    (16*4)
#define LAVA_X_OFFSET  (17*4)
#define LAVA_Y_OFFSET  (18*4)

#define GROUND_Y        248     // matches your Verilog reset value for dino_y
#define GRAVITY           1
#define OBSTACLE_SPEED    1     // pixels per frame
#define SCREEN_WIDTH   1280     // match your HACTIVE in vga_counters

int main(void) {
    // 1) mmap the LW bridge
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open /dev/mem"); return 1; }
    void *lw = mmap(NULL, MAP_SIZE,
                    PROT_READ|PROT_WRITE, MAP_SHARED,
                    fd, LW_BRIDGE_BASE);
    if (lw == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // 2) get pointers into your regs
    volatile uint32_t *dino_y  = (uint32_t *)((char*)lw + DINO_Y_OFFSET);
    volatile uint32_t *sc_x    = (uint32_t *)((char*)lw + SC_X_OFFSET);
    volatile uint32_t *sc_y    = (uint32_t *)((char*)lw + SC_Y_OFFSET);
    volatile uint32_t *cg_x    = (uint32_t *)((char*)lw + CG_X_OFFSET);
    volatile uint32_t *cg_y    = (uint32_t *)((char*)lw + CG_Y_OFFSET);
    volatile uint32_t *lava_x  = (uint32_t *)((char*)lw + LAVA_X_OFFSET);
    volatile uint32_t *lava_y  = (uint32_t *)((char*)lw + LAVA_Y_OFFSET);

    // 3) set up libusb & claim interface
    libusb_context *ctx;
    libusb_device_handle *pad;
    uint8_t ep;
    libusb_init(&ctx);
    pad = openkeyboard(&ep);
    if (!pad) {
        fprintf(stderr, "Gamepad not found\n");
        return 1;
    }
    libusb_claim_interface(pad, 0);

    // 4) initialize positions
    *dino_y = GROUND_Y;
    *sc_x   = SCREEN_WIDTH +  50;  *sc_y   = GROUND_Y;
    *cg_x   = SCREEN_WIDTH + 150;  *cg_y   = GROUND_Y;
    *lava_x = SCREEN_WIDTH + 250;  *lava_y = GROUND_Y;

    int y = GROUND_Y, v = 0;
    unsigned char report[REPORT_LEN];
    int transferred, r;
    unsigned long frame = 0;

    // 5) game loop
    while (1) {
        // --- read joystick with 1 ms timeout ---
        r = libusb_interrupt_transfer(pad, ep,
                                      report, REPORT_LEN,
                                      &transferred,
                                      /*timeout=*/1);
        if (r == 0 && transferred == REPORT_LEN) {
            uint8_t ya = report[4];
            if (ya == 0x00 && y == GROUND_Y) v = -12;
            v += GRAVITY;
            y += v;
            if (y > GROUND_Y) { y = GROUND_Y; v = 0; }
            *dino_y = y;
        }
        // if (r == LIBUSB_ERROR_TIMEOUT) we just skip input this frame

        // --- slide obstacles left every frame ---
        uint32_t sx = *sc_x, cx = *cg_x, lx = *lava_x;
        sx = (sx > OBSTACLE_SPEED) ? sx - OBSTACLE_SPEED : SCREEN_WIDTH + 50;
        cx = (cx > OBSTACLE_SPEED) ? cx - OBSTACLE_SPEED : SCREEN_WIDTH + 150;
        lx = (lx > OBSTACLE_SPEED) ? lx - OBSTACLE_SPEED : SCREEN_WIDTH + 250;
        *sc_x   = sx;
        *cg_x   = cx;
        *lava_x = lx;

        // --- debug print every 100 frames ---
        if (++frame % 100 == 0) {
            printf("small_cactus=%u  group_cactus=%u  lava=%u\n",
                   sx, cx, lx);
            fflush(stdout);
        }

        // --- throttle to ~100 Hz ---
        usleep(10000);
    }

    // never reached…
    munmap(lw, MAP_SIZE);
    close(fd);
    libusb_close(pad);
    libusb_exit(ctx);
    return 0;
}
