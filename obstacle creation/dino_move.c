#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN     8

// Lightweight HPS → FPGA bus base and map size
#define LW_BRIDGE_BASE 0xFF200000
#define MAP_SIZE       0x1000

// These must match the `.address` indices in your vga_ball’s write logic:
//   address == 15 → cg_x, address == 16 → cg_y
// so byte-offset = index * 4
#define CG_X_OFFSET    (15 * 4)    // 0x3C
#define CG_Y_OFFSET    (16 * 4)    // 0x40

#define SCREEN_WIDTH  1280         // must match HACTIVE in your VGA counters
#define OBSTACLE_SPEED   1         // pixels per frame

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }

    void *lw = mmap(NULL, MAP_SIZE,
                    PROT_READ|PROT_WRITE, MAP_SHARED,
                    fd, LW_BRIDGE_BASE);
    if (lw == MAP_FAILED) { perror("mmap"); return 1; }

    // Point at the group-cactus X register
    volatile uint32_t *cg_x = (uint32_t *)((char*)lw + CG_X_OFFSET);

    // USB joystick setup (you can ignore if you just want the obstacle to move)
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred;
    pad = openkeyboard(&ep);
    if (!pad) {
      fprintf(stderr,"Controller not found\n");
      return 1;
    }

    // initialize cactus off-screen right
    *cg_x = SCREEN_WIDTH + 50;

    while (1) {
      // (1) if you want to do your jump logic here, leave it in…
      //     otherwise you can delete this block.
      if (libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0)==0 &&
          transferred==REPORT_LEN)
      {
        // …your jump code…
      }

      // (2) move cactus left
      uint32_t x = *cg_x;
      if (x > OBSTACLE_SPEED) {
        x -= OBSTACLE_SPEED;
      } else {
        x = SCREEN_WIDTH + 50;       // wrap back to right again
      }
      *cg_x = x;

      // (3) frame-rate throttle
      usleep(10000);   // ~100 Hz
    }

    // unreachable in practice
    libusb_close(pad);
    libusb_exit(NULL);
    munmap(lw, MAP_SIZE);
    close(fd);
    return 0;
}
