
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <libusb-1.0/libusb.h>
#include <math.h>
#include <sys/mman.h>

#define SONY_VENDOR_ID 0x0810  // Example for DragonRise; change if needed
#define PRODUCT_ID     0xE501  // Replace with actual Product ID if different
#define LW_BRIDGE_BASE 0xFF200000
#define DINO_Y_OFFSET  0x1000
#define DUCK_OFFSET    0x1004
#define MAP_SIZE       0x10000

#define SAMPLE_RATE 48000

volatile uint32_t *dino_y_reg;
volatile uint32_t *ducking_reg;

// Tone generator using audio core
void play_tone(int freq, int duration_ms) {
    volatile uint32_t *audio_base = (uint32_t *)(LW_BRIDGE_BASE + 0x3040);
    volatile uint32_t *audio_left = audio_base + 0;
    volatile uint32_t *audio_right = audio_base + 1;
    volatile uint32_t *audio_status = audio_base + 2;

    int samples = (SAMPLE_RATE * duration_ms) / 1000;
    double step = (2.0 * M_PI * freq) / SAMPLE_RATE;

    for (int i = 0; i < samples; i++) {
        int16_t sample = (int16_t)(sin(step * i) * 30000);
        while (!((*audio_status >> 16) & 0xFF)); // Wait for space
        *audio_left = sample;
        *audio_right = sample;
    }
}

int main() {
    libusb_context *ctx = NULL;
    libusb_device_handle *handle = NULL;

    // Set up memory-mapped registers
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *lw_base = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, LW_BRIDGE_BASE);
    dino_y_reg = (uint32_t *)(lw_base + DINO_Y_OFFSET);
    ducking_reg = (uint32_t *)(lw_base + DUCK_OFFSET);

    // Setup libusb
    libusb_init(&ctx);
    handle = libusb_open_device_with_vid_pid(ctx, SONY_VENDOR_ID, PRODUCT_ID);
    if (!handle) {
        printf("Joystick not found.\n");
        return 1;
    }

    if (libusb_kernel_driver_active(handle, 0)) {
        libusb_detach_kernel_driver(handle, 0);
    }
    libusb_claim_interface(handle, 0);

    unsigned char report[8];
    int transferred;
    int r;

    // Dino game logic
    int y = 120, v = 0;
    const int ground = 120;
    const int gravity = 1;

    while (1) {
        r = libusb_interrupt_transfer(handle, 0x81, report, sizeof(report), &transferred, 0);
        if (r == 0 && transferred == 8) {
            uint8_t y_axis = report[4];
            uint8_t button = report[5];

            if (y_axis == 0x00 && y == ground) {
                v = -12;
                *ducking_reg = 0;
                play_tone(800, 100);
            } else if (y_axis == 0xFF) {
                *ducking_reg = 1;
            } else {
                *ducking_reg = 0;
            }

            // Jump physics
            v += gravity;
            y += v;
            if (y > ground) { y = ground; v = 0; }

            *dino_y_reg = y;

            // Button A = jump sound
            if (button == 0x2F) {
                play_tone(1000, 80);
            }

            // Button B = crash sound
            if (button == 0x4F) {
                play_tone(300, 120);
            }
        }
        usleep(5000);
    }

    libusb_release_interface(handle, 0);
    libusb_close(handle);
    libusb_exit(ctx);
    return 0;
}
