#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define LW_BRIDGE_BASE 0xFF200000
#define AUDIO_BASE     0x00008800
#define MAP_SIZE       0x1000

#define AUDIO_CTRL     0x00  // Control Register (W)
#define AUDIO_STATUS   0x01  // Status Register  (R)
#define AUDIO_LEFT     0x02  // Left FIFO        (W)
#define AUDIO_RIGHT    0x03  // Right FIFO       (W)

int main() {
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) { perror("open"); return 1; }

    void *lw_virtual = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, LW_BRIDGE_BASE);
    if (lw_virtual == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    // Get pointer to audio base
    volatile uint32_t *audio = (volatile uint32_t *)((char *)lw_virtual + AUDIO_BASE);

    // Enable both left/right channels
    audio[AUDIO_CTRL] = 0b11;

    // Generate a 250 Hz square wave tone for ~8000 cycles
    for (int i = 0; i < 8000; i++) {
        int16_t sample = (i / 100) % 2 ? 0x4000 : -0x4000;

        // Wait until space in FIFO
        while ((audio[AUDIO_STATUS] & 0xFF) < 0xF);

        audio[AUDIO_LEFT] = (uint32_t)(sample << 16);  // write 16-bit left channel
        audio[AUDIO_RIGHT] = (uint32_t)(sample << 16); // write 16-bit right channel

        usleep(50
