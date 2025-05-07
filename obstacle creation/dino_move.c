#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

// how many bytes we'll map from /dev/mem
#define MAP_SIZE         0x1000
#define LW_BRIDGE_BASE   0xFF200000

// these MUST match the 'case(address)' indices in your vga_ball.sv:
//   15→cg_x, 16→cg_y, 17→lava_x, 18→lava_y
#define DINO_Y_OFFSET    0x04  // address==1
#define CG_X_OFFSET      0x3C  // address==15*4
#define CG_Y_OFFSET      0x40  // address==16*4
#define LAVA_X_OFFSET    0x44  // address==17*4
#define LAVA_Y_OFFSET    0x48  // address==18*4

#define REPORT_LEN       8
#define GROUND_Y       120
#define GRAVITY          1
#define OBSTACLE_SPEED   1
#define SCREEN_WIDTH  640

int main(void) {
    // 1) map the lightweight bridge
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }
    void *base = mmap(NULL, MAP_SIZE,
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (base == MAP_FAILED) { perror("mmap"); return 1; }

    // 2) compute pointers to each register (byte offsets!)
    volatile uint32_t *dino_y = 
      (volatile uint32_t *)((char*)base + DINO_Y_OFFSET);
    volatile uint32_t *cg_x   = 
      (volatile uint32_t *)((char*)base + CG_X_OFFSET);
    volatile uint32_t *cg_y   = 
      (volatile uint32_t *)((char*)base + CG_Y_OFFSET);
    volatile uint32_t *lava_x = 
      (volatile uint32_t *)((char*)base + LAVA_X_OFFSET);
    volatile uint32_t *lava_y = 
      (volatile uint32_t *)((char*)base + LAVA_Y_OFFSET);

    // 3) USB joystick setup (exactly as before)
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred;
    pad = openkeyboard(&ep);
    if (!pad) {
      fprintf(stderr, "Controller not found\n");
      return 1;
    }

    // 4) initial positions (off‐screen to the right)
    *cg_x   = SCREEN_WIDTH +  50;
    *cg_y   = /* ground‐y in your VGA logic */ 248;
    *lava_x = SCREEN_WIDTH + 250;
    *lava_y = 248;

    // game state
    int y = GROUND_Y, v = 0;

    while (1) {
      // a) read joystick & update jump
      if ( libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0)==0
        && transferred==REPORT_LEN)
      {
        uint8_t y_axis = report[4];
        if (y_axis==0x00 && y==GROUND_Y) v = -12;
        v += GRAVITY;
        y += v;
        if (y>GROUND_Y) { y=GROUND_Y; v=0; }
        *dino_y = y;
      }

      // b) slide obstacles left, wrap at screen‐width
      uint32_t x;
      x = *cg_x;
      *cg_x   = (x>OBSTACLE_SPEED) ? x-OBSTACLE_SPEED : SCREEN_WIDTH;
      x = *lava_x;
      *lava_x = (x>OBSTACLE_SPEED) ? x-OBSTACLE_SPEED : SCREEN_WIDTH;

      // c) throttle frame‐rate
      usleep(10000);  // ~100 FPS
    }

    // never reached, but nice cleanup:
    libusb_close(pad);
    libusb_exit(NULL);
    munmap(base, MAP_SIZE);
    close(fd);
    return 0;
}
