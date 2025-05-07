#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>               // for bool
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"           // provides openkeyboard()

// =============== USB IDs (from lsusb) ===============
#ifndef USB_VID
#define USB_VID 0x0079            // DragonRise Inc.
#endif
#ifndef USB_PID
#define USB_PID 0x0011            // Gamepad
#endif

#define REPORT_LEN     8           // gamepad report size

// =============== MMIO map & register offsets (bytes) ===============
#define LW_BRIDGE_BASE 0xFF200000
#define MAP_SIZE       0x1000

#define DINO_X_OFFSET   0x0000     //  9'd0
#define DINO_Y_OFFSET   0x0004     //  9'd1

#define JUMP_X_OFFSET   0x0008     //  9'd2
#define JUMP_Y_OFFSET   0x000C     //  9'd3

#define DUCK_X_OFFSET   0x0010     //  9'd4
#define DUCK_Y_OFFSET   0x0014     //  9'd5

#define DUCKING_OFFSET (13*4)      //  9'd13
#define JUMPING_OFFSET (14*4)      //  9'd14

// =============== Game physics ===============
#define GROUND_Y       120         // must match your “floor” in Verilog
#define GRAVITY        1

int main(void) {
    // 1) open /dev/mem and map the lightweight bridge
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open(/dev/mem)"); return 1; }
    void *lw_base = mmap(NULL, MAP_SIZE,
                         PROT_READ|PROT_WRITE, MAP_SHARED,
                         fd, LW_BRIDGE_BASE);
    if (lw_base == MAP_FAILED) { perror("mmap"); return 1; }

    // 2) get pointers to every register
    volatile uint32_t *dino_x_reg = (uint32_t *)((char*)lw_base + DINO_X_OFFSET);
    volatile uint32_t *dino_y_reg = (uint32_t *)((char*)lw_base + DINO_Y_OFFSET);
    volatile uint32_t *jump_x_reg = (uint32_t *)((char*)lw_base + JUMP_X_OFFSET);
    volatile uint32_t *jump_y_reg = (uint32_t *)((char*)lw_base + JUMP_Y_OFFSET);
    volatile uint32_t *duck_x_reg = (uint32_t *)((char*)lw_base + DUCK_X_OFFSET);
    volatile uint32_t *duck_y_reg = (uint32_t *)((char*)lw_base + DUCK_Y_OFFSET);
    volatile uint32_t *duck_reg   = (uint32_t *)((char*)lw_base + DUCKING_OFFSET);
    volatile uint32_t *jump_reg   = (uint32_t *)((char*)lw_base + JUMPING_OFFSET);

    // 3) open the gamepad (this also finds the correct endpoint for you)
    struct libusb_device_handle *pad;
    uint8_t ep;
    pad = openkeyboard(&ep);
    if (!pad) {
        fprintf(stderr, "Cannot open gamepad\n");
        munmap(lw_base, MAP_SIZE);
        close(fd);
        return 1;
    }

    // 4) initialize static X positions (must match your sprite initial X in Verilog)
    const uint32_t XPOS = 100;    // e.g. 100 px from left
    *dino_x_reg = XPOS;
    *jump_x_reg = XPOS;
    *duck_x_reg = XPOS;

    // 5) game loop
    int y = GROUND_Y, v = 0;
    unsigned char report[REPORT_LEN];
    int transferred, r;

    while (1) {
        // read one interrupt packet
        r = libusb_interrupt_transfer(pad, ep,
                                      report, REPORT_LEN,
                                      &transferred, 0);
        if (r < 0) {
            fprintf(stderr, "USB read error: %d\n", r);
            break;
        }

        // 0x00 == up, 0x80 == center, 0xFF == down
        uint8_t y_axis = report[4];
        bool want_jump = (y_axis == 0x00 && y == GROUND_Y);
        bool want_duck = (y_axis == 0xFF && y == GROUND_Y);

        // only on press do we give the Dino upward velocity
        if (want_jump) {
            v = -12;
        }

        // update physics
        v += GRAVITY;
        y += v;
        if (y > GROUND_Y) {
            y = GROUND_Y;
            v = 0;
        }

        // 6) write **all** positions every frame
        *dino_y_reg = (uint32_t)y;
        *jump_y_reg = (uint32_t)y;
        *duck_y_reg = (uint32_t)y;

        // 7) write the flags
        *jump_reg = want_jump ? 1 : 0;
        *duck_reg = want_duck ? 1 : 0;

        usleep(5000);  // ~200 Hz update
    }

    // cleanup
    libusb_close(pad);
    libusb_exit(NULL);
    munmap(lw_base, MAP_SIZE);
    close(fd);
    return 0;
}
