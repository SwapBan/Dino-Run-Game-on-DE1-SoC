/*
 * VGA module with Avalon-MM interface and Dino sprite display
 * Author: Swapnil
 */

module vga_ball(
    input logic         clk,
    input logic         reset,
    input logic [31:0]  writedata,
    input logic         write,
    input               chipselect,
    input logic [8:0]   address,

    output logic [7:0]  VGA_R, VGA_G, VGA_B,
    output logic        VGA_CLK, VGA_HS, VGA_VS,
                        VGA_BLANK_n,
    output logic        VGA_SYNC_n
);

   logic [10:0] hcount;
   logic [9:0]  vcount;

   // Dino sprite ROM
   logic [15:0] dino_sprite_output;
   logic [9:0]  dino_sprite_addr;

   // VGA color output
   logic [7:0] a, b, c;

   // Dino position controlled through Avalon-MM
   logic [7:0] dino_x;
   logic [7:0] dino_y;

   // Instantiate counters
   vga_counters counters(
       .clk50(clk),
       .reset(reset),
       .hcount(hcount),
       .vcount(vcount),
       .VGA_CLK(VGA_CLK),
       .VGA_HS(VGA_HS),
       .VGA_VS(VGA_VS),
       .VGA_BLANK_n(VGA_BLANK_n),
       .VGA_SYNC_n(VGA_SYNC_n)
   );

   // Instantiate Dino ROM (make sure this points to your MIF-backed ROM)
   dino_sprite_rom dino_rom(
       .clk(clk),
       .address(dino_sprite_addr),
       .data(dino_sprite_output)
   );

   always_ff @(posedge clk) begin
      if (reset) begin
         dino_x <= 8'd100;
         dino_y <= 8'd100;
         a <= 8'hFF;
         b <= 8'hFF;
         c <= 8'hFF;
      end else if (chipselect && write) begin
         case (address)
            9'd0: dino_x <= writedata[7:0];
            9'd1: dino_y <= writedata[7:0];
         endcase
      end else if (VGA_BLANK_n) begin
         if (hcount >= dino_x && hcount < dino_x + 32 && vcount >= dino_y && vcount < dino_y + 32) begin
            dino_sprite_addr <= (hcount - dino_x) + ((vcount - dino_y) * 32);
            a <= {dino_sprite_output[15:11], 3'b000};
            b <= {dino_sprite_output[10:5],  2'b00};
            c <= {dino_sprite_output[4:0],   3'b000};
         end else begin
            a <= 8'hFF;
            b <= 8'hFF;
            c <= 8'hFF;
         end
      end
   end

   assign {VGA_R, VGA_G, VGA_B} = {a, b, c};

endmodule

// Simple VGA timing generator
module vga_counters(
    input  logic        clk50, reset,
    output logic [10:0] hcount,
    output logic [9:0]  vcount,
    output logic        VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n
);

   parameter HACTIVE = 11'd1280,
             HFRONT = 11'd32,
             HSYNC  = 11'd192,
             HBACK  = 11'd96,
             HTOTAL = HACTIVE + HFRONT + HSYNC + HBACK;

   parameter VACTIVE = 10'd480,
             VFRONT = 10'd10,
             VSYNC  = 10'd2,
             VBACK  = 10'd33,
             VTOTAL = VACTIVE + VFRONT + VSYNC + VBACK;

   logic endOfLine;
   always_ff @(posedge clk50 or posedge reset)
      if (reset)
         hcount <= 0;
      else if (endOfLine)
         hcount <= 0;
      else
         hcount <= hcount + 1;

   assign endOfLine = (hcount == HTOTAL - 1);

   logic endOfField;
   always_ff @(posedge clk50 or posedge reset)
      if (reset)
         vcount <= 0;
      else if (endOfLine)
         if (endOfField)
            vcount <= 0;
         else
            vcount <= vcount + 1;

   assign endOfField = (vcount == VTOTAL - 1);

   assign VGA_HS = !((hcount >= (HACTIVE + HFRONT)) && (hcount < (HACTIVE + HFRONT + HSYNC)));
   assign VGA_VS = !((vcount >= (VACTIVE + VFRONT)) && (vcount < (VACTIVE + VFRONT + VSYNC)));
   assign VGA_SYNC_n = 1'b0;
   assign VGA_BLANK_n = (hcount < HACTIVE) && (vcount < VACTIVE);
   assign VGA_CLK = hcount[0];

endmodule
