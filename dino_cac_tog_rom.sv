module dino_cacti_together_rom (
    input  logic        clk,
    input  logic [11:0] address,          // More than enough for 2048 entries
    output logic [15:0] data              // Must be 16 bits for RGB565
);

    logic [15:0] memory [0:2047];         // Matches your 64×32 sprite

    initial begin
        $readmemh("better_cactus_64x32.hex", memory);
    end

    always_ff @(posedge clk) begin
        data <= memory[address];
    end
endmodule
