
module tone_gen(
    input logic clk,
    input logic reset,
    output logic signed [15:0] audio_out
);

    parameter TONE_PERIOD = 56818; // 50MHz / (2 * 440Hz) ~ 56818

    logic [31:0] counter;
    logic tone;

    always_ff @(posedge clk or posedge reset) begin
        if (reset) begin
            counter <= 0;
            tone <= 0;
        end else begin
            if (counter >= TONE_PERIOD) begin
                counter <= 0;
                tone <= ~tone;
            end else begin
                counter <= counter + 1;
            end
        end
    end

    assign audio_out = tone ? 16'sh7FFF : -16'sh8000;

endmodule
