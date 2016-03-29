`define WORD [15:0]
`define REGADDR [5:0]
`define REGSIZE [63:0]
`define MEMSIZE [32767:0]
`define ALUOP [3:0]

`define OPadd	4'b0000
`define OPinvf	4'b0001
`define OPaddf	4'b0010
`define OPmulf	4'b0011
`define OPand	4'b0100
`define OPor	4'b0101
`define OPxor	4'b0110
`define OPany	4'b0111
`define OPdup	4'b1000
`define OPshr	4'b1001
`define OPf2i	4'b1010
`define OPi2f	4'b1011 // last ALU op
`define OPld	4'b1100 // first non-ALU op
`define OPst	4'b1101
`define OPjzsz	4'b1110
`define OPli	4'b1111

module pipe(reset, clk);
    input reset, clk;

    reg `WORD pc;

    wire `WORD inst;
    wire `REGADDR src, dst;
    wire `ALUOP op;
    
    wire `WORD data_s, data_d;
    reg `WORD data_i;
    wire `REGADDR addr_i;
    reg write;

    wire `WORD data_o;
    wire wnotr;

    wire `WORD z;

    InstructionMemory im(inst, src, dst, op, pc, clk);
    RegisterFile rf(data_s, data_d, data_i, src, dst, addr_i, write, clk);
    DataMemory dm(data_o, data_s, data_d, wnotr, clk);
    Alu a(z, data_s, data_d, op);

    always @(reset) begin
        pc = 0;
        $display("0\t1\t2\t\t\t3");
        $display("pc\tinst\top\tsrc\tdst\tdata_s\tdata_d\tz\tdata_i");
    end

    always @(posedge clk) begin
        $display("%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h", 
                 pc, inst, op, src, dst, data_s, data_d, z, data_i);
        //$display("inc pc %d\tinstruction %h\t op src dst %h %h %h\tdata_s data_d %h %h", pc, inst, op, src, dst, data_s, data_d);
        if (op < `OPld) begin
            write <= 1;
            data_i <= z;
        end else begin
            write <= 0;
            data_i <= 16'bZ;
        end
        pc <= pc+1;
    end

    always @(negedge clk) begin
    end
endmodule

module InstructionMemory(inst, src, dst, op, addr, clk);
    output reg `WORD inst;
    output reg `REGADDR src, dst;
    output reg `ALUOP op;
    input `WORD addr;
    input clk;
    
    reg `WORD mem `MEMSIZE;
    
    always @(negedge clk) begin
        inst <= mem[addr];
        op <= inst[15:12];
        src <= inst[11:6];
        dst <= inst[5:0];
    end

    initial begin 
        $readmemh("prog_adds.out", mem);
    end 
endmodule

module RegisterFile(data_s, data_d, data_i, addr_s, addr_d, addr_i, write, clk);
    output reg `WORD data_s, data_d;
    input `WORD data_i;
    input `REGADDR addr_s, addr_d, addr_i;
    input write, clk;

    reg `WORD regs `REGSIZE;

    reg [5:0] i;
    
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
        i = 4;
        repeat (60) begin
            regs[i] = 0;
            i+=1;
        end
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

module Alu(z, x, y, op);
    output reg `WORD z;
    input `WORD x, y;
    input `ALUOP op;

    always @(x, y, op) begin
        //$display("ALU %h %h %h", y, op, x);
        case (op)
            `OPadd: z = y + x;
            `OPand: z = y & x;
            `OPor:  z = y | x;
            `OPxor: z = y ^ x;
            `OPany: z = (x ? 1 : 0);
            `OPshr: z = (x >> 1);
            default: z = x;
        endcase
    end
endmodule

module bench;
    reg clk, reset;
    pipe uut(reset, clk);

    initial begin
        $dumpfile("dump.vcd");
        $dumpvars(0, uut);
        #10 reset = 1;
        #10 reset = 0;
        while (1) begin
            #10 clk = 1;
            #10 clk = 0;
        end
    end

    always #1000 $finish();
endmodule
