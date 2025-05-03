module soc_system_ROM_dino_jump #(
    parameter DEPTH       = 5000,                         // number of samples in your hex
    parameter ADDR_WIDTH  = $clog2(DEPTH)
)(
    input  logic                 clk,
    input  logic [ADDR_WIDTH-1:0] address,
    output logic [15:0]          data
);

    // storage
    logic [15:0] memory [0:DEPTH-1];

    // preload from hex file
    initial begin
        $readmemh("dino_jump.hex", memory);
    end

    // synchronous read
    always_ff @(posedge clk) begin
        data <= memory[address];
    end

endmodule
