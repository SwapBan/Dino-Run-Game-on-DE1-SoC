/*
 * Dino Run: Full Game Logic with Random Obstacles, Power-Up (Godzilla Mode),
 * Frame-locked motion, and Replay
 */
module dino_run_top(
    input  logic        clk,        // 50 MHz clock
    input  logic        reset,      // active-high reset
    input  logic [7:0]  controller_report,
    // Memory-mapped I/O (optional)
    input  logic [31:0] writedata,
    input  logic        write,
    input  logic        chipselect,
    input  logic [8:0]  address,

    output logic [7:0]  VGA_R, VGA_G, VGA_B,
    output logic        VGA_CLK, VGA_HS, VGA_VS, VGA_BLANK_n, VGA_SYNC_n
);

//---------------------------------------------------------------------------
// Parameters & Localparams
//---------------------------------------------------------------------------
localparam HACTIVE       = 1280;
localparam VACTIVE       = 480;
localparam MOTION_DIVIDE = 833_333;  // 50 MHz / 60 Hz

//---------------------------------------------------------------------------
// Signals: VGA Counters
//---------------------------------------------------------------------------
logic [10:0] hcount;
logic [9:0]  vcount;

//---------------------------------------------------------------------------
// Signals: Game Entities
//---------------------------------------------------------------------------
// Dino
logic [10:0] dino_x = 100, dino_y = 248;
logic [15:0] dino_sprite_output;
logic [15:0] dino_new_output;
logic [9:0]  dino_sprite_addr;

// Obstacles
logic [10:0] s_cac_x = 1200, s_cac_y = 248;
logic [10:0] group_x = 1600, group_y = 248;
logic [10:0] lava_x = 1800, lava_y = 248;
logic [10:0] ptr_x = 1400, ptr_y = 200;
logic [15:0] scac_sprite_output, group_output, lava_output;
logic [15:0] ptr_up_output, ptr_down_output, ptr_sprite_output;
logic [9:0]  scac_sprite_addr, lava_sprite_addr, ptr_sprite_addr;
logic [12:0] group_addr;
logic [1:0]  sprite_state;

// Power-Up (Godzilla Mode)
  logic [10:0] powerup_x = 900, powerup_y = 248;
logic [15:0] powerup_sprite_output;
logic [9:0]  powerup_sprite_addr;
logic        is_godzilla;
logic [23:0] godzilla_timer;

// Motion & Speed
logic [23:0] frame_counter;
logic [9:0]  obstacle_speed = 1;
logic [4:0]  passed_count;
logic        game_over;

//---------------------------------------------------------------------------
// Modules: VGA Timing
//---------------------------------------------------------------------------
vga_counters #(
    .HACTIVE(HACTIVE),
    .VACTIVE(VACTIVE)
) counters (
    .clk50(clk), .reset(reset),
    .hcount(hcount), .vcount(vcount),
    .VGA_CLK(VGA_CLK), .VGA_HS(VGA_HS),
    .VGA_VS(VGA_VS), .VGA_BLANK_n(VGA_BLANK_n),
    .VGA_SYNC_n(VGA_SYNC_n)
);

//---------------------------------------------------------------------------
// LFSR for Random Selection
//---------------------------------------------------------------------------
logic [7:0] lfsr;
always_ff @(posedge clk or posedge reset) begin
    if (reset) lfsr <= 8'hAC;
    else        lfsr <= {lfsr[6:0], lfsr[7] ^ lfsr[5]};
end

function logic [1:0] pick_obstacle(input logic [7:0] rnd);
    return rnd[1:0];
endfunction

//---------------------------------------------------------------------------
// Sprite-ROM Instantiation
//---------------------------------------------------------------------------
// Dino
 dino_sprite_rom         dino_rom     (.clk(clk), .address(dino_sprite_addr),    .data(dino_new_output));

// Legs (if desired)
 dino_left_leg_up_rom    dino_left_rom(.clk(clk), .address(dino_sprite_addr),    .data(/* not used */ dino_new_output));
 dino_right_leg_up_rom   dino_right_rom(.clk(clk),.address(dino_sprite_addr),    .data(/* not used */ dino_new_output));

// Jump & Duck
 dino_jump_rom           jump_rom     (.clk(clk), .address(dino_sprite_addr),    .data(/* not used */ dino_new_output));
 dino_duck_rom           duck_rom     (.clk(clk), .address(dino_sprite_addr),    .data(/* not used */ dino_new_output));

// Godzilla
 dino_godzilla_rom       godz_rom     (.clk(clk), .address( dino_sprite_addr), .data(dino_sprite_output));

