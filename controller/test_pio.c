#include <stdio.h>
#include "fpga_pio.h"

int main() {
    if (pio_open()) {
        printf("PIO mapping failed\n");
        return 1;
    }
    pio_write(0, 123);   // write 123 to offset 0
    printf("PIO write succeeded\n");
    return 0;
}
