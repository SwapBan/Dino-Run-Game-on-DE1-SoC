#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define LW_BRIDGE_BASE 0xFF200000
#define MAP_SIZE       0x1000

// these indices must match your case(address) in vga_ball.sv:
//   0→dino_x, …, 15→cg_x, 16→cg_y, 17→lava_x, 18→lava_y
#define CG_X_OFFSET   (15*4)   // word 15
#define CG_Y_OFFSET   (16*4)
#define LAVA_X_OFFSET (17*4)
#define LAVA_Y_OFFSET (18*4)

#define SCREEN_WIDTH  1280     // HACTIVE in your vga_counters
#define OBSTACLE_SPEED   1

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }

    void *lw = mmap(NULL, MAP_SIZE,
                    PROT_READ|PROT_WRITE, MAP_SHARED,
                    fd, LW_BRIDGE_BASE);
    if (lw == MAP_FAILED) { perror("mmap"); return 1; }

    volatile uint32_t *cg_x   = (uint32_t*)(lw + CG_X_OFFSET);
    volatile uint32_t *cg_y   = (uint32_t*)(lw + CG_Y_OFFSET);
    volatile uint32_t *lava_x = (uint32_t*)(lw + LAVA_X_OFFSET);
    volatile uint32_t *lava_y = (uint32_t*)(lw + LAVA_Y_OFFSET);

    // place them just off the right edge
    *cg_x   = SCREEN_WIDTH + 100;  // starts at x=1380
    *cg_y   = 248;                 // same ground‐y as your dino
    *lava_x = SCREEN_WIDTH + 300;  // starts a bit farther out
    *lava_y = 248;

    while (1) {
        // read back
        uint32_t cx = *cg_x;
        uint32_t lx = *lava_x;

        // slide left; wrap to right edge when off the left
        cx = (cx > OBSTACLE_SPEED) ? cx - OBSTACLE_SPEED : SCREEN_WIDTH;
        lx = (lx > OBSTACLE_SPEED) ? lx - OBSTACLE_SPEED : SCREEN_WIDTH;

        *cg_x   = cx;
        *lava_x = lx;

        printf("group_cactus@%4u   lava@%4u\r", cx, lx);
        fflush(stdout);

        usleep(10000);  // ~100 Hz update
    }

    return 0;
}
