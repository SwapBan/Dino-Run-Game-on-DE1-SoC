/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 * Amanda Jenkins (alj2155); Swapnil Banerjee (sb5041)
 */

#include <stdio.h>
#include "vga_ball.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int vga_ball_fd;

void set_ball_position(int x, int y) {
  vga_ball_arg_t vla;
  vla.position.x = x;
  vla.position.y = y;
  if (ioctl(vga_ball_fd, VGA_BALL_WRITE_POSITION, &vla)) {
    perror("ioctl(VGA_BALL_WRITE_POSITION) failed");
  }
}

int main()
{
  static const char filename[] = "/dev/vga_ball";
  int x = 0;
  int y = 240; // Fixed vertical center
  int vx = 2;  // Move 2 pixels per frame

  if ((vga_ball_fd = open(filename, O_RDWR)) == -1) {
    perror("Could not open /dev/vga_ball");
    return -1;
  }

  printf("VGA ball Userspace program started (Sideways Movement Only)\n");

  while (1) {
    set_ball_position(x, y);

    x += vx;

    if (x >= 639 || x <= 0) {
      vx = -vx; // Bounce off walls
    }

    usleep(10000); // Delay ~10 ms for smooth animation
  }

  close(vga_ball_fd);
  printf("VGA ball Userspace program terminated\n");
  return 0;
}