// Obstacles
 dino_s_cac_rom          s_cac_rom    (.clk(clk), .address(scac_sprite_addr),  .data(scac_sprite_output));
 dino_cac_tog_rom        group_rom    (.clk(clk), .address(group_addr),         .data(group_output));
 dino_lava_rom           lava_rom     (.clk(clk), .address(lava_sprite_addr),  .data(lava_output));
 dino_pterodactyl_down_rom ptero_dn   (.clk(clk), .address(ptr_sprite_addr),    .data(ptr_up_output));
 dino_pterodactyl_up_rom ptero_up     (.clk(clk), .address(ptr_sprite_addr),    .data(ptr_down_output));

// Powerup
 dino_powerup_rom        pu_rom       (.clk(clk), .address(powerup_sprite_addr), .data(powerup_sprite_output));

//---------------------------------------------------------------------------
// Helpers: Visibility & Collision
//---------------------------------------------------------------------------
function logic is_visible(input logic [15:0] px);
    return (px != 16'hF81F && px != 16'hFFFF);
endfunction

function logic collide(
    input logic [10:0] ax, ay, bx, by,
    input logic [5:0]  aw, ah, bw, bh
);
    return (ax < bx + bw) && (ax + aw > bx) &&
           (ay < by + bh) && (ay + ah > by);
endfunction

//---------------------------------------------------------------------------
// Game FSM & Motion (Frame-locked)
//---------------------------------------------------------------------------
typedef enum logic {S_PLAY, S_GAMEOVER} state_t;
state_t state;
logic vs_prev;
logic vs_rising = VGA_VS && !vs_prev;


    always_ff @(posedge clk or posedge reset) begin
    if (reset) begin
        state           <= S_PLAY;
        game_over       <= 0;
        is_godzilla     <= 0;
        godzilla_timer  <= 0;
        passed_count    <= 0;
        obstacle_speed  <= 1;
        // Reset positions
        s_cac_x <= 1200; group_x <= 1600; lava_x <= 1800;
        ptr_x   <= 1400; powerup_x <= 1300;
        sprite_state <= 0;
        vs_prev        <= 1'b0;       // ← initialize vs_prev here
    end else begin
        vs_prev        <= VGA_VS;     // sample current VSYNC
    case (state)
            S_PLAY: begin
                // Frame tick? move everything
                if (vs_rising) begin
                    // Move obstacles & powerup
                    s_cac_x     <= (s_cac_x <= obstacle_speed)  ? HACTIVE : s_cac_x - obstacle_speed;
                    group_x     <= (group_x <= obstacle_speed)  ? HACTIVE : group_x - obstacle_speed;
                    lava_x      <= (lava_x  <= obstacle_speed)  ? HACTIVE : lava_x  - obstacle_speed;
                    ptr_x       <= (ptr_x   <= obstacle_speed)  ? HACTIVE : ptr_x   - obstacle_speed;
                    powerup_x   <= (powerup_x <= obstacle_speed)? HACTIVE : powerup_x - obstacle_speed;
                    sprite_state<= sprite_state + 1;

                    // Respawn one obstacle randomly for spacing
                    unique case (pick_obstacle(lfsr))
                        2'd0: if (s_cac_x == HACTIVE) begin passed_count++; end
                        2'd1: if (group_x == HACTIVE) begin passed_count++; end
                        2'd2: if (lava_x  == HACTIVE) begin passed_count++; end
                        2'd3: if (ptr_x   == HACTIVE) begin passed_count++; end
                    endcase

                    // Increase speed every 12
                    if (passed_count >= 12) begin
                        obstacle_speed <= obstacle_speed + 1;
                        passed_count   <= 0;
                    end
                end

                // Powerup collision -> Godzilla mode
                if (collide(dino_x,dino_y,powerup_x,powerup_y,32,32,32,32)) begin
                    is_godzilla    <= 1;
                    godzilla_timer <= MOTION_DIVIDE * 5;   // ~5 seconds
                    powerup_x      <= HACTIVE;
                end

                // Godzilla timer
                if (is_godzilla) begin
                    if (godzilla_timer > 0) godzilla_timer <= godzilla_timer - 1;
                    else                    is_godzilla    <= 0;
                end

                // Collisions (only if not in Godzilla)
                if (!is_godzilla && (
                    collide(dino_x,dino_y,s_cac_x,s_cac_y,32,32,32,32) ||
                    collide(dino_x,dino_y,group_x,group_y,32,32,150,40)||
                    collide(dino_x,dino_y,lava_x,lava_y,32,32,32,32)||
                    collide(dino_x,dino_y,ptr_x,ptr_y,32,32,32,32)
                )) begin
                    game_over <= 1;
                    state     <= S_GAMEOVER;
                end

                // While Godzilla: destroy obstacles on touch
                if (is_godzilla) begin
                    if (collide(dino_x,dino_y,s_cac_x,s_cac_y,32,32,32,32)) s_cac_x <= HACTIVE;
                    if (collide(dino_x,dino_y,group_x,group_y,32,32,150,40))  group_x <= HACTIVE;
                    if (collide(dino_x,dino_y,lava_x,lava_y,32,32,32,32)) lava_x <= HACTIVE;
                    if (collide(dino_x,dino_y,ptr_x,ptr_y,32,32,32,32))   ptr_x <= HACTIVE;
                end
            end

            S_GAMEOVER: begin
                if (controller_report[4]) begin
                    // reset entire game
                    state           <= S_PLAY;
                    game_over       <= 0;
                    is_godzilla     <= 0;
                    godzilla_timer  <= 0;
                    obstacle_speed  <= 1;
                    passed_count    <= 0;
                    s_cac_x <= 1200; group_x <= 1600; lava_x <= 1800;
                    ptr_x   <= 1400; powerup_x <= 1300;
                end
            end

        endcase
    end
