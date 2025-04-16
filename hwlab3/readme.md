module dino_sprite_rom (
    input  logic        clk,
    input  logic [9:0]  address,
    output logic [15:0] data
);

    logic [15:0] memory [0:1023];  // Assuming 32x32 sprite

    initial begin
        $readmemh("dino_sprite.mif", memory);
    end

    always_ff @(posedge clk) begin
        data <= memory[address];
    end
endmodule




Error (10170): Verilog HDL syntax error at dino_sprite.mif(2) near text: -. Check for and fix any syntax errors that appear immediately before or at the specified keyword. The Intel FPGA Knowledge Database contains many articles with specific details on how to resolve this error. Visit the Knowledge Database at https://www.altera.com/support/support-resources/knowledge-base/search.html and search for this specific error message number. File: /homes/user/stud/spring25/as7525/Music/lab3-hw/dino_sprite.mif Line: 2
Error (12152): Can't elaborate user hierarchy "soc_system:soc_system0|vga_ball:vga_ball_0|dino_sprite_rom:dino_rom" File: /homes/user/stud/spring25/as7525/Music/lab3-hw/soc_system/synthesis/submodules/vga_ball.sv Line: 52





module dino_sprite_rom (
    input  logic        clk,
    input  logic [9:0]  address,
    output logic [15:0] data
);

    logic [15:0] memory [0:1023];

    initial begin
        $readmemh("dino_sprite.mif", memory);
    end

    always_ff @(posedge clk) begin
        data <= memory[address];
    end
endmodule


Error (12006): Node instance "dino_rom" instantiates undefined entity "dino_sprite_rom". Ensure that required library paths are specified correctly, define the specified entity, or change the instantiation. If this entity represents Intel FPGA or third-party IP, generate the synthesis files for the IP. File: /homes/user/stud/spring25/as7525/Music/lab3-hw/soc_system/synthesis/submodules/vga_ball.sv Line: 52




\


scp as7525@micro31.ee.columbia.edu:lab3-hw/output_files/soc_system.rbf /mnt
scp as7525@micro31.ee.columbia.edu:lab4-hw/soc_system.dtb /mnt


