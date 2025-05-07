#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#define FPGA_LW_BASE  0xFF200000
#define MAP_SIZE        0x1000

#define S_CAC_X_IDX     6
#define CG_X_IDX       15
#define LAVA_X_IDX     17

#define S_CAC_X_OFF  (S_CAC_X_IDX * 4)
#define CG_X_OFF     (CG_X_IDX    * 4)
#define LAVA_X_OFF   (LAVA_X_IDX  * 4)

#define SCREEN_W      640
#define SPEED           1

int main(void) {
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) { perror("open"); return 1; }

    void *base = mmap(NULL, MAP_SIZE,
                      PROT_READ|PROT_WRITE, MAP_SHARED,
                      fd, FPGA_LW_BASE);
    if (base == MAP_FAILED) { perror("mmap"); return 1; }

    volatile uint32_t *s_cac_x = (uint32_t *)((char*)base + S_CAC_X_OFF);
    volatile uint32_t *cg_x    = (uint32_t *)((char*)base + CG_X_OFF);
    volatile uint32_t *lava_x  = (uint32_t *)((char*)base + LAVA_X_OFF);

    // initial positions offscreen to the right
    *s_cac_x = SCREEN_W +  50;
    *cg_x    = SCREEN_W + 150;
    *lava_x  = SCREEN_W + 250;

    while (1) {
        uint32_t x;

        x = *s_cac_x;
        *s_cac_x = (x > SPEED) ? x - SPEED : 0;

        x = *cg_x;
        *cg_x    = (x > SPEED) ? x - SPEED : 0;

        x = *lava_x;
        *lava_x  = (x > SPEED) ? x - SPEED : 0;

        usleep(10000);  // ~100 FPS
    }

    // never reached
    munmap(base, MAP_SIZE);
    close(fd);
    return 0;
}
