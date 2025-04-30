
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <math.h>
#include <sys/mman.h>
#include <libusb-1.0/libusb.h>
#include "usbkeyboard.h"

#define REPORT_LEN 8

#define LW_BRIDGE_BASE 0xFF200000
#define DINO_Y_OFFSET  0x0004
#define DUCK_OFFSET    0x0008
#define JUMP_OFFSET    0x000C
#define MAP_SIZE       0x1000

#define AUDIO_BASE     0x3040
#define AUDIO_LEFT     (AUDIO_BASE + 0x00)
#define AUDIO_RIGHT    (AUDIO_BASE + 0x04)
#define AUDIO_STATUS   (AUDIO_BASE + 0x08)

#define GROUND_Y 120
#define GRAVITY 1
#define SAMPLE_RATE 48000

// Tone generator
void play_tone(volatile uint32_t *audio_base, int freq, int duration_ms) {
    volatile uint32_t *audio_left = audio_base;
    volatile uint32_t *audio_right = audio_base + 1;
    volatile uint32_t *audio_status = audio_base + 2;

    int samples = (SAMPLE_RATE * duration_ms) / 1000;
    double step = (2.0 * M_PI * freq) / SAMPLE_RATE;

    for (int i = 0; i < samples; i++) {
        int16_t sample = (int16_t)(sin(step * i) * 28000);
        while (!((*audio_status >> 16) & 0xFF));
        *audio_left = sample;
        *audio_right = sample;
    }
}

int main() {
    // Map FPGA registers
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *lw_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, LW_BRIDGE_BASE);
    volatile uint32_t *dino_y_reg = (uint32_t *)((char *)lw_base + DINO_Y_OFFSET);
    volatile uint32_t *ducking_reg = (uint32_t *)((char *)lw_base + DUCK_OFFSET);
    volatile uint32_t *jumping_reg = (uint32_t *)((char *)lw_base + JUMP_OFFSET);
    volatile uint32_t *audio_base = (uint32_t *)((char *)lw_base + AUDIO_BASE);

    // USB setup
    struct libusb_device_handle *pad;
    uint8_t ep;
    unsigned char report[REPORT_LEN];
    int transferred;

    pad = openkeyboard(&ep);
    if (!pad) {
        fprintf(stderr, "Controller not found\n");
        return 1;
    }

    // Dino state
    int y = GROUND_Y;
    int v = 0;

    while (1) {
        int r = libusb_interrupt_transfer(pad, ep, report, REPORT_LEN, &transferred, 0);
        if (r == 0 && transferred == REPORT_LEN) {
            uint8_t y_axis = report[4];
            uint8_t button = report[5];

            if (y_axis == 0x00 && y == GROUND_Y) {
                v = -12;
                *ducking_reg = 0;
                play_tone(audio_base, 800, 80);
            } else if (y_axis == 0xFF) {
                *ducking_reg = 1;
            } else {
                *ducking_reg = 0;
            }

            // Optional sound buttons
            if (button == 0x2F) {
                play_tone(audio_base, 1000, 60);
            } else if (button == 0x4F) {
                play_tone(audio_base, 400, 100);
            }

            // Physics + jump flag
            v += GRAVITY;
            y += v;
            if (y > GROUND_Y) {
                y = GROUND_Y;
                v = 0;
            }

            *dino_y_reg = y;
            *jumping_reg = (y == GROUND_Y) ? 0 : 1;

            usleep(5000);
        }
    }

    libusb_close(pad);
    libusb_exit(NULL);
    munmap(lw_base, MAP_SIZE);
    close(fd);
    return 0;
}
