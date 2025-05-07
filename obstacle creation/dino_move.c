#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

#define LW_BRIDGE_BASE   0xFF200000
#define MAP_SIZE         0x1000

// Avalon-MM “address” × 4 bytes:
#define DINO_Y_OFFSET    0x04    // reg 1
#define CACTUS_X_OFFSET  0x3C    // reg 15
#define PTERO_X_OFFSET   0x40    // reg 16
#define LAVA_X_OFFSET    0x44    // reg 17

#define SCREEN_WIDTH     640
#define OBSTACLE_SPEED     1

int main() {
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) { perror("open"); return 1; }
    void *lw = mmap(NULL, MAP_SIZE,
                    PROT_READ|PROT_WRITE, MAP_SHARED,
                    fd, LW_BRIDGE_BASE);
    if (lw == MAP_FAILED) { perror("mmap"); return 1; }

    volatile uint32_t *cactus_x = (uint32_t *)(lw + CACTUS_X_OFFSET);
    volatile uint32_t *ptero_x  = (uint32_t *)(lw + PTERO_X_OFFSET);
    volatile uint32_t *lava_x   = (uint32_t *)(lw + LAVA_X_OFFSET);

    // place them off-screen to the right
    *cactus_x = SCREEN_WIDTH +  50;
    *ptero_x  = SCREEN_WIDTH + 150;
    *lava_x   = SCREEN_WIDTH + 250;

    while (1) {
        uint32_t x;

        x = *cactus_x;
        *cactus_x = (x > OBSTACLE_SPEED) ? x - OBSTACLE_SPEED : 0;

        x = *ptero_x;
        *ptero_x  = (x > OBSTACLE_SPEED) ? x - OBSTACLE_SPEED : 0;

        x = *lava_x;
        *lava_x   = (x > OBSTACLE_SPEED) ? x - OBSTACLE_SPEED : 0;

        // DEBUG: print once every 100 frames
        static int frame=0;
        if (++frame == 100) {
            printf("   cactus=%3u   ptero=%3u   lava=%3u\n",
                   *cactus_x, *ptero_x, *lava_x);
            fflush(stdout);
            frame = 0;
        }

        usleep(10_000);  // ~100 Hz
    }

    munmap(lw, MAP_SIZE);
    close(fd);
    return 0;
}
