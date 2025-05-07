#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

#define LW_BASE        0xFF200000
#define MAP_SIZE       0x1000

// match your vga_ball.sv case(address) × 4
#define OFF_GROUP_CAC_X  (15*4)
#define OFF_LAVA_X       (17*4)

#define SCREEN_WIDTH   640    // visible area width
#define WRAP_OFFSET    100    // how far off‐screen to respawn
#define SPEED            1    // pixels per frame

int main(void) {
    // 1) open /dev/mem and mmap
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }
    void *base = mmap(NULL, MAP_SIZE,
                      PROT_READ|PROT_WRITE, MAP_SHARED,
                      fd, LW_BASE);
    if (base == MAP_FAILED) { perror("mmap"); close(fd); return 1; }

    // 2) pointers to your two PIO registers
    volatile uint32_t *gx = (uint32_t *)( (char*)base + OFF_GROUP_CAC_X );
    volatile uint32_t *lx = (uint32_t *)( (char*)base + OFF_LAVA_X      );

    // 3) initial positions (just off the right edge)
    *gx = SCREEN_WIDTH + WRAP_OFFSET;
    *lx = SCREEN_WIDTH + WRAP_OFFSET*2;

    // 4) main loop: move left, wrap when <=0
    while (1) {
        uint32_t g = *gx;
        uint32_t l = *lx;

        // move left or wrap
        g = (g > SPEED) ? g - SPEED : SCREEN_WIDTH + WRAP_OFFSET;
        l = (l > SPEED) ? l - SPEED : SCREEN_WIDTH + WRAP_OFFSET*2;

        *gx = g;
        *lx = l;

        usleep(20000);  // ~50 Hz update
    }

    // never reached
    munmap(base, MAP_SIZE);
    close(fd);
    return 0;
}
