
module vga_dino_simple(
    input logic clk,
    input logic reset,

    output logic [7:0] VGA_R, VGA_G, VGA_B,
    output logic       VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n
);

    logic [10:0] hcount;
    logic [9:0]  vcount;
    logic [15:0] sprite_pixel;
    logic [7:0]  r, g, b;

    // Position of dino image
    parameter DINO_X = 100;
    parameter DINO_Y = 100;

    // Sprite memory
    logic [7:0] sprite_addr;
    dino_rom sprite (
        .address(sprite_addr),
        .clock(clk),
        .q(sprite_pixel)
    );

    // VGA controller
    vga_counters counters(
        .clk50(clk), .reset(reset),
        .hcount(hcount), .vcount(vcount),
        .VGA_CLK(VGA_CLK), .VGA_HS(VGA_HS),
        .VGA_VS(VGA_VS), .VGA_BLANK_n(VGA_BLANK_n),
        .VGA_SYNC_n(VGA_SYNC_n)
    );

    always_ff @(posedge clk) begin
        if (VGA_BLANK_n) begin
            if (hcount >= DINO_X && hcount < DINO_X + 16 &&
                vcount >= DINO_Y && vcount < DINO_Y + 16) begin
                sprite_addr <= (vcount - DINO_Y) * 16 + (hcount - DINO_X);
                r <= {sprite_pixel[15:11], 3'b000};
                g <= {sprite_pixel[10:5],  2'b00};
                b <= {sprite_pixel[4:0],   3'b000};
            end else begin
                r <= 8'hFF; g <= 8'hFF; b <= 8'hFF;  // White background
            end
        end else begin
            r <= 0; g <= 0; b <= 0;
        end
    end

    assign VGA_R = r;
    assign VGA_G = g;
    assign VGA_B = b;

endmodule
