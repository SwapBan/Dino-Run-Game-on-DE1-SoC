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

    logic [15:0] dino_new_output, dino_left_output, dino_right_output;
    logic [15:0] dino_sprite_output, jump_sprite_output, duck_sprite_output;
    logic [15:0] godzilla_sprite_output, scac_sprite_output;

    logic [9:0] dino_sprite_addr, jump_sprite_addr, duck_sprite_addr;
    logic [9:0] godzilla_sprite_addr, scac_sprite_addr;

    logic [7:0] dino_x, dino_y;
    logic [7:0] jump_x, jump_y;
    logic [7:0] duck_x, duck_y;
    logic [7:0] s_cac_x, s_cac_y;
    logic [7:0] godzilla_x, godzilla_y;

    logic [7:0] a, b, c;
 // === SCORE overlay signals ===
    logic [3:0]  score;
    logic [7:0]  score_x, score_y;
    logic [5:0]  score_addr;
    logic        score_pixel;
    // Inline 8×8 font ROM
    logic [7:0] font_rom [0:9][0:7];
    initial begin
        font_rom[0] = '{8'h3C,8'h66,8'h6E,8'h7E,8'h76,8'h66,8'h3C,8'h00};
        font_rom[1] = '{8'h18,8'h38,8'h18,8'h18,8'h18,8'h18,8'h7E,8'h00};
        font_rom[2] = '{8'h3C,8'h66,8'h06,8'h1C,8'h30,8'h66,8'h7E,8'h00};
        font_rom[3] = '{8'h3C,8'h66,8'h06,8'h1C,8'h06,8'h66,8'h3C,8'h00};
        font_rom[4] = '{8'h0C,8'h1C,8'h2C,8'h4C,8'h7E,8'h0C,8'h0C,8'h00};
        font_rom[5] = '{8'h7E,8'h60,8'h7C,8'h06,8'h06,8'h66,8'h3C,8'h00};
        font_rom[6] = '{8'h3C,8'h66,8'h60,8'h7C,8'h66,8'h66,8'h3C,8'h00};
        font_rom[7] = '{8'h7E,8'h06,8'h0C,8'h18,8'h30,8'h30,8'h30,8'h00};
        font_rom[8] = '{8'h3C,8'h66,8'h66,8'h3C,8'h66,8'h66,8'h3C,8'h00};
        font_rom[9] = '{8'h3C,8'h66,8'h66,8'h3E,8'h06,8'h66,8'h3C,8'h00};
    end
    // === NEW: frame counter and sprite state ===
    logic [23:0] frame_counter;
    logic [1:0] sprite_state;

    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            frame_counter <= 0;
            sprite_state <= 0;
        end else begin
            if (frame_counter == 24'd5_000_000) begin
                sprite_state <= sprite_state + 1;
                frame_counter <= 0;
            end else begin
                frame_counter <= frame_counter + 1;
            end
        end
    end

    // === VGA TIMING COUNTERS ===
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

    // === SPRITE ROMS ===
    dino_sprite_rom dino_rom(.clk(clk), .address(dino_sprite_addr), .data(dino_new_output));
    dino_left_leg_up_rom dino_rom1(.clk(clk), .address(dino_sprite_addr), .data(dino_left_output));
    dino_right_leg_up_rom dino_rom2(.clk(clk), .address(dino_sprite_addr), .data(dino_right_output));

    dino_jump_rom jump_rom(.clk(clk), .address(jump_sprite_addr), .data(jump_sprite_output));
    dino_duck_rom duck_rom(.clk(clk), .address(duck_sprite_addr), .data(duck_sprite_output));
    dino_godzilla_rom godzilla_rom(.clk(clk), .address(godzilla_sprite_addr), .data(godzilla_sprite_output));
    dino_s_cac_rom s_cac_rom(.clk(clk), .address(scac_sprite_addr), .data(scac_sprite_output));

    // === CHOOSE CURRENT DINO SPRITE BASED ON STATE ===
    always_comb begin
        case (sprite_state)
            2'd0: dino_sprite_output = dino_new_output;
            2'd1: dino_sprite_output = dino_left_output;
            2'd2: dino_sprite_output = dino_right_output;
            default: dino_sprite_output = dino_new_output;
        endcase
    end

    // === SPRITE DRAWING + CONTROL LOGIC ===
    always_ff @(posedge clk) begin
        if (reset) begin
            dino_x <= 8'd100;   dino_y <= 8'd100;
            jump_x <= 8'd200;   jump_y <= 8'd150;
            duck_x <= 8'd300;   duck_y <= 8'd200;
            s_cac_x <= 8'd500;  s_cac_y <= 8'd100;
            godzilla_x <= 8'd100; godzilla_y <= 8'd260;
            // default score
            score   <= 4'd0;
            score_x <= 8'd0;
            score_y <= 8'd0;
            a <= 8'hFF; b <= 8'hFF; c <= 8'hFF;
        end else if (chipselect && write) begin
            case (address)
                9'd0: dino_x <= writedata[7:0];
                9'd1: dino_y <= writedata[7:0];
                9'd2: jump_x <= writedata[7:0];
                9'd3: jump_y <= writedata[7:0];
                9'd4: duck_x <= writedata[7:0];
                9'd5: duck_y <= writedata[7:0];
                9'd6: s_cac_x <= writedata[7:0];
                9'd7: s_cac_y <= writedata[7:0];
                9'd8: godzilla_x <= writedata[7:0];
                9'd9: godzilla_y <= writedata[7:0];
                9'd10: score   <= writedata[3:0];
                9'd11: score_x <= writedata[7:0];
                9'd12: score_y <= writedata[7:0];
            endcase
        end else if (VGA_BLANK_n) begin
            a <= 8'hFF; b <= 8'hFF; c <= 8'hFF;

            if (hcount >= dino_x && hcount < dino_x + 32 &&
                vcount >= dino_y && vcount < dino_y + 32) begin
                dino_sprite_addr <= (hcount - dino_x) + ((vcount - dino_y) * 32);
                a <= {dino_sprite_output[15:11], 3'b000};
                b <= {dino_sprite_output[10:5],  2'b00};
                c <= {dino_sprite_output[4:0],   3'b000};
            end

            if (hcount >= jump_x && hcount < jump_x + 32 &&
                vcount >= jump_y && vcount < jump_y + 32) begin
                jump_sprite_addr <= (hcount - jump_x) + ((vcount - jump_y) * 32);
                a <= {jump_sprite_output[15:11], 3'b000};
                b <= {jump_sprite_output[10:5],  2'b00};
                c <= {jump_sprite_output[4:0],   3'b000};
            end

            if (hcount >= duck_x && hcount < duck_x + 32 &&
                vcount >= duck_y && vcount < duck_y + 32) begin
                duck_sprite_addr <= (hcount - duck_x) + ((vcount - duck_y) * 32);
                a <= {duck_sprite_output[15:11], 3'b000};
                b <= {duck_sprite_output[10:5],  2'b00};
                c <= {duck_sprite_output[4:0],   3'b000};
            end

            if (hcount >= s_cac_x && hcount < s_cac_x + 32 &&
                vcount >= s_cac_y && vcount < s_cac_y + 32) begin
                scac_sprite_addr <= (hcount - s_cac_x) + ((vcount - s_cac_y) * 32);
                a <= {scac_sprite_output[15:11], 3'b000};
                b <= {scac_sprite_output[10:5],  2'b00};
                c <= {scac_sprite_output[4:0],   3'b000};
            end

            if (hcount >= godzilla_x && hcount < godzilla_x + 32 &&
                vcount >= godzilla_y && vcount < godzilla_y + 32) begin
                godzilla_sprite_addr <= (hcount - godzilla_x) + ((vcount - godzilla_y) * 32);
                a <= {godzilla_sprite_output[15:11], 3'b000};
                b <= {godzilla_sprite_output[10:5],  2'b00};
                c <= {godzilla_sprite_output[4:0],   3'b000};
            end
            // score overlay
                        // score overlay (inline lookup, no extra regs)
            if (hcount >= score_x && hcount < score_x + 8 &&
                vcount >= score_y && vcount < score_y + 8) begin
                // row = vcount−score_y, col = hcount−score_x
               // if (font_rom[score][vcount - score_y][7 - (hcount - score_x)]) begin
                    a <= 8'h00;
                    b <= 8'h00;
                    c <= 8'h00;
                //end
            end

        end
    end

    assign {VGA_R, VGA_G, VGA_B} = {a, b, c};

endmodule

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
