// === Dino Run: Full Game Logic with Obstacles, Collision, Replay ===
// Includes lava, small cactus, group cactus, and animated pterodactyl
module vga_ball(
    input  logic        clk,
    input  logic        reset,
    input  logic [7:0]  controller_report,

    output logic [7:0]  VGA_R, VGA_G, VGA_B,
    output logic        VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n
);

    // VGA timing constants
    localparam HACTIVE = 11'd1280;
    localparam SCORE_X = 50;
localparam SCORE_Y = 10;

    // VGA TIMING
    logic [10:0] hcount;
    logic [9:0]  vcount;
      // === Score (0–999) ===
    logic [9:0]  score;
    logic [3:0]  digit_h, digit_t, digit_u;
    logic [7:0]  font_rom [0:9][0:7];

    initial begin
        // simple 8×8 font for digits 0–9
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


    logic [7:0]  a, b, c;

    // === Dino ===
    logic [15:0] dino_sprite_output;
    logic [15:0] dino_new_output;
    logic [9:0]  dino_sprite_addr;
    logic [10:0] dino_x = 100, dino_y = 348;

    // === Obstacles ===
    logic [10:0] s_cac_x = 1200, s_cac_y = 248;
    logic [10:0] group_x = 1600, group_y = 248;
    logic [10:0] lava_x   = 1800, lava_y   = 248;
    logic [10:0] ptr_x    = 1400, ptr_y    = 200;

    logic [15:0] scac_sprite_output, group_output, lava_output;
    logic [15:0] ptr_up_output, ptr_down_output, ptr_sprite_output;
    logic [9:0]  scac_sprite_addr, lava_sprite_addr, ptr_sprite_addr;
    logic [12:0] group_addr;

    // === Replay ===
    logic [15:0] replay_output;
    logic [9:0]  replay_addr;
    logic [10:0] replay_x = 560, replay_y = 200;

    // === Motion ===
    logic [23:0] motion_timer;
    logic [10:0] obstacle_speed = 1;
    logic [4:0]  passed_count;
    logic        game_over;
    logic [1:0]  sprite_state;

    // === Random seed (6-bit LFSR) ===
    logic [5:0] lfsr;
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            lfsr <= 6'b101011; // non‐zero seed
        end else if (!game_over && motion_timer >= 24'd2_000_000) begin
            // x^6 + x^5 + 1 polynomial
            lfsr <= { lfsr[4:0], lfsr[5] ^ lfsr[4] };
        end
    end

    function automatic logic is_visible(input logic [15:0] px);
        return (px != 16'hF81F && px != 16'hFFFF);
    endfunction

    function automatic logic collide(
        input logic [10:0] ax, ay, bx, by,
        input logic [5:0]  aw, ah, bw, bh
    );
        return ((ax < bx + bw) && (ax + aw > bx) &&
                (ay < by + bh) && (ay + ah > by));
    endfunction

    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            s_cac_x        <= 1200;
            group_x        <= 1600;
            lava_x         <= 1800;
            ptr_x          <= 1400;
            obstacle_speed <= 1;
            passed_count   <= 0;
            game_over      <= 0;
            sprite_state   <= 0;
            motion_timer   <= 0;
          // … existing reset of positions, flags, etc. …
        score <= 10'd0;
        end else if (!game_over) begin
            if (motion_timer >= 24'd2_000_000) begin
                // wrap each obstacle with a different pseudo‐random offset
                s_cac_x <= (s_cac_x <= obstacle_speed)
                           ? (HACTIVE + {lfsr,       4'd0})
                           : s_cac_x - obstacle_speed;
                group_x <= (group_x <= obstacle_speed)
                           ? (HACTIVE + {lfsr ^ 6'h3F,4'd0})
                           : group_x - obstacle_speed;
                lava_x  <= (lava_x  <= obstacle_speed)
                           ? (HACTIVE + {{lfsr[3:0]},6'd0})
                           : lava_x  - obstacle_speed;
                ptr_x   <= (ptr_x   <= obstacle_speed)
                           ? (HACTIVE + {{lfsr[5:2]},6'd0})
                           : ptr_x   - obstacle_speed;
                // tick the score (wrap from 999 back to 0)
            score <= (score == 10'd999) ? 10'd0 : score + 10'd1;
                // count passes and speed up
                if (s_cac_x <= obstacle_speed || group_x <= obstacle_speed ||
                    lava_x  <= obstacle_speed || ptr_x   <= obstacle_speed) begin
                    passed_count <= passed_count + 1;
                end
                if (passed_count >= 12) begin
                    obstacle_speed <= obstacle_speed + 1;
                    passed_count   <= 0;
                end

                motion_timer <= 0;
                sprite_state <= sprite_state + 1;
            end else begin
                motion_timer <= motion_timer + 1;
            end

            if (collide(dino_x, dino_y, s_cac_x,  s_cac_y,  32,32,32,32) ||
                collide(dino_x, dino_y, group_x,  group_y, 150,40,32,32) ||
                collide(dino_x, dino_y, lava_x,   lava_y,   32,32,32,32) ||
                collide(dino_x, dino_y, ptr_x,    ptr_y,    32,32,32,32)) begin
                game_over <= 1;
            end
        end else begin
            // on replay, reset everything
            if (controller_report[4]) begin
                s_cac_x        <= 1200;
                group_x        <= 1600;
                lava_x         <= 1800;
                ptr_x          <= 1400;
                obstacle_speed <= 1;
                passed_count   <= 0;
                game_over      <= 0;
            end
        end
    end

    // VGA COUNTERS
    vga_counters counters(
        .clk50     (clk),
        .reset     (reset),
        .hcount    (hcount),
        .vcount    (vcount),
        .VGA_CLK   (VGA_CLK),
        .VGA_HS    (VGA_HS),
        .VGA_VS    (VGA_VS),
        .VGA_BLANK_n(VGA_BLANK_n),
        .VGA_SYNC_n(VGA_SYNC_n)
    );

    // SPRITE ROMS (unchanged) …
    dino_s_cac_rom       s_cac_rom(.clk(clk), .address(scac_sprite_addr), .data(scac_sprite_output));
    dino_cac_tog_rom     cacti_group_rom(.clk(clk), .address(group_addr),      .data(group_output));
    dino_lava_rom        lava_rom(.clk(clk),   .address(lava_sprite_addr),  .data(lava_output));
    dino_pterodactyl_down_rom  ptero_up(.clk(clk), .address(ptr_sprite_addr), .data(ptr_up_output));
    dino_pterodactyl_up_rom    ptero_down(.clk(clk), .address(ptr_sprite_addr), .data(ptr_down_output));

    // DINO ROM (unchanged) …
    assign dino_sprite_output = dino_new_output;
    dino_sprite_rom dino_rom(.clk(clk), .address(dino_sprite_addr), .data(dino_new_output));

    // REPLAY ROM (unchanged) …
    dino_replay_rom replay_rom(.clk(clk), .address(replay_addr), .data(replay_output));

    // PTR animation (unchanged) …
    always_comb begin
        case (sprite_state)
            2'd0: ptr_sprite_output = ptr_up_output;
            2'd1: ptr_sprite_output = ptr_down_output;
            default: ptr_sprite_output = ptr_up_output;
        endcase
    end

    // DRAWING
always_ff @(posedge clk) begin
    a <= 8'd135; b <= 8'd206; c <= 8'd235;

    if (!game_over) begin
      // split into hundreds, tens, units
        digit_h = score / 100;
        digit_t = (score / 10) % 10;
        digit_u = score % 10;
// --- hundreds digit at (SCORE_X, SCORE_Y) ---
if (hcount >= SCORE_X      && hcount < SCORE_X +  8 &&
    vcount >= SCORE_Y      && vcount < SCORE_Y +  8 &&
    font_rom[score/100]    [vcount - SCORE_Y][7 - (hcount - SCORE_X)])
begin
  a <= 8'h00; b <= 8'h00; c <= 8'h00;
end

// --- tens digit at (SCORE_X+16, SCORE_Y) ---
if (hcount >= SCORE_X + 16 && hcount < SCORE_X + 24 &&
    vcount >= SCORE_Y      && vcount < SCORE_Y +  8 &&
    font_rom[(score/10)%10][vcount - SCORE_Y][7 - (hcount - (SCORE_X+16))])
begin
  a <= 8'h00; b <= 8'h00; c <= 8'h00;
end

// --- units digit at (SCORE_X+32, SCORE_Y) ---
if (hcount >= SCORE_X + 32 && hcount < SCORE_X + 40 &&
    vcount >= SCORE_Y      && vcount < SCORE_Y +  8 &&
    font_rom[score%10]     [vcount - SCORE_Y][7 - (hcount - (SCORE_X+32))])
begin
  a <= 8'h00; b <= 8'h00; c <= 8'h00;
end

        /*// hundreds digit at (10,10)
        if (hcount >= 10 && hcount < 18 && vcount >= 10 && vcount < 18 &&
            font_rom[digit_h][vcount-10][7 - (hcount-10)]) begin
            a <= 8'd0; b <= 8'd0; c <= 8'd0;
        end

        // tens digit at (20,10)
        if (hcount >= 20 && hcount < 28 && vcount >= 10 && vcount < 18 &&
            font_rom[digit_t][vcount-10][7 - (hcount-20)]) begin
            a <= 8'd0; b <= 8'd0; c <= 8'd0;
        end

        // units digit at (30,10)
        if (hcount >= 30 && hcount < 38 && vcount >= 10 && vcount < 18 &&
            font_rom[digit_u][vcount-10][7 - (hcount-30)]) begin
            a <= 8'd0; b <= 8'd0; c <= 8'd0;
        end*/
        if (hcount >= dino_x && hcount < dino_x + 32 && vcount >= dino_y && vcount < dino_y + 32) begin
            dino_sprite_addr <= (hcount - dino_x) + ((vcount - dino_y) * 32);
            if (is_visible(dino_sprite_output)) begin
                a <= {dino_sprite_output[15:11], 3'b000};
                b <= {dino_sprite_output[10:5],  2'b00};
                c <= {dino_sprite_output[4:0],   3'b000};
            end
        end
        if (hcount >= s_cac_x && hcount < s_cac_x + 32 && vcount >= s_cac_y && vcount < s_cac_y + 32) begin
            scac_sprite_addr <= (hcount - s_cac_x) + ((vcount - s_cac_y) * 32);
            if (is_visible(scac_sprite_output)) begin
                a <= {scac_sprite_output[15:11], 3'b000};
                b <= {scac_sprite_output[10:5],  2'b00};
                c <= {scac_sprite_output[4:0],   3'b000};
            end
        end
        if (hcount >= group_x && hcount < group_x + 150 && vcount >= group_y && vcount < group_y + 40) begin
            group_addr <= (hcount - group_x) + ((vcount - group_y) * 150);
            if (is_visible(group_output)) begin
                a <= {group_output[15:11], 3'b000};
                b <= {group_output[10:5],  2'b00};
                c <= {group_output[4:0],   3'b000};
            end
        end
        if (hcount >= lava_x && hcount < lava_x + 32 && vcount >= lava_y && vcount < lava_y + 32) begin
            lava_sprite_addr <= (hcount - lava_x) + ((vcount - lava_y) * 32);
            if (is_visible(lava_output)) begin
                a <= {lava_output[15:11], 3'b000};
                b <= {lava_output[10:5],  2'b00};
                c <= {lava_output[4:0],   3'b000};
            end
        end
        if (hcount >= ptr_x && hcount < ptr_x + 32 && vcount >= ptr_y && vcount < ptr_y + 32) begin
            ptr_sprite_addr <= (31 - (hcount - ptr_x)) + ((vcount - ptr_y) * 32);
            if (is_visible(ptr_sprite_output)) begin
                a <= {ptr_sprite_output[15:11], 3'b000};
                b <= {ptr_sprite_output[10:5],  2'b00};
                c <= {ptr_sprite_output[4:0],   3'b000};
            end
        end
    end else begin
        if (hcount >= replay_x && hcount < replay_x + 160 && vcount >= replay_y && vcount < replay_y + 40) begin
            replay_addr <= (hcount - replay_x) + ((vcount - replay_y) * 160);
            if (is_visible(replay_output)) begin
                a <= {replay_output[15:11], 3'b000};
                b <= {replay_output[10:5],  2'b00};
                c <= {replay_output[4:0],   3'b000};
            end
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