end
        

always_comb begin
    if (is_godzilla) begin
        // Godzilla mode overrides everything
        dino_sprite_output = godzilla_sprite_output;
    end
    else begin
        case (sprite_state)
            2'd0: dino_sprite_output = dino_new_output;
            default: dino_sprite_output = dino_new_output;
        endcase
    end
end



//---------------------------------------------------------------------------
// Draw Logic: Pixels
//---------------------------------------------------------------------------
always_ff @(posedge clk) begin
    logic [7:0] r = 135, g = 206, b = 235;  // sky color

    if (!game_over) begin
        // Dino (with Godzilla sprite if powered)
        if (hcount>=dino_x && hcount<dino_x+32 && vcount>=dino_y && vcount<dino_y+32) begin
            dino_sprite_addr <= (hcount-dino_x) + ((vcount-dino_y)*32);
            if (is_visible(is_godzilla ? dino_sprite_output : dino_new_output)) begin
                {r,g,b} <= is_godzilla ?
                          {dino_sprite_output[15:11],3'b000,
                           dino_sprite_output[10:5],2'b00,
                           dino_sprite_output[4:0],3'b000} :
                          {dino_new_output[15:11],3'b000,
                           dino_new_output[10:5],2'b00,
                           dino_new_output[4:0],3'b000};
            end
        end

       // --- Small Cactus ---
if (hcount >= s_cac_x && hcount < s_cac_x + 32
    && vcount >= s_cac_y && vcount < s_cac_y + 32) begin
    scac_sprite_addr <= (hcount - s_cac_x)
                      + ((vcount - s_cac_y) * 32);
    if (is_visible(scac_sprite_output)) begin
        {r,g,b} <= {
            scac_sprite_output[15:11], 3'b000,
            scac_sprite_output[10:5],  2'b00,
            scac_sprite_output[4:0],   3'b000
        };
    end
end

// --- Group Cactus (150×40) ---
if (hcount >= group_x && hcount < group_x + 150
    && vcount >= group_y && vcount < group_y + 40) begin
    group_addr <= (hcount - group_x)
                + ((vcount - group_y) * 150);
    if (is_visible(group_output)) begin
        {r,g,b} <= {
            group_output[15:11], 3'b000,
            group_output[10:5],  2'b00,
            group_output[4:0],   3'b000
        };
    end
end

// --- Lava ---
if (hcount >= lava_x && hcount < lava_x + 32
    && vcount >= lava_y && vcount < lava_y + 32) begin
    lava_sprite_addr <= (hcount - lava_x)
                      + ((vcount - lava_y) * 32);
    if (is_visible(lava_output)) begin
        {r,g,b} <= {
            lava_output[15:11], 3'b000,
            lava_output[10:5],  2'b00,
            lava_output[4:0],   3'b000
        };
    end
end

// --- Pterodactyl (flapping) ---
if (hcount >= ptr_x && hcount < ptr_x + 32
    && vcount >= ptr_y && vcount < ptr_y + 32) begin
    ptr_sprite_addr <= (31 - (hcount - ptr_x))
                     + ((vcount - ptr_y) * 32);
    if (is_visible(ptr_sprite_output)) begin
        {r,g,b} <= {
            ptr_sprite_output[15:11], 3'b000,
            ptr_sprite_output[10:5],  2'b00,
            ptr_sprite_output[4:0],   3'b000
        };
    end
end

// --- Power-Up (Godzilla token) ---
if (hcount >= powerup_x && hcount < powerup_x + 32
    && vcount >= powerup_y && vcount < powerup_y + 32) begin
    powerup_sprite_addr <= (hcount - powerup_x)
                         + ((vcount - powerup_y) * 32);
    if (is_visible(powerup_sprite_output)) begin
        {r,g,b} <= {
            powerup_sprite_output[15:11], 3'b000,
            powerup_sprite_output[10:5],  2'b00,
            powerup_sprite_output[4:0],   3'b000
        };
    end
end

    end

    VGA_R <= r;
    VGA_G <= g;
    VGA_B <= b;
end
endmodule

//---------------------------------------------------------------------------
// VGA Counters Module
//---------------------------------------------------------------------------
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
