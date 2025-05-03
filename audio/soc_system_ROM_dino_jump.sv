

module soc_system_ROM_dino_jump #(
  parameter ADDR_WIDTH = 13,      // 2^13 = 8192 words, > MAX_JUMP=5000
  parameter DATA_WIDTH = 16
)(
  input  logic                      clk,
  input  logic [ADDR_WIDTH-1:0]    address,
  output logic [DATA_WIDTH-1:0]    readdata
);

  // Memory array
  logic [DATA_WIDTH-1:0] mem [(1<<ADDR_WIDTH)-1:0];

  // Initialize from hex file (one 16-bit word per line)
  initial begin
    $readmemh("dino_jump.hex", mem);
  end

  // Synchronous read
  always_ff @(posedge clk) begin
    readdata <= mem[address];
  end

endmodule
