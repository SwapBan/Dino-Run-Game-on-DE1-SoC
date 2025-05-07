// game.c
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

// ------------------------------------------------------------------
//  HPS→FPGA lightweight bridge (LW) base and span
// ------------------------------------------------------------------
#define LW_BRIDGE_BASE 0xFF200000UL
#define LW_BRIDGE_SPAN 0x00001000UL

// PIO offsets (in bytes) from LW_BRIDGE_BASE
#define OFF_DINO_X    (0x00)
#define OFF_DINO_Y    (0x04)
#define OFF_CG_X      (0x3C)  // 15*4
#define OFF_CG_Y      (0x40)  // 16*4
#define OFF_LAVA_X    (0x44)  // 17*4
#define OFF_LAVA_Y    (0x48)  // 18*4
#define OFF_KEYS      (0x50)  // pushbutton PIO

// ------------------------------------------------------------------
//  Screen & sprite dims
// ------------------------------------------------------------------
#define SCREEN_W    1280
#define DINO_W       32
#define DINO_H       32
#define CG_W        150
#define CG_H         40
#define LAVA_W       32
#define LAVA_H       32

// AABB overlap test
static bool overlap(int ax,int ay,int aw,int ah,
                    int bx,int by,int bw,int bh) {
    return !( ax+aw <= bx ||
              bx+bw <= ax ||
              ay+ah <= by ||
              by+bh <= ay );
}

int main() {
    int    fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) { perror("open /dev/mem"); return 1; }

    void *lw_base = mmap(NULL, LW_BRIDGE_SPAN, PROT_READ|PROT_WRITE,
                         MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (lw_base == MAP_FAILED) { perror("mmap"); return 1; }

    volatile uint32_t *pio = (volatile uint32_t *)lw_base;

    // PIO register pointers
    volatile uint32_t *r_dino_x  = pio + (OFF_DINO_X/4);
    volatile uint32_t *r_dino_y  = pio + (OFF_DINO_Y/4);
    volatile uint32_t *r_cg_x    = pio + (OFF_CG_X/4);
    volatile uint32_t *r_cg_y    = pio + (OFF_CG_Y/4);
    volatile uint32_t *r_lava_x  = pio + (OFF_LAVA_X/4);
    volatile uint32_t *r_lava_y  = pio + (OFF_LAVA_Y/4);
    volatile uint32_t *r_keys    = pio + (OFF_KEYS/4);

    // initialize to match your Verilog reset
    *r_dino_x = 100;  *r_dino_y = 248;
    *r_cg_x   = 400;  *r_cg_y   = 248;
    *r_lava_x = 600;  *r_lava_y = 248;

    int speed_cg   = 1;
    int speed_lava = 2;
    bool collision = false;

    while (1) {
        // ~16 ms for ~60 Hz frame
        usleep(16666);

        // read pushbuttons (active‐low)
        uint32_t keys = *r_keys & 0x7;  
        // KEY0=bit0, KEY1=bit1, KEY2=bit2

        // dino up/down via KEY0/KEY1
        if (!(keys & 0x1))  (*r_dino_y)--;
        if (!(keys & 0x2))  (*r_dino_y)++;

        // obstacle motion
        if (!collision) {
            *r_cg_x   -= speed_cg;
            *r_lava_x -= speed_lava;

            if ((int)*r_cg_x + CG_W <= 0) {
                *r_cg_x = SCREEN_W;
                speed_cg++;
            }
            if ((int)*r_lava_x + LAVA_W <= 0) {
                *r_lava_x = SCREEN_W;
                speed_lava++;
            }
        }

        // collision detection
        if (!collision &&
            ( overlap(*r_dino_x, *r_dino_y, DINO_W, DINO_H,
                      *r_cg_x,   *r_cg_y,   CG_W,   CG_H)   ||
              overlap(*r_dino_x, *r_dino_y, DINO_W, DINO_H,
                      *r_lava_x, *r_lava_y, LAVA_W, LAVA_H) ))
        {
            collision = true;
        }

        // on collision, wait for KEY2 to restart
        if (collision) {
            if (!(keys & 0x4)) {  // KEY2 pressed
                *r_cg_x   = 400;   speed_cg   = 1;
                *r_lava_x = 600;   speed_lava = 2;
                collision = false;
            }
        }
    }

    // never reached
    munmap(lw_base, LW_BRIDGE_SPAN);
    close(fd);
    return 0;
}
