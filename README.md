Overview:
Dino Run on the DE1-SoC is similar to the Google T-Rex runner but comes with our own tweaks. Our project uses an external Controller (Dragonrise Gamepad) for gaming and also has an audio background. 
Hardware:.
Sprites: Renders Sprites such as dinosaur, small cactus, cacti together, lava, pterodactyl, power-up, and replay using ROMs.
Game Logic: Code handles obstacles, collisions, game speed up logics and has a “Godzilla mode” powerup. The game also switches between day and night backgrounds.
Audio: Streams 16-bit samples to left/right channels.

Software:
Memory-Mapped Input and Output: Maps registers for X,Y position of Dino and also duck, jump and replay mode.
Controller: Reads USB gamepad inputs using libusb.
Physics Loop: Integrates gravity and jumping, then writes updates into FPGA.


<img width="1266" height="1346" alt="image" src="https://github.com/user-attachments/assets/f3ea46c3-6465-4a2b-8f19-8be804fdba3a" />


Obstacle Movement Logic:

The obstacle positions are only updated every few milliseconds. Technically we are using a 50MHz clock so the obstacles should be moving 2,000,000/50,000,000 =0.04 seconds. Each obstacle moves in line to the left by a fixed number of pixels and resets to the right side of the screen (starting at 1280 pixels) when it hits the left edge. The new horizontal position of the obstacle is then calculated using a linear feedback shift register.

Motion timer variable checks if 2,000,000 cycles has been reached  (which happens every 0.04s). 

s_cac_x refers to the small cactus obstacle position (same for group_x => cacti together, lava_x => lava obstacle positon, ptr_x => pterodactyl position). 

If the obstacle_x position is less than the current obstacle speed that means it has crossed the left edge of the screen or it is at the left edge of the screen.

If the above condition is true, then we push the obstacle to HACTIVE(1280) + offset. This offset is calculated using a Linear Shift Feedback register.

If the condition is false, then change the obstacle_x position by obstacle_speed(set at 1 initially but keeps increasing with score).


Offset Calculation:
We use a linear feedback shift register to cycle through 63 numbers to give us a pseudo randomization. On each motion update the LFSR shifts its bits and reupdates using the XOR logic this makes sure that there is a new number that is generated every time. We then multiply the number by 16(for this sprite) which would mean that a new cactus spawns in 16 pixel increments somewhere between 1280 and 2288 pixels. 
LFSR starts at a non-zero value (LFSR cannot start at 0 or else it will keep giving output as 0) and updates only if the game is not over. 
Core of the LFSR logic –
            lfsr <= { lfsr[4:0], lfsr[5] ^ lfsr[4] };

This represents the polynomial - (x^6) + (x^5) + 1
This 6-bit register cycles through all possible 6 bit values except 0 in a random order. Hence, fetching us a random offset.

Initial lfsr = 6'b101011
            = b5 b4 b3 b2 b1 b0
            =  1  0  1  0  1  1

feedback = b5 ^ b4 = 1 ^ 0 = 1

new_lfsr = { b4, b3, b2, b1, b0, feedback }
         = { 0 , 1 , 0 , 1 , 1 , 1 }
         = 6'b010111

Gameplay Logic:

The mechanics of the game involve ducking, jumping, power-ups and game reset. The user input is captured using a USB Dragonrise Gamepad. The software code will capture whether the user wants to jump, duck, or restart the game. These are actions are stored as boolean values and written to memory mapped registers–

#define DINO_Y_OFFSET      (1 * 4)
#define DUCKING_OFFSET     (13 * 4)
#define JUMPING_OFFSET     (14 * 4)
#define REPLAY_OFFSET      (19 * 4)





Jumping Physics:

Jumping is implemented in software code. When a jump is triggered, a negative initial velocity is applied and gravity is added to it each cycle. The resulting dino position is then written back into the dino_y_reg:

  *dino_y_reg = y_fixed >> FIXED_SHIFT;

The above line converts the fixed-point value back to an integer (screen pixel) before writing to hardware.
 

Obstacle Movement:
Obstacles move towards the left edge of the screen every 0.04 seconds which is controlled by a motion timer variable. When the obstacle hits the left edge of the screen, it resets to a position to the right of the screen (HACTIVE + calculated randomized offset).

Collision Detection: 

The collision logic is handled in Verilog by bounding box comparisons between dino’s position and each obstacle –

function automatic logic collide(
        input logic [10:0] ax, ay, bx, by,
        input logic [5:0]  aw, ah, bw, bh
    );
        return ((ax < bx + bw) && (ax + aw > bx) &&
                (ay < by + bh) && (ay + ah > by));
    endfunction

