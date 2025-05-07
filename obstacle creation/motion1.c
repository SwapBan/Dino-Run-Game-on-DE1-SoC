#include <stdint.h>
#include <stdbool.h>

// ------------------------------------------------------------------
//  PIO register map (word-aligned) 
//  Matches your Verilog‘s `case (address)`:
//    0→dino_x, 1→dino_y, …, 15→cg_x, 16→cg_y, 17→lava_x, 18→lava_y
// ------------------------------------------------------------------
#define PIO_BASE     0xFF200000UL
#define REG_DINO_X   (*(volatile uint32_t*)(PIO_BASE +  0*4))
#define REG_DINO_Y   (*(volatile uint32_t*)(PIO_BASE +  1*4))
#define REG_CG_X     (*(volatile uint32_t*)(PIO_BASE + 15*4))
#define REG_CG_Y     (*(volatile uint32_t*)(PIO_BASE + 16*4))
#define REG_LAVA_X   (*(volatile uint32_t*)(PIO_BASE + 17*4))
#define REG_LAVA_Y   (*(volatile uint32_t*)(PIO_BASE + 18*4))

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

// Simple AABB overlap test
static bool overlap(int ax,int ay,int aw,int ah,
                    int bx,int by,int bw,int bh) {
    return !( ax+aw <= bx ||
              bx+bw <= ax ||
              ay+ah <= by ||
              by+bh <= ay );
}

// ------------------------------------------------------------------
//  Platform-specific stubs – replace these with your actual impl.
// ------------------------------------------------------------------
void wait_for_vsync(void);         // block until next VGA frame
void read_controller(void);        // updates global dino_x, dino_y
bool start_pressed(void);          // true when “Start” is pressed
void show_game_over_screen(void);  // draw your Game Over prompt

int main(void) {
    // 1) Initialize from PIO (must match your Verilog reset values)
    int dino_x   = REG_DINO_X;   // driven by your controller code
    int dino_y   = REG_DINO_Y;
    int cg_x     = REG_CG_X;     // defaults to 400
    int cg_y     = REG_CG_Y;     //             248
    int lava_x   = REG_LAVA_X;   //             600
    int lava_y   = REG_LAVA_Y;   //             248

    // 2) Speeds
    int speed_cg   = 1;
    int speed_lava = 2;

    bool collision = false;

    while (1) {
        // --- per-frame sync & input ---
        wait_for_vsync();
        read_controller();
        REG_DINO_X = dino_x;
        REG_DINO_Y = dino_y;

        // --- obstacle movement & wrap & accel ---
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

        // --- collision detection ---
        if (!collision &&
            ( overlap(dino_x,dino_y,DINO_W,DINO_H,
                      cg_x,   cg_y,   CG_W,  CG_H)   ||
              overlap(dino_x,dino_y,DINO_W,DINO_H,
                      lava_x, lava_y, LAVA_W,LAVA_H) ))
        {
            collision = true;
        }

        // --- game-over & restart ---
        if (collision) {
            show_game_over_screen();
            if (start_pressed()) {
                // reset positions & speeds
                cg_x       = 400;   REG_CG_X   = cg_x;
                lava_x     = 600;   REG_LAVA_X = lava_x;
                speed_cg   = 1;
                speed_lava = 2;
                collision  = false;
            }
        }
    }

    return 0;
}
