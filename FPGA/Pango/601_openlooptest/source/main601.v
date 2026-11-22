`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    09:47:47 06/08/2018 
// Design Name: 
// Module Name:    main601 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module main601(
    input CLK20M_IN,
    input [7:0] XA,
    inout [15:0] XD,
    input WE,
    input RD,
    input CS,
    output DIR,
    input BK,
    output FB,
    output [7:0] LED,
    input IO_A,
    input IO_B,
    input IO_C,
    input [3:2] GPIO,
	 input SYNC,
	 input SHUTDOWN,
	 output [15:0] OP_OUT,
    output [1:0] FPGA_LED
    );
	 
parameter PANEL_FILTER = 16'd100;		//背板信号滤波 1us

wire CLK_100M;
clock clock_100M (
    .clkout0(CLK_100M), // output
    .lock(CLK_100M_LOCKED), // output
    .clkin1(CLK20M_IN) // input
);

//注释掉其他插件上的寄存器
reg [15:0] data;
reg [15:0] reg_ID;
reg [15:0] reg_LED;
reg [15:0] reg_TEST3A = 16'h52FF;
reg [15:0] reg_TEST3B = 16'h52FF;

//reg [15:0] reg_DOUT;
reg [15:0] reg21;
reg [15:0] reg22;
reg [15:0] reg23;
reg [15:0] reg24;
reg [15:0] reg25;
reg [15:0] reg26;
reg [15:0] reg27;
//reg [15:0] reg_AD_PROT_EN;
reg [15:0] reg29;
reg [15:0] reg2A;
reg [15:0] reg2B;
reg [15:0] reg2C;
reg [15:0] reg2D;
reg [15:0] reg2E;
reg [15:0] reg2F;

// 保持所有发射口熄灭，直到 DSP 写入各模块的关断位图。
reg [15:0] reg_SHUTDOWN = 16'hFFFF;
//reg [15:0] reg_FO_EN;

//reg [15:0] reg_DSP_CTL;

reg WE_R1, WE_R2, WE_R3;
reg RD_R1, RD_R2, RD_R3;
reg CS_R1, CS_R2, CS_R3;
reg RD_clk = 1'b0;
reg WE_clk = 1'b0;

always @ (posedge CLK_100M)
begin
	WE_R3 <= WE_R2;
	RD_R3 <= RD_R2;
	CS_R3 <= CS_R2;
	
	WE_R2 <= WE_R1;
	RD_R2 <= RD_R1;
	CS_R2 <= CS_R1;
	
	WE_R1 <= WE;
	RD_R1 <= RD;
	CS_R1 <= CS;
	
	RD_clk <= RD_R2 & ~RD_R1;
	WE_clk <= WE_R2 & ~WE_R1;
end

always @ (posedge CLK_100M)
begin
	if(~CS_R2 & RD_clk)	//DSP Read
		begin
			case(XA)
			8'h00: data <= reg_ID;
			8'h01: data <= reg_LED;

			8'h22: data <= reg_SHUTDOWN;
			8'h3A: data <= reg_TEST3A;
			8'h3B: data <= reg_TEST3B;
			default: data <= data;
			endcase
		end
	else if(~CS_R2 & WE_clk)	//DSP Write
		begin
			case(XA)	
			8'h00: reg_ID <= XD;
			8'h01: reg_LED <= XD;

			8'h22: reg_SHUTDOWN <= XD;
			8'h3B: reg_TEST3B <= XD;
			default: data <= data;
			endcase
		end
	else
		begin
			data <= data;
		end
end

assign XD = (~RD_R3 & ~CS_R3) ? data : 16'hzzzz;
assign DIR = ~RD_R3 & ~CS_R3;

wire shut;
antishake #(PANEL_FILTER) shutdown
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(SHUTDOWN),
	 .OUTPUT(shut)
);

//读写故障测试
always @ (posedge CLK_100M)
begin
	if(CS_R2)	
	begin
		if(reg_TEST3A == 16'h52FF)	reg_TEST3A <= 16'h5300;
		else reg_TEST3A <= 16'h52FF;
	end
end

reg TEST3B_FAULT = 1'b0;

always @ (posedge CLK_100M)
begin
	if(((reg_TEST3B == 16'h52FF || reg_TEST3B == 16'h5300) && TEST3B_FAULT == 1'b0) || reg_LED == 16'h00EB)		TEST3B_FAULT <= 1'b0;
	else TEST3B_FAULT <= 1'b1;
end

// AFBR-1629Z 高电平无光。总关断熄灭所有口，本地关断位只熄灭对应口。
assign OP_OUT = reg_SHUTDOWN | {16{shut}};
assign LED = reg_LED | {TEST3B_FAULT, 1'b0, TEST3B_FAULT, 1'b0, TEST3B_FAULT, 1'b0, TEST3B_FAULT, 1'b0};
assign FB = 1'b0;

//test
//assign OP_OUT ={FPGA_LED,FPGA_LED,FPGA_LED,FPGA_LED,FPGA_LED,FPGA_LED,FPGA_LED,FPGA_LED};

wire [3:0]op_state;
assign op_state = 4'b0001;
dual_led led_inst(
    .clk(CLK_100M),
   .rst_n(1'b1),
	.op_state(op_state),
	.led(FPGA_LED)
);

endmodule
