#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN       8
#define LW_BRIDGE_BASE   0xFF200000
#define MAP_SIZE         0x1000

// Register offsets (in bytes)
#define DINO_Y_OFFSET     0x0004      // reg 1
#define DUCKING_OFFSET    (13 * 4)    // reg13
#define JUMPING_OFFSET    (14 * 4)    // reg14
#define REPLAY_OFFSET     (19 * 4)    // reg19 = controller_report[4]

// Physics
#define GROUND_Y          100
#define GRAVITY           1

int main(void) {
    // MMIO setup
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open(/dev/mem)"); return 1; }

    void *lw_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (lw_base == MAP_FAILED) { perror("mmap"); return 1; }

    volatile uint32_t *dino_y_reg = (uint32_t *)(lw_base + DINO_Y_OFFSET);
    volatile uint32_t *duck_reg   = (uint32_t *)(lw_base + DUCKING_OFFSET);
    volatile uint32_t *jump_reg   = (uint32_t *)(lw_base + JUMPING_OFFSET);
    volatile uint32_t *replay_reg = (uint32_t *)(lw_base + REPLAY_OFFSET);

    // USB gamepad setup
    struct libusb_device_handle *pad;
    uint8_t ep;
    pad = openkeyboard(&ep);
    if (!pad) {
        fprintf(stderr, "Controller not found\n");
        munmap(lw_base, MAP_SIZE);
        close(fd);
        return 1;
    }

    int y = GROUND_Y, v = 0;
    unsigned char report[REPORT_LEN];
    int transferred, r;
    bool last_replay = false;

    while (1) {
        r = libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0);
        if (r < 0) {
            fprintf(stderr, "USB read error: %d\n", r);
            break;
        }

        // Joystick jump/duck
        uint8_t y_axis = report[4];
        bool want_jump = (y_axis == 0x00 && y == GROUND_Y);
        bool want_duck = (y_axis == 0xFF && y == GROUND_Y);

        if (want_jump) v = -12;
        *jump_reg = want_jump;
        *duck_reg = want_duck;

        // Start button triggers replay
        bool want_replay = (report[5] & 0x20); // Start = bit 5 of byte 5

        if (want_replay && !last_replay) {
            *replay_reg = 1;
            usleep(1000);       // brief pause to ensure pulse registers
            *replay_reg = 0;
        }
        last_replay = want_replay;

        // Physics
        v += GRAVITY;
        y += v;
        if (y > GROUND_Y) { y = GROUND_Y; v = 0; }

        *dino_y_reg = (uint32_t)y;
        usleep(5000);
    }

    libusb_close(pad);
    libusb_exit(NULL);
    munmap(lw_base, MAP_SIZE);
    close(fd);
    return 0;
}
