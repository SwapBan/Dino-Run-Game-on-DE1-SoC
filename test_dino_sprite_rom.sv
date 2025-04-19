module dino_sprite_rom (
    input  logic        clk,
    input  logic [9:0]  address,
    output logic [15:0] data
);

    // 32x32 sprite ROM = 1024 pixels
    logic [15:0] memory [0:1023];

    // For testing: Fill memory with red
    initial begin
        for (int i = 0; i < 1024; i++) begin
            memory[i] = 16'hF800; // Red color in RGB565
        end
    end

    always_ff @(posedge clk) begin
        data <= memory[address];
    end
endmodule
