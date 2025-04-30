#include "usbkeyboard.h"        // your HID-over-USB helper :contentReference[oaicite:2]{index=2}&#8203;:contentReference[oaicite:3]{index=3}
#include <libusb-1.0/libusb.h>
#include <stdio.h>
#include <stdlib.h>

#define REPORT_LEN 8

int main(void) {
  struct libusb_device_handle *pad;
  uint8_t ep;
  unsigned char report[REPORT_LEN];
  int transferred;

  // open the DragonRise pad
  pad = openkeyboard(&ep);
  if (!pad) {
    fprintf(stderr, "Controller not found\n");
    return EXIT_FAILURE;
  }

  // loop printing raw 8-byte HID reports
  while (1) {
    int r = libusb_interrupt_transfer(pad, ep,
                                      report, REPORT_LEN,
                                      &transferred, 0);
    if (r == 0 && transferred == REPORT_LEN) {
      for (int i = 0; i < REPORT_LEN; ++i)
        printf("%02x ", report[i]);
      putchar('\n');
    }
  }

  // (not reached)
  libusb_close(pad);
  libusb_exit(NULL);
  return EXIT_SUCCESS;
}
