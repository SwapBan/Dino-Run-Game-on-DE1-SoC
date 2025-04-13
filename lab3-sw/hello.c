/*
 * Userspace program that communicates with the vga_ball device driver
 * through ioctls
 * Stephen A. Edwards
 * Columbia University
 * 
 * Amanda Jenkins (alj2155); Swapnil Banerjee(sb5041)
 */

 #include <stdio.h>
 #include "vga_ball.h"
 #include <sys/ioctl.h>
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <string.h>
 #include <unistd.h>
 
 int vga_ball_fd;
 
 /* Read and print the background color */
 void print_background_color() {
   vga_ball_arg_t vla;
   
   if (ioctl(vga_ball_fd, VGA_BALL_READ_BACKGROUND, &vla)) {
       perror("ioctl(VGA_BALL_READ_BACKGROUND) failed");
       return;
   }
   printf("%02x %02x %02x\n",
    vla.background.red, vla.background.green, vla.background.blue);
 }
 
 /* Set the background color */
 void set_background_color(const vga_ball_color_t *c)
 {
   vga_ball_arg_t vla;
   vla.background = *c;
   if (ioctl(vga_ball_fd, VGA_BALL_WRITE_BACKGROUND, &vla)) {
       perror("ioctl(VGA_BALL_SET_BACKGROUND) failed");
       return;
   }
 }
 
 int main()
 {
   vga_ball_arg_t vla;
   int i;
   static const char filename[] = "/dev/vga_ball";
 
   int x = 300;
   int y = 300;
 
   int vx = 1;
   int vy = 1;
   int r = 16;
   static vga_ball_color_t colors[] = {
    { 0xff, 0xb3, 0xba, 0x9f, 0x00, 0x9f, 0x00, 0x01 }, /* Pastel Pink */
    { 0xb3, 0xff, 0xb3, 0x9f, 0x00, 0x9f, 0x00, 0x01 }, /* Pastel Green */
    { 0xb3, 0xd1, 0xff, 0x9f, 0x00, 0x9f, 0x00, 0x01 }, /* Pastel Blue */
    { 0xff, 0xf7, 0xb3, 0x9f, 0x00, 0x9f, 0x00, 0x01 }, /* Pastel Yellow */
    { 0xd1, 0xb3, 0xff, 0x9f, 0x00, 0x9f, 0x00, 0x01 }, /* Pastel Purple */
    { 0xff, 0xc2, 0x99, 0x9f, 0x00, 0x9f, 0x00, 0x01 }, /* Peach */
    { 0xcc, 0xff, 0xff, 0x9f, 0x00, 0x9f, 0x00, 0x01 }, /* Light Cyan */
    { 0xff, 0xff, 0xff, 0x9f, 0x00, 0x9f, 0x00, 0x01 }, /* White */
    { 0xe6, 0xe6, 0xfa, 0x9f, 0x00, 0x9f, 0x00, 0x01 }  /* Lavender */
  };
  
 
 # define COLORS 9
 
   printf("VGA ball Userspace program started\n");
 
   if ( (vga_ball_fd = open(filename, O_RDWR)) == -1) {
     fprintf(stderr, "could not open %s\n", filename);
     return -1;
   }
 
   printf("initial state: ");
   print_background_color();
 
   while(1){
     colors[i%COLORS].x_down = x;
     colors[i%COLORS].x_up = (x >> 5);
     colors[i%COLORS].y_down = y;
     colors[i%COLORS].y_up = (y >> 5);
     colors[i%COLORS].red = r;

     x+=vx;
     y+=vy;
     if(x-r <= 0 || x+r >= 639){
       vx = -vx;
       if(r>16) r--;
     }
     if(y+r >= 479 || y-r <= 0){
       vy = -vy;
       if(r>16) r--;
     } 
 
     set_background_color(&colors[i % COLORS ]);
     usleep(10000);
   }
   printf("VGA BALL Userspace program terminating\n");
   return 0;
 }
 
