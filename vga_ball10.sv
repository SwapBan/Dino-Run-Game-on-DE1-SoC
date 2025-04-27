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
    // -------------------------------------------------------------------
  // 1) Replace the old 4-bit score with a 10-bit counter + 50MHz timer
  // -------------------------------------------------------------------
  logic [9:0]  score;           // allows 0…999
  logic [25:0] sec_counter;     // counts 50 MHz cycles → 1 s

    logic [10:0] hcount;
    logic [9:0]  vcount;

    logic [15:0] dino_new_output, dino_left_output, dino_right_output;
    logic [15:0] dino_sprite_output, jump_sprite_output, duck_sprite_output;
    logic [15:0] godzilla_sprite_output, scac_sprite_output, ptr_sprite_output;
    logic [15:0] powerup_sprite_output;
    logic [15:0] ptr_up_output, ptr_down_output;

    // at the top, alongside dino_sprite_addr, etc.
    logic [5:0]  score_sprite_addr;
    logic  score_sprite_output;
    logic [9:0] dino_sprite_addr, jump_sprite_addr, duck_sprite_addr;
    logic [9:0] godzilla_sprite_addr, scac_sprite_addr;
    logic [9:0] powerup_sprite_addr, ptr_sprite_addr;

    logic [7:0] dino_x, dino_y;
    logic [7:0] jump_x, jump_y;
    logic [7:0] duck_x, duck_y;
    logic [9:0] s_cac_x, s_cac_y;
    logic [9:0] godzilla_x, godzilla_y;
    logic [7:0] powerup_x, powerup_y;
    logic [7:0] ptr_x, ptr_y;
    logic [3:0] ones, tens, hundreds;

    logic [7:0] a, b, c;
 // === SCORE overlay signals ===
   // logic [3:0]  score;
  //  logic [7:0]  score_x, score_y;
    logic [7:0] score_x = 8'd225, score_y = 8'd441;
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
    logic [1:0] sprite_state2;

    function automatic logic is_visible(input logic [15:0] pixel);

 

        return (pixel != 16'hF81F && pixel != 16'hFFFF && pixel != 16'hFFDF && pixel != 16'hFFDE && pixel != 16'h8410);
 

    endfunction



    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            frame_counter <= 0;
            sprite_state <= 0;
            sprite_state2 <= 0;
            sec_counter <= 0;
            score <= 0;
        end else if (sec_counter == 26'd50_000_000-1) begin
            sec_counter <= 0;
            score <= score + 1;
        end else begin
             sec_counter <= sec_counter + 1;
            if (frame_counter == 24'd5_000_000) begin
                sprite_state <= sprite_state + 1;
                sprite_state2 <= sprite_state2 + 1;
                frame_counter <= 0;
            end else begin
                frame_counter <= frame_counter + 1;
               
            end
            
        end
    end

always_comb begin
    ones     = score % 10;
    tens     = (score / 10) % 10;
    hundreds = (score / 100) % 10;
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

    dino_pterodactyl_down_rom ptero_up(.clk(clk), .address(ptr_sprite_addr), .data(ptr_up_output));
    dino_pterodactyl_up_rom ptero_down(.clk(clk), .address(ptr_sprite_addr), .data(ptr_down_output));

    

    dino_jump_rom jump_rom(.clk(clk), .address(jump_sprite_addr), .data(jump_sprite_output));
    dino_duck_rom duck_rom(.clk(clk), .address(duck_sprite_addr), .data(duck_sprite_output));
    dino_godzilla_rom godzilla_rom(.clk(clk), .address(godzilla_sprite_addr), .data(godzilla_sprite_output));
    dino_s_cac_rom s_cac_rom(.clk(clk), .address(scac_sprite_addr), .data(scac_sprite_output));
    dino_powerup_rom powerup_rom(.clk(clk), .address(powerup_sprite_addr), .data(powerup_sprite_output));

    
   

    // === CHOOSE CURRENT DINO SPRITE BASED ON STATE ===
    always_comb begin
        case (sprite_state)
            2'd0: dino_sprite_output = dino_new_output;
            2'd1: dino_sprite_output = dino_left_output;
            2'd2: dino_sprite_output = dino_right_output;
            default: dino_sprite_output = dino_new_output;
        endcase
    end

    always_comb begin
        case (sprite_state2)
            2'd0: ptr_sprite_output = ptr_up_output;
            2'd1: ptr_sprite_output = ptr_down_output;
            default: ptr_sprite_output = ptr_up_output;
        endcase
    end


    // === SPRITE DRAWING + CONTROL LOGIC ===
    always_ff @(posedge clk) begin
        if (reset) begin
            dino_x <= 8'd100;   dino_y <= 8'd100;
            jump_x <= 8'd200;   jump_y <= 8'd150;
            duck_x <= 8'd300;   duck_y <= 8'd550;
            s_cac_x <= 9'd500;  s_cac_y <= 9'd100;
            godzilla_x <= 9'd100; godzilla_y <= 9'd360;
            powerup_x <= 8'd130; powerup_y <= 8'd260;
            ptr_x <= 8'd700;     ptr_y <= 8'd100;
            // default score
            //score   <= 4'd0;
            score_x <= 8'd25;
            score_y <= 8'd41;
            a <= 8'hFF; b <= 8'hFF; c <= 8'hFF;
             
         if (vcount < 200) begin
    // light sky blue
    a <= 8'd135;  // R = 135
    b <= 8'd206;  // G = 206
    c <= 8'd235;  // B = 235
          end else begin
            a <= 8'd139;  // brown R
        b <= 8'd69;   // brown G
        c <= 8'd19;   // brown B
          end
       if (vcount == 200) begin
        a <= 8'd0;
        b <= 8'd0;
        c <= 8'd0;
    end     
        end else if (chipselect && write) begin
            case (address)
                9'd0: dino_x <= writedata[7:0];
                9'd1: dino_y <= writedata[7:0];
                9'd2: jump_x <= writedata[7:0];
                9'd3: jump_y <= writedata[7:0];
                9'd4: duck_x <= writedata[7:0];
                9'd5: duck_y <= writedata[7:0];
                9'd6: s_cac_x <= writedata[9:0];
                9'd7: s_cac_y <= writedata[9:0];
                9'd8: godzilla_x <= writedata[9:0];
                9'd9: godzilla_y <= writedata[9:0];
               // 9'd10: score   <= writedata[3:0];
                9'd11: score_x <= writedata[7:0];
                9'd12: score_y <= writedata[7:0];
            endcase
    end else if (VGA_BLANK_n) begin
            
    if (vcount < 200) begin
    // light sky blue
        a <= 8'd135;  // R = 135
        b <= 8'd206;  // G = 206
        c <= 8'd235;  // B = 235
    end else begin
        a <= 8'd139;  // brown R
        b <= 8'd69;   // brown G
        c <= 8'd19;   // brown B
    end
    if (vcount == 200) begin
        a <= 8'd0;
        b <= 8'd0;
        c <= 8'd0;
    end
     // === Sun (big yellow circle at top right) ===
    if ((hcount-1150)*(hcount-1150) + (vcount-80)*(vcount-80) < 900) begin
        a <= 8'd255; // Yellow R
        b <= 8'd255; // Yellow G
        c <= 8'd0;   // Yellow B
    end

    // === Cloud 1 (around (250, 70)) ===
    if (((hcount-240)*(hcount-240) + (vcount-70)*(vcount-70) < 100) ||
        ((hcount-250)*(hcount-250) + (vcount-60)*(vcount-60) < 100) ||
        ((hcount-260)*(hcount-260) + (vcount-70)*(vcount-70) < 100)) begin
        a <= 8'd255;
        b <= 8'd255;
        c <= 8'd255;
    end

    // === Cloud 2 (around (450, 100)) ===
    if (((hcount-440)*(hcount-440) + (vcount-100)*(vcount-100) < 100) ||
        ((hcount-450)*(hcount-450) + (vcount-90)*(vcount-90) < 100) ||
        ((hcount-460)*(hcount-460) + (vcount-100)*(vcount-100) < 100)) begin
        a <= 8'd250;
        b <= 8'd250;
        c <= 8'd250;
    end

    // === Cloud 3 (around (700, 60)) ===
    if (((hcount-690)*(hcount-690) + (vcount-60)*(vcount-60) < 100) ||
        ((hcount-700)*(hcount-700) + (vcount-50)*(vcount-50) < 100) ||
        ((hcount-710)*(hcount-710) + (vcount-60)*(vcount-60) < 100)) begin
        a <= 8'd245;
        b <= 8'd245;
        c <= 8'd245;
    end

    // === Ground rocks / bumps (small dark brown spots) ===
    if (vcount > 200 && vcount < 480) begin
        if ((hcount % 120 == 0 && vcount % 50 < 10) ||
            (hcount % 200 == 15 && vcount % 60 < 8)) begin
            a <= 8'd110; // darker brown
            b <= 8'd50;
            c <= 8'd10;
        end
    end
            if (hcount >= dino_x && hcount < dino_x + 32 &&
                vcount >= dino_y && vcount < dino_y + 32) begin
                dino_sprite_addr <= (hcount - dino_x) + ((vcount - dino_y) * 32);
                if (is_visible(dino_sprite_output)) begin
                    a <= {dino_sprite_output[15:11], 3'b000};
                    b <= {dino_sprite_output[10:5],  2'b00};
                    c <= {dino_sprite_output[4:0],   3'b000};
                end
            end

            if (hcount >= jump_x && hcount < jump_x + 32 &&
                vcount >= jump_y && vcount < jump_y + 32) begin
                jump_sprite_addr <= (hcount - jump_x) + ((vcount - jump_y) * 32);
                if (is_visible(jump_sprite_output)) begin
                a <= {jump_sprite_output[15:11], 3'b000};
                b <= {jump_sprite_output[10:5],  2'b00};
                c <= {jump_sprite_output[4:0],   3'b000};
                end
            end

            if (hcount >= duck_x && hcount < duck_x + 32 &&
                vcount >= duck_y && vcount < duck_y + 32) begin
                duck_sprite_addr <= (hcount - duck_x) + ((vcount - duck_y) * 32);
                if (is_visible(duck_sprite_output)) begin
                a <= {duck_sprite_output[15:11], 3'b000};
                b <= {duck_sprite_output[10:5],  2'b00};
                c <= {duck_sprite_output[4:0],   3'b000};
                end
            end

            if (hcount >= s_cac_x && hcount < s_cac_x + 32 &&
                vcount >= s_cac_y && vcount < s_cac_y + 32) begin
                scac_sprite_addr <= (hcount - s_cac_x) + ((vcount - s_cac_y) * 32);
                if (is_visible(scac_sprite_output)) begin
                a <= {scac_sprite_output[15:11], 3'b000};
                b <= {scac_sprite_output[10:5],  2'b00};
                c <= {scac_sprite_output[4:0],   3'b000};
                end
            end

            if (hcount >= godzilla_x && hcount < godzilla_x + 32 &&
                vcount >= godzilla_y && vcount < godzilla_y + 32) begin
                godzilla_sprite_addr <= (hcount - godzilla_x) + ((vcount - godzilla_y) * 32);
                if (is_visible(godzilla_sprite_output)) begin
                a <= {godzilla_sprite_output[15:11], 3'b000};
                b <= {godzilla_sprite_output[10:5],  2'b00};
                c <= {godzilla_sprite_output[4:0],   3'b000};
                end
            end
            if (hcount >= powerup_x && hcount < powerup_x + 32 &&
                vcount >= powerup_y && vcount < powerup_y + 32) begin
                powerup_sprite_addr <= (hcount - powerup_x) + ((vcount - powerup_y) * 32);
                if (is_visible(powerup_sprite_output)) begin
                a <= {powerup_sprite_output[15:11], 3'b000};
                b <= {powerup_sprite_output[10:5],  2'b00};
                c <= {powerup_sprite_output[4:0],   3'b000};
                end
            end
            if (hcount >= ptr_x && hcount < ptr_x + 32 &&
                vcount >= ptr_y && vcount < ptr_y + 32) begin
                ptr_sprite_addr <= (31 - (hcount - ptr_x)) + ((vcount - ptr_y) * 32);
                if (is_visible(ptr_sprite_output)) begin
                    a <= {ptr_sprite_output[15:11], 3'b000};
                    b <= {ptr_sprite_output[10:5],  2'b00};
                    c <= {ptr_sprite_output[4:0],   3'b000};
                end
            end
            // score overlay
                        // score overlay (inline lookup, no extra regs)
           // --- SCORE as an 8×8 “sprite” ---
           // 5) overlay “3” at (score_x+20,score_y)
          // draw “3” at (score_x+20, score_y)
/*if (hcount >= score_x+20 && hcount <  score_x+28 &&
    vcount >= score_y   && vcount <  score_y+8  &&
    font_rom[3][vcount-score_y]
               [7 - (hcount - (score_x+20))])
begin
    a <= 8'h00; b <= 8'h00; c <= 8'h00;
end

// draw “5” at (score_x+30, score_y)
if (hcount >= score_x+30 && hcount <  score_x+38 &&
    vcount >= score_y   && vcount <  score_y+8  &&
    font_rom[5][vcount-score_y]
               [7 - (hcount - (score_x+30))])
begin
    a <= 8'h00; b <= 8'h00; c <= 8'h00;
end
// draw “5” at (score_x+30, score_y)
if (hcount >= score_x+40 && hcount <  score_x+48 &&
    vcount >= score_y   && vcount <  score_y+8  &&
                font_rom[7][vcount-score_y]
    [7 - (hcount - (score_x+40))])
begin
    a <= 8'h00; b <= 8'h00; c <= 8'h00;
end
*/
        // --- SCORE overlay as three 8×8 glyphs ---
            // hundreds at (score_x,   score_y)
            if (hcount >= score_x
                && hcount <  score_x + 8
                && vcount >= score_y
                && vcount <  score_y + 8
                && font_rom[ hundreds ][ vcount - score_y ][ 7 - (hcount - score_x) ])
            begin
                a <= 8'h00; b <= 8'h00; c <= 8'h00;
            end

            // tens at (score_x+8, score_y)
            if (hcount >= score_x + 8
                && hcount <  score_x + 16
                && vcount >= score_y
                && vcount <  score_y + 8
                && font_rom[ tens ][ vcount - score_y ][ 7 - (hcount - (score_x+8)) ])
            begin
                a <= 8'h00; b <= 8'h00; c <= 8'h00;
            end

            // ones at (score_x+16, score_y)
            if (hcount >= score_x + 16
                && hcount <  score_x + 24
                && vcount >= score_y
                && vcount <  score_y + 8
                && font_rom[ ones ][ vcount - score_y ][ 7 - (hcount - (score_x+16)) ])
            begin
                a <= 8'h00; b <= 8'h00; c <= 8'h00;
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
