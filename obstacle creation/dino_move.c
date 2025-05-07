#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN       8

// === Memory‐mapped I/O base & size ===
#define LW_BRIDGE_BASE   0xFF200000
#define MAP_SIZE         0x1000

// === PIO register offsets (word‐address × 4) ===
// These must match the case(address) in your vga_ball.sv:
//
//   0 ⇒ dino_x
//   1 ⇒ dino_y
//   2 ⇒ jump_x
//   3 ⇒ jump_y
//   4 ⇒ duck_x
//   5 ⇒ duck_y
//   6 ⇒ small_cactus_x
//   7 ⇒ small_cactus_y
//   8 ⇒ godzilla_x
//   9 ⇒ godzilla_y
//  10 ⇒ score
//  11 ⇒ score_x
//  12 ⇒ score_y
//  13 ⇒ ducking
//  14 ⇒ jumping
//  15 ⇒ group_cactus_x
//  16 ⇒ group_cactus_y
//  17 ⇒ lava_x
//  18 ⇒ lava_y
//  19 ⇒ ptero_x     // <– use if you add these to the hardware
//  20 ⇒ ptero_y

#define OFF_DINO_Y       (1*4)
#define OFF_SMALL_CAC_X  (6*4)
#define OFF_GROUP_CAC_X  (15*4)
#define OFF_LAVA_X       (17*4)
//#define OFF_PTERO_X    (19*4)    // uncomment if you wire these through
#define OFF_CACTUS_Y     (7*4)    // small‐cactus Y
#define OFF_GROUP_CAC_Y  (16*4)
#define OFF_LAVA_Y       (18*4)
//#define OFF_PTERO_Y    (20*4)    // ditto

#define GROUND_Y         120
#define GRAVITY          1
#define OBSTACLE_SPEED   1
#define SCREEN_WIDTH     640

int main(void) {
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) { perror("open /dev/mem"); return 1; }

    void *lw = mmap(NULL, MAP_SIZE,
                    PROT_READ|PROT_WRITE, MAP_SHARED,
                    fd, LW_BRIDGE_BASE);
    if (lw == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    // Pointers into the PIO‐mapped registers:
    volatile uint32_t *dino_y     = (uint32_t *)((char*)lw + OFF_DINO_Y);
    volatile uint32_t *small_cac_x= (uint32_t *)((char*)lw + OFF_SMALL_CAC_X);
    volatile uint32_t *group_cac_x= (uint32_t *)((char*)lw + OFF_GROUP_CAC_X);
    volatile uint32_t *lava_x     = (uint32_t *)((char*)lw + OFF_LAVA_X);
    //volatile uint32_t *ptero_x   = (uint32_t *)((char*)lw + OFF_PTERO_X);

    volatile uint32_t *small_cac_y= (uint32_t *)((char*)lw + OFF_CACTUS_Y);
    volatile uint32_t *group_cac_y= (uint32_t *)((char*)lw + OFF_GROUP_CAC_Y);
    volatile uint32_t *lava_y     = (uint32_t *)((char*)lw + OFF_LAVA_Y);
    //volatile uint32_t *ptero_y   = (uint32_t *)((char*)lw + OFF_PTERO_Y);

    // USB joystick setup
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred;
    libusb_init(NULL);
    pad = openkeyboard(&ep);
    if (!pad) { fprintf(stderr, "Controller not found\n"); return 1; }

    // initial positions (off right edge)
    *small_cac_x = SCREEN_WIDTH +  50;
    *group_cac_x = SCREEN_WIDTH + 150;
    *lava_x      = SCREEN_WIDTH + 250;
    // *ptero_x  = SCREEN_WIDTH + 350;

    *small_cac_y = GROUND_Y;
    *group_cac_y = GROUND_Y;
    *lava_y      = GROUND_Y;
    // *ptero_y  = GROUND_Y -  60;  // e.g. flying

    int y = GROUND_Y, v = 0;

    while (1) {
        // 1) read joystick & update dino Y
        if (libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0)==0
         && transferred==REPORT_LEN) {
            uint8_t y_axis = report[4];
            if (y_axis==0x00 && y==GROUND_Y) v = -12;
            v += GRAVITY;
            y += v;
            if (y > GROUND_Y) { y=GROUND_Y; v=0; }
            *dino_y = y;
        }

        // 2) slide obstacles left
        uint32_t sx = *small_cac_x;
        uint32_t gx = *group_cac_x;
        uint32_t lx = *lava_x;
        //uint32_t px = *ptero_x;

        sx = (sx > OBSTACLE_SPEED) ? sx - OBSTACLE_SPEED : SCREEN_WIDTH;
        gx = (gx > OBSTACLE_SPEED) ? gx - OBSTACLE_SPEED : SCREEN_WIDTH;
        lx = (lx > OBSTACLE_SPEED) ? lx - OBSTACLE_SPEED : SCREEN_WIDTH;
        //px = (px > OBSTACLE_SPEED) ? px - OBSTACLE_SPEED : SCREEN_WIDTH;

        *small_cac_x = sx;
        *group_cac_x = gx;
        *lava_x      = lx;
        //*ptero_x   = px;

        // 3) frame‐rate
        usleep(10000);
    }

    // never reached
    libusb_close(pad);
    libusb_exit(NULL);
    munmap(lw, MAP_SIZE);
    close(fd);
    return 0;
}
