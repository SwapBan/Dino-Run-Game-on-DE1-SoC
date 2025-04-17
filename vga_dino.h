#ifndef _VGA_DINO_H
#define _VGA_DINO_H

#include <linux/ioctl.h>

typedef struct {
  unsigned char red, green, blue;
} vga_dino_color_t;
  
typedef struct {
  int xcoor, ycoor;
} vga_dino_pos_t;
  

typedef struct {
  vga_dino_color_t background;
} vga_dino_arg_t;

#define VGA_DINO_MAGIC 'q'

/* ioctls and their arguments */
#define VGA_DINO_WRITE_BACKGROUND _IOW(VGA_DINO_MAGIC, 1, vga_dino_arg_t)
#define VGA_DINO_READ_BACKGROUND  _IOR(VGA_DINO_MAGIC, 2, vga_dino_arg_t)
#define VGA_DINO_WRITE_POS _IOR(VGA_DINO_MAGIC, 3, vga_dino_arg_t)
#define VGA_DINO_READ_POS _IOR(VGA_DINO_MAGIC, 4, vga_dino_arg_t)

#endif
