#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>

#define LW_BASE       0xFF200000
#define MAP_SIZE      0x1000

// these indices must match your vga_ball's case(address)
#define S_CAC_X_IDX   6
#define CG_X_IDX     15
#define LAVA_X_IDX   17

// byte offsets = index * 4
#define S_CAC_X_OFF  (S_CAC_X_IDX * 4)
#define CG_X_OFF     (CG_X_IDX    * 4)
#define LAVA_X_OFF   (LAVA_X_IDX  * 4)

#define SCREEN_WIDTH 640
#define SPEED         1

int main() {
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    void *base = mmap(NULL, MAP_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED,
                      fd, LW_BASE);

    volatile uint32_t *s_cac_x = (uint32_t *)((char*)base + S_CAC_X_OFF);
    volatile uint32_t *cg_x    = (uint32_t *)((char*)base + CG_X_OFF);
    volatile uint32_t *lava_x  = (uint32_t *)((char*)base + LAVA_X_OFF);

    // initialize them off the right edge
    *s_cac_x = SCREEN_WIDTH +  50;
    *cg_x    = SCREEN_WIDTH + 150;
    *lava_x  = SCREEN_WIDTH + 250;

    while (1) {
        // move each left by SPEED; wrap around to SCREEN_WIDTH
        uint32_t x;

        x = *s_cac_x;
        *s_cac_x = (x > SPEED) ? x - SPEED : SCREEN_WIDTH;

        x = *cg_x;
        *cg_x    = (x > SPEED) ? x - SPEED : SCREEN_WIDTH;

        x = *lava_x;
        *lava_x  = (x > SPEED) ? x - SPEED : SCREEN_WIDTH;

        usleep(10000);
    }
}
