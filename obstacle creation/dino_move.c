#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define LW_BRIDGE_BASE   0xFF200000
#define MAP_SIZE         0x1000

#define DINO_Y_OFFSET    0x0004
#define CACTUS_X_OFFSET  0x0008
#define PTERO_X_OFFSET   0x000C
#define LAVA_X_OFFSET    0x0010

#define GROUND_Y         120
#define OBSTACLE_SPEED     1
#define SCREEN_WIDTH    1280   // Must match your VGA HACTIVE

int main(void) {
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) { perror("open"); return 1; }

    void *mem = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE,
                     MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (mem == MAP_FAILED) { perror("mmap"); return 1; }

    // Byte-pointer so offsets are in raw bytes
    uint8_t *base = (uint8_t*)mem;

    // Map our four 32-bit registers
    volatile uint32_t *dino_y   = (uint32_t*)(base + DINO_Y_OFFSET);
    volatile uint32_t *cactus_x = (uint32_t*)(base + CACTUS_X_OFFSET);
    volatile uint32_t *ptero_x  = (uint32_t*)(base + PTERO_X_OFFSET);
    volatile uint32_t *lava_x   = (uint32_t*)(base + LAVA_X_OFFSET);

    // Initial positions
    *dino_y   = GROUND_Y;
    *cactus_x = SCREEN_WIDTH +  50;
    *ptero_x  = SCREEN_WIDTH + 150;
    *lava_x   = SCREEN_WIDTH + 250;

    while (1) {
        // Keep Dino on the ground
        *dino_y = GROUND_Y;

        // Slide each obstacle left by OBSTACLE_SPEED, wrap at SCREEN_WIDTH
        uint32_t x;
        x = *cactus_x;
        *cactus_x = (x > OBSTACLE_SPEED) ? x - OBSTACLE_SPEED : SCREEN_WIDTH;

        x = *ptero_x;
        *ptero_x = (x > OBSTACLE_SPEED) ? x - OBSTACLE_SPEED : SCREEN_WIDTH;

        x = *lava_x;
        *lava_x = (x > OBSTACLE_SPEED) ? x - OBSTACLE_SPEED : SCREEN_WIDTH;

        // ~100 Hz update
        usleep(10000);
    }

    // (never reached, but good form)
    munmap(mem, MAP_SIZE);
    close(fd);
    return 0;
}
