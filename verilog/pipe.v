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

module pipe(halt, reset, clk);
    input reset, clk;
    output reg halt;

    //stage 0
    reg `WORD pc;

    //stage 1
    wire `WORD inst;
    wire `REGADDR src, dst;
    wire `ALUOP op;
    
    //stage 2
    wire `WORD data_s, data_d;
    reg `WORD data_i;
    reg `REGADDR addr_i;
    reg write;

    wire `WORD data_o;
    wire wnotr;

    wire `WORD z;

    //pipe regs
    //12
    reg wb_12;
    reg `ALUOP op_12;
    reg `REGADDR dst_12;
    reg wnotr_12;

    //23
    reg wb_23;
    reg `REGADDR dst_23;
    reg `WORD data_i_23;
    reg `ALUOP op_23;

    //for forwarding
    reg `WORD addr_s, addr_d;
    wire forward_s, forward_d;
    reg `WORD data_s_12, data_d_12;

    //for jumps
    reg [1:0] pcinc;

    InstructionMemory im(inst, src, dst, op, pc, clk);
    RegisterFile rf(data_s, data_d, data_i, src, dst, addr_i, write, clk);
    DataMemory dm(data_o, data_s_12, data_d_12, wnotr_12, clk);
    Alu a(z, data_s_12, data_d_12, op_12);

    always @(reset) begin
        pc = 0;
        $display("0\t\t1\t\t\t2\t\t\t3");
        $display("pc\tinst\top\tsrc\tdst\tdata_s\tdata_d\tz\tdata_o\tdata_i");
        write = 0;
        wb_12 = 0;
        halt = 0;
    end

    always @(posedge clk) begin
        $display("%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h\t%h", 
                 pc, inst, op, src, dst, data_s, data_d, z, data_o, data_i);
        //$display("inc pc %d\tinstruction %h\t op src dst %h %h %h\tdata_s data_d %h %h", pc, inst, op, src, dst, data_s, data_d);
        pcinc <= 1;
        case (op) 
            `OPadd:     begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPinvf:    begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPaddf:    begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPmulf:    begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPand:     begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPor:      begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPxor:     begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPany:     begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPdup:     begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPshr:     begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPf2i:     begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPi2f:     begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPld:      begin wb_12 <= 1; wnotr_12 <= 0; end
            `OPst:      begin wb_12 <= 0; wnotr_12 <= 1; end
            `OPjzsz:    begin 
                wb_12 <= 0;
                wnotr_12 <= 0;
                case (addr_s)
                    0:          begin halt <= 1; end
                    1:          begin pc <= pc+2; end
                    default:    begin end
                endcase
                        end
            `OPli:      begin wb_12 <= 0; wnotr_12 <= 0; end
            default:    begin
                $display("halt");
                //halt <= 1;
            end
        endcase
        
        //wb_12 <= (op !== 1'bx && op < `OPst ? 1 : 0);
        data_s_12 <= (write && addr_i == addr_s ? data_i : data_s);
        data_d_12 <= (write && addr_i == addr_d ? data_i : data_d);
    end

    always @(negedge clk) begin
        pc <= pc+pcinc;

        wb_23 <= wb_12;
        write <= wb_23;

        op_12 <= op;
        op_23 <= op_12;

        addr_i <= dst_12;
        dst_12 <= dst;
        //addr_i <= dst_23;

        data_i <= (op_12 == `OPld ? data_o : z);
        //data_i <= data_o;
        //data_i <= z;
        //data_i <= data_i_23;

        addr_s <= src;
        addr_d <= dst;
        
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
    end

    always @(inst) begin
        op <= inst[15:12];
        dst <= inst[11:6];
        src <= inst[5:0];
    end

    initial begin 
        $readmemh("prog_jumps.text.vmem", mem);
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
            $display("write %h", data_i);
            regs[addr_i] <= data_i;
            $display("%h", regs[addr_i]);
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
    wire halt;
    pipe uut(halt, reset, clk);

    initial begin
        $dumpfile("dump.vcd");
        $dumpvars(0, uut);
        #10 reset = 1;
        #10 reset = 0;
        while (!halt) begin
            #10 clk = 1;
            #10 clk = 0;
        end
    end
    
    always #1000 $finish();
endmodule
