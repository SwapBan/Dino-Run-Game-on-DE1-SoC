module font_rom(
    input logic clk,
    input logic [3:0] address,  // 4-bit address to store 10 digits (0-9)
    output logic [31:0] data    // 32-bit wide data for 8x10 bitmap (one row of the 8x10 grid)
);
    always_ff @(posedge clk) begin
        case (address)
            4'd0: data <= 32'b11111111000000000000000000000000;  // Digit '0'
            4'd1: data <= 32'b01100011000011000000110000000000;  // Digit '1'
            4'd2: data <= 32'b11111110000000010000000000001111;  // Digit '2'
            4'd3: data <= 32'b11111110000000010001000000001111;  // Digit '3'
            4'd4: data <= 32'b10001111000001110000000000001110;  // Digit '4'
            4'd5: data <= 32'b11111100000000010001000011111100;  // Digit '5'
            4'd6: data <= 32'b11111100000000010001000011111110;  // Digit '6'
            4'd7: data <= 32'b11111000000000010000000000000001;  // Digit '7'
            4'd8: data <= 32'b11111110000000010001000011111111;  // Digit '8'
            4'd9: data <= 32'b11111110000000010001000011111110;  // Digit '9'
            default: data <= 32'b00000000000000000000000000000000;  // Default case for invalid address
        endcase
    end
endmodule
