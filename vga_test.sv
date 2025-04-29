// vga_ball.v
`timescale 1ns/1ps

module vga_ball(
    input  logic         clk,
    input  logic         reset,
    input  logic [31:0]  writedata,
    input  logic         write,
    input               chipselect,
    input  logic [8:0]   address,

    output logic [7:0]  VGA_R,
    output logic [7:0]  VGA_G,
    output logic [7:0]  VGA_B,
    output logic        VGA_CLK,
    output logic        VGA_HS,
    output logic        VGA_VS,
    output logic        VGA_BLANK_n,
    output logic        VGA_SYNC_n
);

    // === VGA Timing Counters ===
    logic [10:0] hcount;
    logic [9:0]  vcount;

    // === Time-of-day cycle parameters ===
    localparam integer CYCLE_TICKS  = 32'd500_000_000;   // full cycle (~10s @50MHz)
    logic [31:0]   tod_counter;
    localparam integer PHASE_TICKS  = CYCLE_TICKS >> 2;

    // === Key-frame colors ===
    localparam [7:0] DAY_R   = 8'd135, DAY_G   = 8'd206, DAY_B   = 8'd235;
    localparam [7:0] SUNS_R  = 8'd255, SUNS_G  = 8'd102, SUNS_B  = 8'd0;
    localparam [7:0] NIGHT_R = 8'd15,  NIGHT_G = 8'd15,  NIGHT_B = 8'd60;
    localparam [7:0] RISE_R  = 8'd255, RISE_G  = 8'd153, RISE_B  = 8'd204;

    // === Interpolation logic ===
    wire [1:0]   phase      = tod_counter / PHASE_TICKS;
    wire [31:0]  phase_time = tod_counter % PHASE_TICKS;
    wire [15:0]  frac       = (phase_time * 16'd255) / PHASE_TICKS;
    wire [15:0]  inv_frac   = 16'd255 - frac;

    reg  [7:0]   C0_R, C0_G, C0_B;
    reg  [7:0]   C1_R, C1_G, C1_B;
    wire [7:0]   sky_r      = (C0_R * inv_frac + C1_R * frac) >> 8;
    wire [7:0]   sky_g      = (C0_G * inv_frac + C1_G * frac) >> 8;
    wire [7:0]   sky_b      = (C0_B * inv_frac + C1_B * frac) >> 8;

    // === Animation Counters ===
    logic [23:0] frame_counter, cloud_counter, sun_counter;
    logic [1:0]  sprite_state, sprite_state2;
    logic [10:0] cloud_offset, sun_offset_x, sun_offset_y;

    // === Pixel registers ===
    logic [7:0]  a, b, c;

    // === Sprite ROM outputs & addresses ===
    logic [15:0] dino_new_output, dino_left_output, dino_right_output;
    logic [15:0] dino_sprite_output, jump_sprite_output, duck_sprite_output;
    logic [15:0] godzilla_sprite_output, scac_sprite_output;
    logic [15:0] ptr_up_output, ptr_down_output, powerup_sprite_output;
    logic [9:0]  dino_sprite_addr, jump_sprite_addr, duck_sprite_addr;
    logic [9:0]  scac_sprite_addr, godzilla_sprite_addr;
    logic [9:0]  ptr_sprite_addr, powerup_sprite_addr;

    // === Sprite positions ===
    logic [10:0] dino_x, dino_y, jump_x, jump_y;
    logic [10:0] duck_x, duck_y, s_cac_x, s_cac_y;
    logic [10:0] godzilla_x, godzilla_y;
    logic [10:0] ptr_x, ptr_y, powerup_x, powerup_y;

    // === Score overlay ===
    logic [3:0]  score;
    logic [7:0]  score_x, score_y;
    logic [7:0]  font_rom [0:9][0:7];

    // Initialize font ROM
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

    // Update counters & animation
    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            tod_counter   <= 0;
            frame_counter <= 0;
            cloud_counter <= 0;
            sun_counter   <= 0;
            sprite_state  <= 0;
            sprite_state2 <= 0;
            cloud_offset  <= 0;
            sun_offset_x  <= 0;
            sun_offset_y  <= 0;
            score         <= 0;
            score_x       <= 8'd25;
            score_y       <= 8'd41;
        end else begin
            // Time-of-day
            if (tod_counter == CYCLE_TICKS-1) tod_counter <= 0;
            else tod_counter <= tod_counter + 1;
            // Sprite frames
            if (frame_counter == 24'd5_000_000) begin
                sprite_state  <= sprite_state + 1;
                sprite_state2 <= sprite_state2 + 1;
                frame_counter <= 0;
            end else frame_counter <= frame_counter + 1;
            // Clouds
            if (cloud_counter == 24'd8_000_000) begin
                cloud_counter <= 0;
                cloud_offset  <= cloud_offset + 1;
                if (cloud_offset > 1280) cloud_offset <= 0;
            end else cloud_counter <= cloud_counter + 1;
            // Sun
            if (sun_counter == 24'd8_000_000) begin
                sun_counter   <= 0;
                sun_offset_x  <= sun_offset_x + 1;
                if (sun_offset_x > 1280) begin
                    sun_offset_x <= 0;
                    sun_offset_y <= sun_offset_y + 1;
                    if (sun_offset_y > 50) sun_offset_y <= 0;
                end
            end else sun_counter <= sun_counter + 1;
        end
    end

    // Pick colors for current phase
    always_comb begin
        case(phase)
            2'd0: begin // Day->Sunset
                {C0_R,C0_G,C0_B} = {DAY_R, DAY_G, DAY_B};
                {C1_R,C1_G,C1_B} = {SUNS_R,SUNS_G,SUNS_B};
            end
            2'd1: begin // Sunset->Night
                {C0_R,C0_G,C0_B} = {SUNS_R,SUNS_G,SUNS_B};
                {C1_R,C1_G,C1_B} = {NIGHT_R,NIGHT_G,NIGHT_B};
            end
            2'd2: begin // Night->Sunrise
                {C0_R,C0_G,C0_B} = {NIGHT_R,NIGHT_G,NIGHT_B};
                {C1_R,C1_G,C1_B} = {RISE_R, RISE_G, RISE_B };
            end
            2'd3: begin // Sunrise->Day
                {C0_R,C0_G,C0_B} = {RISE_R,RISE_G,RISE_B};
                {C1_R,C1_G,C1_B} = {DAY_R, DAY_G, DAY_B };
            end
        endcase
    end

    // Timing generator
    vga_counters counters(
        .clk50(clk), .reset(reset),
        .hcount(hcount), .vcount(vcount),
        .VGA_CLK(VGA_CLK), .VGA_HS(VGA_HS), .VGA_VS(VGA_VS),
        .VGA_BLANK_n(VGA_BLANK_n), .VGA_SYNC_n(VGA_SYNC_n)
    );

    // Sprite ROMs
    dino_sprite_rom       dino_rom     (.clk(clk), .address(dino_sprite_addr),     .data(dino_new_output));
    dino_left_leg_up_rom  dino_rom1    (.clk(clk), .address(dino_sprite_addr),     .data(dino_left_output));
    dino_right_leg_up_rom dino_rom2    (.clk(clk), .address(dino_sprite_addr),     .data(dino_right_output));
    dino_jump_rom         jump_rom     (.clk(clk), .address(jump_sprite_addr),    .data(jump_sprite_output));
    dino_duck_rom         duck_rom     (.clk(clk), .address(duck_sprite_addr),    .data(duck_sprite_output));
    dino_godzilla_rom     godzilla_rom (.clk(clk), .address(godzilla_sprite_addr),  .data(godzilla_sprite_output));
    dino_s_cac_rom        s_cac_rom    (.clk(clk), .address(scac_sprite_addr),     .data(scac_sprite_output));
    dino_powerup_rom      powerup_rom  (.clk(clk), .address(powerup_sprite_addr), .data(powerup_sprite_output));
    dino_pterodactyl_down_rom ptero_down(.clk(clk), .address(ptr_sprite_addr),       .data(ptr_down_output));
    dino_pterodactyl_up_rom   ptero_up  (.clk(clk), .address(ptr_sprite_addr),       .data(ptr_up_output));

    // Choose current sprite frames
    always_comb begin
        case(sprite_state)
            2'd0: dino_sprite_output = dino_new_output;
            2'd1: dino_sprite_output = dino_left_output;
            2'd2: dino_sprite_output = dino_right_output;
        endcase
    end
    always_comb begin
        case(sprite_state2)
            2'd0: ptr_sprite_output = ptr_up_output;
            2'd1: ptr_sprite_output = ptr_down_output;
        endcase
    end

    // Draw everything
    function automatic logic is_visible(input logic [15:0] pixel);
        return (pixel != 16'hF81F && pixel != 16'hFFFF && pixel != 16'hFFDF && pixel != 16'hFFDE && pixel != 16'h8410);
    endfunction

    always_ff @(posedge clk) begin
        if(reset) begin
            dino_x <= 100; dino_y <= 100;
            jump_x <= 200; jump_y <= 150;
            duck_x <= 300; duck_y <= 550;
            s_cac_x<= 500; s_cac_y<= 100;
            godzilla_x<=100; godzilla_y<=360;
            powerup_x<=130; powerup_y<=260;
            ptr_x  <=700; ptr_y  <=100;
            score  <=0;   score_x<=25; score_y<=41;
            a<=8'hFF; b<=8'hFF; c<=8'hFF;
        end else if(chipselect && write) begin
            case(address)
                9'd0:  dino_x    <= writedata[7:0];
                9'd1:  dino_y    <= writedata[7:0];
                9'd2:  jump_x    <= writedata[7:0];
                9'd3:  jump_y    <= writedata[7:0];
                9'd4:  duck_x    <= writedata[7:0];
                9'd5:  duck_y    <= writedata[7:0];
                9'd6:  s_cac_x   <= writedata[9:0];
                9'd7:  s_cac_y   <= writedata[9:0];
                9'd8:  godzilla_x<= writedata[9:0];
                9'd9:  godzilla_y<= writedata[9:0];
                9'd10: score     <= writedata[3:0];
                9'd11: score_x   <= writedata[7:0];
                9'd12: score_y   <= writedata[7:0];
            endcase
        end else if(VGA_BLANK_n) begin
            // Sky/Ground
            if(vcount<280)      {a,b,c}<= {sky_r,sky_g,sky_b};
            else if(vcount<400) {a,b,c}<= {8'd139,8'd69,8'd19};
            else                {a,b,c}<= {8'd100,8'd40,8'd10};
            if(vcount==280)     {a,b,c}<= {8'd0,8'd0,8'd0};
            // Sun
            if(((hcount-(1150-sun_offset_x))*(hcount-(1150-sun_offset_x))+ (vcount-(80+sun_offset_y))*(vcount-(80+sun_offset_y)))<1200 &&
               ((hcount-(1150-sun_offset_x))*(hcount-(1150-sun_offset_x))+ (vcount-(80+sun_offset_y))*(vcount-(80+sun_offset_y)))>900)
                {a,b,c}<={8'd255,8'd255,8'd100};
            if(((hcount-(1150-sun_offset_x))*(hcount-(1150-sun_offset_x))+ (vcount-(80+sun_offset_y))*(vcount-(80+sun_offset_y)))<900)
                {a,b,c}<={8'd255,8'd255,8'd0};
            // Clouds (reuse existing checks...)
            // Sprites
            if(hcount>=dino_x&&hcount<dino_x+32&&vcount>=dino_y&&vcount<dino_y+32) begin
                dino_sprite_addr<=(hcount-dino_x)+((vcount-dino_y)*32);
                if(is_visible(dino_sprite_output)) {a,b,c}<={dino_sprite_output[15:11],3'b000, dino_sprite_output[10:5],2'b00, dino_sprite_output[4:0],3'b000};
            end
            if(hcount>=jump_x&&hcount<jump_x+32&&vcount>=jump_y&&vcount<jump_y+32) begin
                jump_sprite_addr<=(hcount-jump_x)+((vcount-jump_y)*32);
                if(is_visible(jump_sprite_output)) {a,b,c}<={jump_sprite_output[15:11],3'b000, jump_sprite_output[10:5],2'b00, jump_sprite_output[4:0],3'b000};
            end
            if(hcount>=duck_x&&hcount<duck_x+32&&vcount>=duck_y&&vcount<duck_y+32) begin
                duck_sprite_addr<=(hcount-duck_x)+((vcount-duck_y)*32);
                if(is_visible(duck_sprite_output)) {a,b,c}<={duck_sprite_output[15:11],3'b000, duck_sprite_output[10:5],2'b00, duck_sprite_output[4:0],3'b000};
            end
            if(hcount>=s_cac_x&&hcount<s_cac_x+32&&vcount>=s_cac_y&&vcount<s_cac_y+32) begin
                scac_sprite_addr<=(hcount-s_cac_x)+((vcount-s_cac_y)*32);
                if(is_visible(scac_sprite_output)) {a,b,c}<={scac_sprite_output[15:11],3'b000, scac_sprite_output[10:5],2'b00, scac_sprite_output[4:0],3'b000};
            end
            if(hcount>=godzilla_x&&hcount<godzilla_x+32&&vcount>=godzilla_y&&vcount<godzilla_y+32) begin
                godzilla_sprite_addr<=(hcount-godzilla_x)+((vcount-godzilla_y)*32);
                if(is_visible(godzilla_sprite_output)) {a,b,c}<={godzilla_sprite_output[15:11],3'b000, godzilla_sprite_output[10:5],2'b00, godzilla_sprite_output[4:0],3'b000};
            end
            if(hcount>=powerup_x&&hcount<powerup_x+32&&vcount>=powerup_y&&vcount<powerup_y+32) begin
                powerup_sprite_addr<=(hcount-powerup_x)+((vcount-powerup_y)*32);
                if(is_visible(powerup_sprite_output)) {a,b,c}<={powerup_sprite_output[15:11],3'b000, powerup_sprite_output[10:5],2'b00, powerup_sprite_output[4:0],3'b000};
            end
            if(hcount>=ptr_x&&hcount<ptr_x+32&&vcount>=ptr_y&&vcount<ptr_y+32) begin
                ptr_sprite_addr<=(31-(hcount-ptr_x))+((vcount-ptr_y)*32);
                if(is_visible(ptr_up_output)) {a,b,c}<={ptr_up_output[15:11],3'b000, ptr_up_output[10:5],2'b00, ptr_up_output[4:0],3'b000};
            end
            // Score overlay
            if(hcount>=score_x+20&&hcount<score_x+28&&vcount>=score_y&&vcount<score_y+8&&font_rom[score][vcount-score_y][7-(hcount-(score_x+20))])
                {a,b,c}<={8'h00,8'h00,8'h00};
        end
    end

    // Drive DAC lines
    assign VGA_R = a;
    assign VGA_G = b;
    assign VGA_B = c;
endmodule

// vga_counters.v
module vga_counters(
    input  logic        clk50,
    input  logic        reset,
    output logic [10:0] hcount,
    output logic [9:0]  vcount,
    output logic        VGA_CLK,
    output logic        VGA_HS,
    output logic        VGA_VS,
    output logic        VGA_BLANK_n,
    output logic        VGA_SYNC_n
);
    parameter HACTIVE = 11'd1280, HFRONT = 11'd32, HSYNC = 11'd192, HBACK = 11'd96;
    parameter HTOTAL  = HACTIVE+HFRONT+HSYNC+HBACK;
    parameter VACTIVE = 10'd480, VFRONT = 10'd10, VSYNC = 10'd2, VBACK = 10'd33;
    parameter VTOTAL  = VACTIVE+VFRONT+VSYNC+VBACK;

    logic endOfLine = (hcount == HTOTAL-1);
    always_ff @(posedge clk50 or posedge reset)
        if(reset) hcount <= 0;
        else if(endOfLine) hcount <= 0;
        else hcount <= hcount + 1;

    logic endOfField = (vcount == VTOTAL-1);
    always_ff @(posedge clk50 or posedge reset)
        if(reset) vcount <= 0;
        else if(endOfLine) begin
            if(endOfField) vcount <= 0;
            else vcount <= vcount + 1;
        end

    assign VGA_HS     = !((hcount >= HACTIVE+HFRONT) && (hcount < HACTIVE+HFRONT+HSYNC));
    assign VGA_VS     = !((vcount >= VACTIVE+VFRONT) && (vcount < VACTIVE+VFRONT+VSYNC));
    assign VGA_SYNC_n = 1'b0;
    assign VGA_BLANK_n= (hcount < HACTIVE) && (vcount < VACTIVE);
    assign VGA_CLK    = hcount[0];
endmodule