If the collision occurs and the dino is not in godzilla mode, the game enter the game-over state where the replay screen comes up —

 if (!godzilla_mode &&(collide(dino_x, dino_y, s_cac_x,  s_cac_y,  32,32,32,32) ||
                collide(dino_x, dino_y, group_x,  group_y, 64,32,32,32) ||
                collide(dino_x, dino_y, lava_x,   lava_y,   32,32,32,32) ||
                                  collide(dino_x, dino_y, ptr_x,    ptr_y,    32,32,32,32))) begin
                game_over <= 1;
             
            end

The replay screen is displayed until the player presses the Start button. 

In Godzilla mode, which is activated by collecting powerups, the dino becomes invincible and destroys obstacles on contact (pushes them back by 2000 pixels).

Background:

Day and night transition: 

  // Night Timer Logic 
            if (night_timer < 40'd1_500_000_000) begin
            night_timer <= night_timer + 1;  // Increment the timer
         end else if (night_timer == 40'd1_500_000_000) begin
            night_time <= ~night_time;
             night_timer <= 32'd0;
         end
            if (night_time) begin
                sky_r <= 8'd10;  // Dark blue night sky
                sky_g <= 8'd10;
                sky_b <= 8'd40;      
            sun_r <= 8'd255; // White moon
            sun_g <= 8'd255;
            sun_b <= 8'd255;
            end else begin
                sky_r <= 8'd135;
                sky_g <= 8'd206;
                sky_b <= 8'd235; // Day sky color
                sun_r <= 8'd255; // Yellow sun
                sun_g <= 8'd255;
                sun_b <= 8'd0;
            end

We change between day and night by making use of a long timer. 

Powerup Logic:

if (collide(dino_x, dino_y, powerup_x, powerup_y, 32, 32, 32, 32)) begin
    godzilla_mode <= 1;
    godzilla_timer <= 0;
    powerup_x <= 2000; // move off screen
end
            //Godzilla destroys 
if (godzilla_mode) begin
    if (collide(dino_x, dino_y, s_cac_x, s_cac_y, 32, 32, 32, 32))
        s_cac_x <= 2000;
    if (collide(dino_x, dino_y, group_x, group_y, 64, 32, 32, 32))
        group_x <= 2000;
    if (collide(dino_x, dino_y, lava_x, lava_y, 32, 32, 32, 32))
        lava_x <= 2000;
    if (collide(dino_x, dino_y, ptr_x, ptr_y, 32, 32, 32, 32))
        ptr_x <= 2000;
end

Difficulty Progression:

if (s_cac_x <= obstacle_speed || group_x <= obstacle_speed ||
    lava_x  <= obstacle_speed || ptr_x   <= obstacle_speed) begin
    passed_count <= passed_count + 1;
end

if (passed_count >= 12) begin
    obstacle_speed <= obstacle_speed + 1;
    passed_count   <= 0;
end



Input Processing and Jump/Duck Physics:

bool want_jump = (y_axis == 0x00 && y == GROUND_Y);
bool want_duck = (y_axis == 0xFF && y == GROUND_Y);

if (want_jump) v = -12;
*jump_reg = want_jump;
*duck_reg = want_duck;

v += GRAVITY;
y += v;
if (y > GROUND_Y) { y = GROUND_Y; v = 0; }
*dino_y_reg = (uint32_t)y;








Audio:

Instantiated in our VGA ball module: 

input  logic        L_READY,
input  logic        R_READY,
output logic [15:0] L_DATA,
output logic [15:0] R_DATA,
output logic        L_VALID,
output logic        R_VALID

Audio control–
if (sample_clock >= 285) begin  // 50MHz / 175550 ≈ 285
    sample_clock <= 0;
    audio_sample <= audio_data[audio_index];

    if (audio_index == 9659)
        audio_index <= 0;
    else
        audio_index <= audio_index + 1;
end else begin
    sample_clock <= sample_clock + 1;
end

Audio streaming–
if (L_READY) begin
    L_DATA  <= audio_sample;
    L_VALID <= (sample_clock == 0);
end else begin
    L_VALID <= 0;
end

if (R_READY) begin
    R_DATA  <= audio_sample;
    R_VALID <= (sample_clock == 0);
end else begin
    R_VALID <= 0;
end

Looping–
audio_index <= (audio_index == 9659) ? 0 : audio_index + 1;


Display Logic:

Each sprite is stored in a .rom module that works like a look-up table. These are synthesized are read-only memory blocks on the FPGA. Sprites like the Dino and obstacles are stored in these roms. To get each pixel in the sprite image, we use –
scac_sprite_addr <= (hcount - s_cac_x) + ((vcount - s_cac_y) * 32);

