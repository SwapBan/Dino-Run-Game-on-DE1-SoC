
module dino_cacti_together_rom (
    input  logic        clk,
    input  logic [10:0] address,          // Needs 11 bits for 2048 addresses
    output logic [15:0] data
);

    logic [15:0] memory [0:2047];         // Increased memory size to 2048

    initial begin
        $readmemh("better_cactus_64x32.hex", memory); // Use new sprite file
    end

    always_ff @(posedge clk) begin
        data <= memory[address];
    end
endmodule

