#ifndef FPGA_PIO_H
#define FPGA_PIO_H
#include <stdint.h>

// Opens /dev/mem and mmaps the PIO region. Returns 0 on success.
int  pio_open(void);

// Writes a 32-bit value to (base_address + offset).
void pio_write(uint32_t offset, uint32_t value);
#endif
