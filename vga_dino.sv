/*
 * Avalon memory-mapped peripheral that generates VGA
 *
 * Stephen A. Edwards
 * Columbia University
 *
 * Register map:
 * 
 * Byte Offset  32 ... 0   Meaning
 *        0    | XCOOR |  X-Coordinate of the center of the dino
 *        1    | YCOOR |  Y-Coordinate of the center of the dino
 *        2    |  Red  |  Red component of background color (0-255)
 *        3    | Green |  Green component
 *        4    | Blue  |  Blue component
 */

module vga_dino(
    input logic        clk,
    input logic        reset,
    input logic [31:0] writedata,
    input logic        write,
    input              chipselect,
    input logic [2:0]  address,

    output logic [7:0] VGA_R, VGA_G, VGA_B,
    output logic       VGA_CLK, VGA_HS, VGA_VS,
    output logic       VGA_BLANK_n,
    output logic       VGA_SYNC_n
);

    logic [10:0] hcount;
    logic [9:0]  vcount;

    // Dino position registers
    logic [9:0]  dino_x, dino_y; // Top-left corner of dino sprite
    logic [7:0]  background_r, background_g, background_b;

    // Simple 16x16 monochrome dino sprite ROM (1 = dino pixel, 0 = transparent)
    logic [15:0] dino_sprite [15:0];

    initial begin
        // Pattern for dino
        dino_sprite[0]  = 16'b0000001111000000;
        dino_sprite[1]  = 16'b0000011111100000;
        dino_sprite[2]  = 16'b0000111111110000;
        dino_sprite[3]  = 16'b0001111111111000;
        dino_sprite[4]  = 16'b0011111111111100;
        dino_sprite[5]  = 16'b0011111111111100;
        dino_sprite[6]  = 16'b0011111111111100;
        dino_sprite[7]  = 16'b0001111111111000;
        dino_sprite[8]  = 16'b0000111111110000;
        dino_sprite[9]  = 16'b0000011111100000;
        dino_sprite[10] = 16'b0000001111000000;
        dino_sprite[11] = 16'b0000000110000000;
        dino_sprite[12] = 16'b0000000110000000;
        dino_sprite[13] = 16'b0000000110000000;
        dino_sprite[14] = 16'b0000000110000000;
        dino_sprite[15] = 16'b0000000110000000;
    end

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

    // Register map: 0 = dino_x, 1 = dino_y, 2 = background_r, 3 = background_g, 4 = background_b
    always_ff @(posedge clk) begin
        if (reset) begin
            background_r <= 8'h00;
            background_g <= 8'h00;
            background_b <= 8'h00;
            dino_x <= 10'd100;
            dino_y <= 10'd360;
        end else if (chipselect && write) begin
            case (address)
                3'h0: dino_x <= writedata[9:0];
                3'h1: dino_y <= writedata[9:0];
                3'h2: background_r <= writedata[7:0];
                3'h3: background_g <= writedata[7:0];
                3'h4: background_b <= writedata[7:0];
            endcase
        end
    end

    always_comb begin
        // Default background color
        VGA_R = background_r;
        VGA_G = background_g;
        VGA_B = background_b;

        // Only draw within visible area
        if (VGA_BLANK_n) begin
            // Check if current pixel is within dino sprite area
            if ((hcount[10:1] >= dino_x) && (hcount[10:1] < dino_x + 16) &&
                (vcount >= dino_y) && (vcount < dino_y + 16)) begin
                // Sprite pixel coordinates
                logic [3:0] sprite_x = hcount[10:1] - dino_x;
                logic [3:0] sprite_y = vcount - dino_y;
                if (dino_sprite[sprite_y][15 - sprite_x]) begin
                    // Dino color: green (customize as needed)
                    VGA_R = 8'h00;
                    VGA_G = 8'hFF;
                    VGA_B = 8'h00;
                end
            end
        end
    end

endmodule
