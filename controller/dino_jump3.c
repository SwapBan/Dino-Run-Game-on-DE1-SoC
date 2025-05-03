
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"
// <<< ADDED: for Wolfson codec I²C init
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#define REPORT_LEN 8

#define OFF_PLAY   0x003C   // must match whatever address your HDL uses for play_jump
#define OFF_VOL    0x0040   // ditto for vol_jump
#define OFF_DELAY  0x0044   // ditto for delay

// Memory-mapped I/O base
#define LW_BRIDGE_BASE 0xFF200000
#define DINO_Y_OFFSET  0x0004
#define MAP_SIZE       0x1000

#define GROUND_Y 120
#define GRAVITY 1

int main() {
    // Map FPGA registers
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }
    void *lw_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (lw_base == MAP_FAILED) { perror("mmap"); return 1; }
  // <<< ADDED: Wolfson codec initialization via I²C
    int i2c_fd = open("/dev/i2c-0", O_RDWR);
    if (i2c_fd < 0) { perror("open /dev/i2c-0"); return 1; }
    if (ioctl(i2c_fd, I2C_SLAVE, 0x1B) < 0) { perror("ioctl I2C_SLAVE"); return 1; }
    uint8_t cfg[2];
    // reset
    cfg[0]=0x0F; cfg[1]=0x00; write(i2c_fd, cfg,2);
    // power-up
    cfg[0]=0x0C; cfg[1]=0x00; write(i2c_fd, cfg,2);
    // left line in +0dB
    cfg[0]=0x00; cfg[1]=0x17; write(i2c_fd, cfg,2);
    // right line in +0dB
    cfg[0]=0x01; cfg[1]=0x17; write(i2c_fd, cfg,2);
    // left headphone out 0dB
    cfg[0]=0x02; cfg[1]=0x79; write(i2c_fd, cfg,2);
    // right headphone out 0dB
    cfg[0]=0x03; cfg[1]=0x79; write(i2c_fd, cfg,2);
    // analog audio path
    cfg[0]=0x08; cfg[1]=0x12; write(i2c_fd, cfg,2);
    // digital audio path
    cfg[0]=0x09; cfg[1]=0x00; write(i2c_fd, cfg,2);
    // I2S, 16-bit
    cfg[0]=0x07; cfg[1]=0x02; write(i2c_fd, cfg,2);
    // sample rate 48 kHz
    cfg[0]=0x0A; cfg[1]=0x00; write(i2c_fd, cfg,2);
    // activate
    cfg[0]=0x06; cfg[1]=0x01; write(i2c_fd, cfg,2);
    close(i2c_fd);
    // <<< END ADDED codec init

    volatile uint32_t *dino_y_reg = (uint32_t *)((char *)lw_base + DINO_Y_OFFSET);

  // <<< ADDED: pointers into the same LW bridge for audio control
    volatile uint32_t *play_jump  =
      (uint32_t *)((char*)lw_base + OFF_PLAY);
    volatile uint32_t *vol_jump   =
      (uint32_t *)((char*)lw_base + OFF_VOL);
    volatile uint32_t *delay_jump =
      (uint32_t *)((char*)lw_base + OFF_DELAY);

    // Set up USB controller
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred;

    pad = openkeyboard(&ep);
    if (!pad) {
        fprintf(stderr, "Controller not found\n");
        return 1;
    }

    // Game loop: physics and input
    int y = GROUND_Y;
    int v = 0;

    while (1) {
        int r = libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0);
        if (r == 0 && transferred == REPORT_LEN) {
            uint8_t y_axis = report[4];

            if (y_axis == 0x00 && y == GROUND_Y) {
                v = -12;  // Jump trigger
              // <<< ADDED: trigger our jump sound
                *vol_jump   =  8;    // set volume (0–15)
                *delay_jump =  2;    // sample-step delay
                *play_jump  =  1;    // strobe playback
            }

            // Apply gravity
            v += GRAVITY;
            y += v;
            if (y > GROUND_Y) {
                y = GROUND_Y;
                v = 0;
            }

            *dino_y_reg = y;
            usleep(5000);
        }
    }

    libusb_close(pad);
    libusb_exit(NULL);
    munmap(lw_base, MAP_SIZE);
    close(fd);
    return 0;
}
