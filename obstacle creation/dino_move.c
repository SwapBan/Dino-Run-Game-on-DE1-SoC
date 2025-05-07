#define REPORT_LEN     8
#define LW_BRIDGE_BASE 0xFF200000
#define MAP_SIZE       0x1000

#define DINO_Y_OFFSET    0x04    // address==1
#define CG_X_OFFSET      0x3C    // address==15
#define CG_Y_OFFSET      0x40    // address==16
#define LAVA_X_OFFSET    0x44    // address==17
#define LAVA_Y_OFFSET    0x48    // address==18

#define GROUND_Y       120
#define GRAVITY          1
#define OBSTACLE_SPEED   1
#define SCREEN_WIDTH  640

int main() {
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    if(fd<0){ perror("open"); return 1; }
    void *lw = mmap(NULL, MAP_SIZE,
                    PROT_READ|PROT_WRITE, MAP_SHARED,
                    fd, LW_BRIDGE_BASE);
    if(lw==MAP_FAILED){ perror("mmap"); return 1; }

    // correctly cast to byte-offset
    volatile uint32_t *dino_y = (uint32_t *)((char*)lw + DINO_Y_OFFSET);
    volatile uint32_t *cg_x   = (uint32_t *)((char*)lw + CG_X_OFFSET);
    volatile uint32_t *cg_y   = (uint32_t *)((char*)lw + CG_Y_OFFSET);
    volatile uint32_t *lava_x = (uint32_t *)((char*)lw + LAVA_X_OFFSET);
    volatile uint32_t *lava_y = (uint32_t *)((char*)lw + LAVA_Y_OFFSET);

    // … set up joystick exactly as before …

    // initialize
    *cg_x   = SCREEN_WIDTH + 50;
    *cg_y   = /* whatever your ground‐y */ 248;
    *lava_x = SCREEN_WIDTH + 250;
    *lava_y = 248;

    int y = GROUND_Y, v = 0;
    while(1) {
      // 1) jump physics (unchanged) …
      // 2) shift obstacles:
      uint32_t x = *cg_x;
      x = (x > OBSTACLE_SPEED) ? x - OBSTACLE_SPEED : SCREEN_WIDTH;
      *cg_x = x;

      x = *lava_x;
      x = (x > OBSTACLE_SPEED) ? x - OBSTACLE_SPEED : SCREEN_WIDTH;
      *lava_x = x;

      // 3) small sleep to control speed
      usleep(10000);
    }

    // … cleanup …
}
