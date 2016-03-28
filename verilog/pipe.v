`define WORD [15:0]
`define REGADDR [5:0]
`define REGSIZE [63:0]
`define MEMSIZE [32767:0]
`define ALUOP [3:0]

module pipe(reset, clk);
    input reset, clk;

    reg `WORD pc;

    wire `WORD inst;
    wire `REGADDR src, dst;
    
    wire `WORD data_s, data_d, data_i;
    wire `REGADDR addr_i;
    wire write;

    wire `WORD data_o;
    wire wnotr;

    InstructionMemory im(inst, src, dst, pc, clk);
    RegisterFile rf(data_s, data_d, data_i, src, dst, addr_i, write, clk);
    DataMemory dm(data_o, data_s, data_d, wnotr, clk);
    
    always @(reset) begin
        pc = 0;
    end

    always @(posedge clk) begin
        pc <= pc+1;
        $display("inc pc %d", pc);
        $display("instruction %h", inst);
    end
endmodule

module InstructionMemory(inst, src, dst, addr, clk);
    output reg `WORD inst;
    output reg `REGADDR src, dst;
    input `WORD addr;
    input clk;
    
    reg `WORD mem `MEMSIZE;
    
    always @(negedge clk) begin
        inst <= mem[addr];
        src <= inst[11:6];
        dst <= inst[5:0];
    end

    initial begin 
        $readmemh("im.vmem", mem);
    end 
endmodule

module RegisterFile(data_s, data_d, data_i, addr_s, addr_d, addr_i, write, clk);
    output reg `WORD data_s, data_d;
    input `WORD data_i;
    input `REGADDR addr_s, addr_d, addr_i;
    input write, clk;

    reg `WORD regs `REGSIZE;
    
    always @(negedge clk) begin
        data_s <= regs[addr_s];
        data_d <= regs[addr_d];
        if (write) begin
            regs[addr_i] <= data_i;
        end
    end

    initial begin
        regs[0] = 0;
        regs[1] = 1;
        regs[2] = 16'h8000;
        regs[3] = 16'hffff;
    end
endmodule

module DataMemory(data_o, addr, data_i, wnotr, clk);
    output reg `WORD data_o;
    input `WORD addr, data_i;
    input wnotr, clk;

    reg `WORD mem `MEMSIZE;

    always @(negedge clk) begin
        if (wnotr) begin
            mem[addr] <= data_i;
            data_o <= data_i;
        end else begin
            data_o <= mem[addr];
        end
    end

    initial begin
        $readmemh("dm.vmem", mem);
    end
endmodule

module ALU(z, x, y, op);
    output reg `WORD z;
    input `WORD x, y;
    input `ALUOP op;
endmodule

module bench;
    reg clk, reset;
    pipe uut(reset, clk);

    initial begin
        #10 reset = 1;
        #10 reset = 0;
        while (1) begin
            #10 clk = 1;
            #10 clk = 0;
        end
    end

    always #1000 $finish();
endmodule
