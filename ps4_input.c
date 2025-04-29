#include <stdio.h>
#include <stdlib.h>
#include <libusb-1.0/libusb.h>

#define SONY_VENDOR_ID  0x054C
#define DUALSHOCK4_PRODUCT_ID 0x05C4  // Try 0x09CC if this one fails

int main() {
    libusb_device_handle *handle;
    int r;

    r = libusb_init(NULL);
    if (r < 0) {
        perror("libusb init failed");
        return 1;
    }

    handle = libusb_open_device_with_vid_pid(NULL, SONY_VENDOR_ID, DUALSHOCK4_PRODUCT_ID);
    if (!handle) {
        printf("DualShock 4 not found.\n");
        libusb_exit(NULL);
        return 1;
    }

    printf("DualShock 4 detected!\n");

    // Detach kernel driver if necessary
    if (libusb_kernel_driver_active(handle, 0) == 1) {
        libusb_detach_kernel_driver(handle, 0);
    }

    r = libusb_claim_interface(handle, 0);
    if (r != 0) {
        printf("Failed to claim interface: %d\n", r);
        libusb_close(handle);
        libusb_exit(NULL);
        return 1;
    }

    printf("Reading input...\n");

    unsigned char data[64];
    int actual_length;

    while (1) {
        r = libusb_interrupt_transfer(handle, 0x81, data, sizeof(data), &actual_length, 0);
        if (r == 0 && actual_length > 0) {
            printf("Raw report: ");
            for (int i = 0; i < actual_length; i++) {
                printf("%02X ", data[i]);
            }
            printf("\n");

            // Example: data[6] = left stick Y axis (byte positions may vary!)
            // Use these to detect jump/duck later
        }
        usleep(5000);
    }

    libusb_release_interface(handle, 0);
    libusb_close(handle);
    libusb_exit(NULL);
    return 0;
}
