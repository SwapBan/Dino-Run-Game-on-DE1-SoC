/*
 * Userspace program that communicates with the vga_dino device driver
 * through ioctls
 *
 * Stephen A. Edwards
 * Columbia University
 */

#include <stdio.h>
#include "vga_dino.h"
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int vga_dino_fd;

/* Read and print the background color */
void print_background_color() {
  vga_dino_arg_t vla;
  
  if (ioctl(vga_dino_fd, VGA_DINO_READ_BACKGROUND, &vla)) {
      perror("ioctl(VGA_DINO_READ_BACKGROUND) failed");
      return;
  }
  printf("%02x %02x %02x\n",
	 vla.background.red, vla.background.green, vla.background.blue);
}

/* Set the background color */
void set_background_color(const vga_dino_color_t *c)
{
  vga_dino_arg_t vla;
  vla.background = *c;
  if (ioctl(vga_dino_fd, VGA_DINO_WRITE_BACKGROUND, &vla)) {
      perror("ioctl(VGA_DINO_SET_BACKGROUND) failed");
      return;
  }
}

void set_pos(const vga_dino_pos_t *pos)
b{
  vga_dino_pos_t bpos;
  bpos.xcoor = pos-> xcoor;
  bpos.ycoor = pos-> ycoor;

	       printf("Setting position to x: %d, y: %d\n", bpos.xcoor, bpos.ycoor);	
    if (ioctl(vga_dino_fd, VGA_dino_WRITE_POS, &bpos)) {
      perror("ioctl(VGA_DINO_WRITE_POS) failed");
      return;
  }
}

int main()
{
  vga_dino_color_t default_color ={0x00, 0x00, 0x00};
  
  vga_dino_arg_t vla;
  int i;
  static const char filename[] = "/dev/vga_dino";

  static const vga_dino_color_t colors[] = {
    { 0xff, 0x00, 0x00 }, /* Red */
    { 0x00, 0xff, 0x00 }, /* Green */
    { 0x00, 0x00, 0xff }, /* Blue */
    { 0xff, 0xff, 0x00 }, /* Yellow */
    { 0x00, 0xff, 0xff }, /* Cyan */
    { 0xff, 0x00, 0xff }, /* Magenta */
    { 0x80, 0x80, 0x80 }, /* Gray */
    { 0x00, 0x00, 0x00 }, /* Black */
    { 0xff, 0xff, 0xff }  /* White */
  };

# define COLORS 9

  printf("VGA dino Userspace program started\n");

  if ( (vga_dino_fd = open(filename, O_RDWR)) == -1) {
    fprintf(stderr, "could not open %s\n", filename);
    return -1;
  }

  printf("initial state: ");

  print_background_color();

for (i = 0 ; i < 9 ; i++) {
    set_background_color(&colors[i % COLORS ]);
    print_background_color();
    usleep(400000);
  }

  set_background_color(&default_color);
  set_pos(&default_color);

	int x = 320;
	int y = 240;
	while(1)
	{	
	        if(x < 630 && y < 465)		
		{			
			while(x < 630 && y <= 465)
			{
				x++;
				y++;
				vga_dino_pos_t default_pos = {x, y};
				set_pos(&default_pos);
				usleep(10000);
				  set_background_color(&default_color);
			}
		}
		if(x < 630 && y > 465)
		{
			while(x <= 630)
			{
				x++;
				y--;
				vga_dino_pos_t default_pos = {x, y};
				set_pos(&default_pos);
				usleep(10000);
			}
		}
		printf("Did you reach\n");
		if(x > 630 && y > 12)
		{
			while(y >= 12)
			{
				x--;
				y--;
				vga_dino_pos_t default_pos = {x, y};
				set_pos(&default_pos);
				usleep(10000);
			}
		}
		if(x > 10 && y < 465)
		{
			while(x >= 10 && y <= 468)
			{
				x--;
				y++;
				vga_dino_pos_t default_pos = {x, y};
				set_pos(&default_pos);
				usleep(10000);
			}
		}
		if(x > 10 &&  y >465 )
		{
			while(x > 10)
			{
				x--;
				y--;
				vga_dino_pos_t default_pos = {x, y};
				set_pos(&default_pos);
				usleep(10000);
			}
		}
		if(x >= 10 &&  y > 12 )
		{
			while(y > 12)
			{
				x++;
				y--;
				vga_dino_pos_t default_pos = {x, y};
				set_pos(&default_pos);
				usleep(10000);
			}
		}
				
	}




 

/*  for (i = 0 ; i < 24 ; i++) {
    set_background_color(&colors[i % COLORS ]);
    print_background_color();
    usleep(400000);
  }*/
  
  printf("VGA DINO Userspace program terminating\n");
  return 0;
}
