#include <stdint.h>
#include <stdbool.h>

// ------------------------------------------------------------------
//  Hardware register base & offsets (word-aligned)
// ------------------------------------------------------------------
#define PIO_BASE      0xFF200000UL

// From your Verilog’s chipselect/write case(address):
//   0 → dino_x    1 → dino_y
//   …
//   6 → s_cac_x   7 → s_cac_y
//  15 → cg_x     16 → cg_y
//  17 → lava_x   18 → lava_y
#define REG_DINO_X   (*(volatile uint32_t*)(PIO_BASE +  0*4))
#define REG_DINO_Y   (*(volatile uint32_t*)(PIO_BASE +  1*4))
#define REG_S_CAC_X  (*(volatile uint32_t*)(PIO_BASE +  6*4))
#define REG_S_CAC_Y  (*(volatile uint32_t*)(PIO_BASE +  7*4))
#define REG_CG_X     (*(volatile uint32_t*)(PIO_BASE + 15*4))
#define REG_CG_Y     (*(volatile uint32_t*)(PIO_BASE + 16*4))
#define REG_LAVA_X   (*(volatile uint32_t*)(PIO_BASE + 17*4))
#define REG_LAVA_Y   (*(volatile uint32_t*)(PIO_BASE + 18*4))

// (You currently don’t have a PIO mapping for ptr_x/ptr_y in your Verilog;
// if you need to drive pterodactyl from C you’ll want to add case 19/20
// in your Verilog and then define REG_PTR_X = PIO_BASE+19*4, etc.)

// ------------------------------------------------------------------
//  Sprite & screen dims for collision checks
// ------------------------------------------------------------------
#define SCREEN_W   1280
#define DINO_W      32
#define DINO_H      32
#define CG_W       150
#define CG_H        40
#define LAVA_W      32
#define LAVA_H      32

// ------------------------------------------------------------------
//  Simple AABB overlap test
// ------------------------------------------------------------------
static bool overlap(int ax, int ay, int aw, int ah,
                    int bx, int by, int bw, int bh)
{
    return !( ax + aw <= bx ||
              bx + bw <= ax ||
              ay + ah <= by ||
              by + bh <= ay );
}

int main(void) {
    // initial positions (must match your Verilog reset values)
    int dino_x = REG_DINO_X;   // you’ll update this from your controller code
    int dino_y = REG_DINO_Y;

    int cg_x   = REG_CG_X;     // starts at 400 in Verilog
    int cg_y   = REG_CG_Y;     // 248

    int lava_x = REG_LAVA_X;   // 600
    int lava_y = REG_LAVA_Y;   // 248

    // obstacle speeds
    int speed_cg   = 1;
    int speed_lava = 2;

    bool collision = false;

    for (;;) {
        wait_for_vsync();      // sync to start of frame

        // --- 1) read and apply your existing Dino control updates ---
        read_controller();     // updates some local dino_x/dino_y
        REG_DINO_X = dino_x;
        REG_DINO_Y = dino_y;

        // --- 2) obstacle movement & wrap & speed-up ---
        if (!collision) {
            cg_x   -= speed_cg;
            lava_x -= speed_lava;

            if (cg_x + CG_W <= 0) {
                cg_x = SCREEN_W;
                speed_cg++;
            }
            if (lava_x + LAVA_W <= 0) {
                lava_x = SCREEN_W;
                speed_lava++;
            }

            REG_CG_X   = cg_x;
            REG_LAVA_X = lava_x;
        }

        // --- 3) collision detection (AABB) ---
        if (!collision) {
            if ( overlap(dino_x, dino_y, DINO_W, DINO_H,
                         cg_x,    cg_y,    CG_W,   CG_H) ||
                 overlap(dino_x, dino_y, DINO_W, DINO_H,
                         lava_x,  lava_y,  LAVA_W, LAVA_H) )
            {
                collision = true;
            }
        }

        // --- 4) on collision, show Game Over & wait for Start to reset ---
        if (collision) {
            show_game_over_screen();
            if (start_pressed()) {
                // reset positions & speeds back to Verilog defaults
                cg_x   = 400;  REG_CG_X   = cg_x;
                lava_x = 600;  REG_LAVA_X = lava_x;
                speed_cg   = 1;
                speed_lava = 2;
                collision = false;
            }
        }
    }

    return 0;
}
