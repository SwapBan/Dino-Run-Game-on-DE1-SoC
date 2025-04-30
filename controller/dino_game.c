#include <stdio.h>
#include <stdint.h>
#include <libusb-1.0/libusb.h>
#include <unistd.h>

// Define the report array to store the controller input data
uint8_t report[8];  // 8-byte HID report from the controller

// Define Dino's position
int dino_x = 100, dino_y = 100;  // Initial position

// USB device information
#define VENDOR_ID 0x16C0
#define PRODUCT_ID 0x05DC
#define TIMEOUT 1000  // Timeout for reading the report (in milliseconds)

// Define VGA parameters
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 480

// VGA Colors
#define SKY_COLOR 0x87CEEB // Sky Blue

// Function to read the NES controller data
void read_controller_data(struct usb_device_handle *dev_handle) {
    int ret;

    // Read the HID report (assuming the report length is 8 bytes)
    ret = usb_interrupt_read(dev_handle, 0x81, (char *)report, 8, TIMEOUT);
    if (ret < 0) {
        fprintf(stderr, "Failed to read HID report: %d\n", ret);
    } else {
        // Successfully read the report, process it
        printf("Controller Report: ");
        for (int i = 0; i < 8; i++) {
            printf("%02X ", report[i]);
        }
        printf("\n");

        // Process the report and update dino's position
        if (report[4] == 0xFF) {
            // Down
            dino_y += 5;  // Move down
        }
        if (report[3] == 0x00) {
            // Up
            dino_y -= 5;  // Move up
        }
        if (report[2] == 0x7F) {
            // Left
            dino_x -= 5;  // Move left
        }
        if (report[2] == 0xFF) {
            // Right
            dino_x += 5;  // Move right
        }
    }
}

// Function to initialize libusb and open the NES controller device
int initialize_usb_device() {
    struct usb_device_handle *dev_handle;
    struct usb_bus *bus;
    struct usb_device *dev;

    libusb_init(NULL);

    // Find the device with specified Vendor and Product ID
    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (dev = bus->devices; dev; dev = dev->next) {
            if (dev->descriptor.idVendor == VENDOR_ID && dev->descriptor.idProduct == PRODUCT_ID) {
                // Open the device
                dev_handle = usb_open(dev);
                if (dev_handle == NULL) {
                    fprintf(stderr, "Unable to open USB device\n");
                    libusb_exit(NULL);
                    return -1;
                }

                // Claim interface 0 (assuming the NES controller uses interface 0)
                if (usb_claim_interface(dev_handle, 0) < 0) {
                    fprintf(stderr, "Failed to claim interface\n");
                    usb_close(dev_handle);
                    libusb_exit(NULL);
                    return -1;
                }

                return (int)dev_handle;
            }
        }
    }

    fprintf(stderr, "Device not found\n");
    libusb_exit(NULL);
    return -1;
}

// Function to release the USB device
void release_usb_device(struct usb_device_handle *dev_handle) {
    usb_release_interface(dev_handle, 0);
    usb_close(dev_handle);
    libusb_exit(NULL);
}

// Function to update the VGA display based on Dino's position
void update_vga_display() {
    // Here, you would integrate the VGA code to move the Dino based on its position (dino_x, dino_y)
    // For example, drawing the Dino at (dino_x, dino_y) coordinates
    printf("Dino position: (%d, %d)\n", dino_x, dino_y);
}

// Function to draw the background (Sky and Ground)
void draw_background() {
    // Set the background color to sky blue
    // This is a placeholder; you should implement the actual VGA drawing logic here
    printf("Drawing background with Sky color: %06X\n", SKY_COLOR);
}

// Main function to handle the game loop
int main() {
    struct usb_device_handle *dev_handle;

    // Initialize USB device and open NES controller
    dev_handle = initialize_usb_device();
    if (dev_handle == -1) {
        return 1;
    }

    // Main game loop
    while (1) {
        // Read the controller data to update Dino's position
        read_controller_data(dev_handle);

        // Update the VGA display with the new Dino position
        update_vga_display();

        // Draw the background (Sky and Ground)
        draw_background();

        // Delay to slow down the loop (e.g., to match the game frame rate)
        usleep(50000);  // 50ms delay (adjust as needed)
    }

    // Release USB device
    release_usb_device(dev_handle);
    return 0;
}
