module dino_sprite_rom (
    input  logic        clk,
    input  logic [9:0]  address,
    output logic [15:0] data
);

    logic [15:0] memory [0:255];

    initial begin
        $readmemh("dino_sprite.hex", memory);
    end

    always_ff @(posedge clk) begin
        data <= memory[address];
    end
endmodule


