#include "fpga_pio.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// Adjust if your PIO base is different in Qsys
#define PIO_BASE 0xFF200000
#define PIO_SPAN 0x1000

static volatile uint8_t *pio_mem;

int pio_open(void) {
    int fd = open("/dev/mem", O_RDWR|O_SYNC);
    if (fd < 0) return -1;
    pio_mem = mmap(NULL, PIO_SPAN, PROT_READ|PROT_WRITE, MAP_SHARED, fd, PIO_BASE);
    close(fd);
    return (pio_mem==(void*)-1) ? -1 : 0;
}

void pio_write(uint32_t offset, uint32_t value) {
    *((volatile uint32_t*)(pio_mem + offset)) = value;
}
