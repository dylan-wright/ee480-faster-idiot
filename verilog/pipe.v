`define WORD [15:0]
`define REGADDR [5:0]
`define REGSIZE [63:0]
`define MEMSIZE [32767:0]
`define ALUOP [3:0]

module pipe(reset, clk);
    input reset, clk;


endmodule

module InstructionMemory(instruction, src, dst, addr, clk);
    output reg `WORD instruction;
    output reg `REGADDR src, dst;
    input `WORD addr;
    input clk;
    
    reg `WORD mem `MEMSIZE;
    
    assign src = instruction[:];
    assign dst = instruction[:];

    always @(negedge clk) begin
        instruction <= mem[addr];
    end
endmodule

module RegisterFile(data_s, data_d, data_i, addr_s, addr_d, addr_i, clk);
    output reg `WORD data_s, data_d;
    input `WORD data_i;
    input `REGADDR addr_s, addr_d, addr_i;
    input clk;

    reg `WORD regs `REGSIZE;
    
    always @(negedge clk) begin
        data_s <= regs[addr_s];
        data_d <= regs[addr_d];
        if (data_i != 16'bZ) begin
            regs[addr_i] <= data_i;
        end
    end
endmodule

module DataMemory(data_o, addr, data_i, clk);
    output reg `WORD data_o;
    input `WORD addr, data_i;
    input clk;

    reg `WORD mem `MEMSIZE;
endmodule

module ALU(z, x, y, op);
    output reg `WORD z;
    input `WORD x, y;
    input `ALUOP op;
endmodule
