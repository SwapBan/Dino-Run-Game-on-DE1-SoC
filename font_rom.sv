module font_rom(
    input logic clk,
    input logic [3:0] address,  // 4-bit address to store 10 digits (0-9)
    output logic [15:0] data    // 16-bit wide data for 5x7 bitmap (one row of the 5x7 grid)
);
    always_ff @(posedge clk) begin
        case (address)
            4'd0: data <= 16'b111110000100010001000100011111;  // Digit '0'
            4'd1: data <= 16'b011001110001110001000001111110;  // Digit '1'
            4'd2: data <= 16'b11111000010001000100000100000111;  // Digit '2'
            4'd3: data <= 16'b111110000100010000100010011111;  // Digit '3'
            4'd4: data <= 16'b10001000100010001111110001000100;  // Digit '4'
            4'd5: data <= 16'b11111100010000010001000011111100;  // Digit '5'
            4'd6: data <= 16'b11111100010000010001000111111000;  // Digit '6'
            4'd7: data <= 16'b11111000010001000010000010000001;  // Digit '7'
            4'd8: data <= 16'b11111100010001000100010011111111;  // Digit '8'
            4'd9: data <= 16'b11111100010001000010000011111111;  // Digit '9'
            default: data <= 16'b0000000000000000;  // Default case for invalid address
        endcase
    end
endmodule
