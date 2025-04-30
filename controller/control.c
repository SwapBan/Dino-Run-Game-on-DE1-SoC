#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>

// Simulating the values from NES controller input (can be replaced with actual USB HID read)
uint8_t controller_input[] = { 0x01, 0x7f, 0x7f, 0x7f, 0x7f, 0x0f, 0x00, 0x00 }; // Example for "Nothing"

// FPGA memory-mapped registers for dino position
volatile uint32_t *dino_x_reg = (uint32_t *)0x40000000;  // Address of dino_x register in FPGA memory
volatile uint32_t *dino_y_reg = (uint32_t *)0x40000004;  // Address of dino_y register in FPGA memory

// Function to update dino's position
void update_dino_position(int dino_x, int dino_y) {
    *dino_x_reg = (uint32_t)dino_x;
    *dino_y_reg = (uint32_t)dino_y;
}

// Function to read the NES controller input (simulated here)
void read_controller_input(uint8_t *input) {
    // In a real application, this should read data from the NES controller through USB
    // For this example, we're simulating the "Down" button press (second sequence)
    input[4] = 0xFF;  // Simulating "Down" press (Bit for Down set to 1)
}

int main() {
    int dino_x = 100;  // Initial position of the Dino
    int dino_y = 100;  // Initial position of the Dino

    uint8_t controller_input[8];  // Array to hold the controller input values

    // Main game loop
    while (1) {
        read_controller_input(controller_input);  // Read NES controller input

        // Interpret controller input and update the Dino's position
        if (controller_input[4] == 0xFF) {  // Down button pressed
            dino_y++;
        } else if (controller_input[2] == 0x00) {  // Up button pressed
            dino_y--;
        } else if (controller_input[3] == 0x7F) {  // Left button pressed
            dino_x--;
        } else if (controller_input[5] == 0xFF) {  // Right button pressed
            dino_x++;
        }

        // Update the Dino's position in the FPGA's memory-mapped registers
        update_dino_position(dino_x, dino_y);

        // Print the new position for debugging
        printf("Dino Position: (%d, %d)\n", dino_x, dino_y);

        usleep(50000);  // Simulate game loop with a 50ms delay
    }

    return 0;
}

