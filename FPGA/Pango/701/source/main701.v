`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    12:37:57 06/08/2018 
// Design Name: 
// Module Name:    main701 
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
module main701(
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
    input GPIO,		//GPIO3
	 input SYNC,		//GPIO0
	 input SHUTDOWN,	//GPIO1
	 input HW_RST,		//GPIO2
	 input [15:0] OP_IN,
     output [1:0] FPGA_LED
    );
	 
parameter OP_FILTER = 16'd10;			//光纤信号滤波 100ns
parameter PANEL_FILTER = 16'd10;		//背板信号滤波 100ns

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

//reg [15:0] reg_SHUTDOWN = 16'hFFFF;
reg [15:0] reg_FO_EN = 16'h0000;  // DSP 写入 FO_EN 后启用对应通道

//reg [15:0] reg_DSP_CTL;

//reg [15:0] reg_DIN;
//reg [15:0] reg_DIN_LATCH;
//reg [15:0] reg_AD_PROT;
//reg [15:0] reg_AD_PROT_LATCH;
reg [15:0] reg_FO;
reg [15:0] reg_FO_LATCH;

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
	if(~CS & RD_clk)	//DSP Read
		begin
			case(XA)
			8'h00: data <= reg_ID;
			8'h01: data <= reg_LED;

			8'h23: data <= reg_FO_EN;
			
			8'h3A: data <= reg_TEST3A;
			8'h3B: data <= reg_TEST3B;
			
//			8'hE0: data <= reg_DIN;
//			8'hE1: data <= reg_DIN_LATCH;
//			8'hE2: data <= reg_AD_PROT;
//			8'hE3: data <= reg_AD_PROT_LATCH;
			8'hE4: data <= reg_FO;
			8'hE5: data <= reg_FO_LATCH;
			default: data <= data;
			endcase
		end
	else if(~CS & WE_clk)	//DSP Write
		begin
			case(XA)	
			8'h00: reg_ID <= XD;
			8'h01: reg_LED <= XD;

			8'h23: reg_FO_EN <= XD;
			
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


// AFBR-2529Z 输出直连 FPGA，亮光时为低电平，失光时为高电平，FO 高有效。
wire [15:0] fo;
//PROT
antishake #(OP_FILTER) OP_IN0
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[0]),
	 .OUTPUT(fo[0])
);
antishake #(OP_FILTER) OP_IN1
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[1]),
	 .OUTPUT(fo[1])
);
antishake #(OP_FILTER) OP_IN2
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[2]),
	 .OUTPUT(fo[2])
);
antishake #(OP_FILTER) OP_IN3
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[3]),
	 .OUTPUT(fo[3])
);
antishake #(OP_FILTER) OP_IN4
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[4]),
	 .OUTPUT(fo[4])
);
antishake #(OP_FILTER) OP_IN5
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[5]),
	 .OUTPUT(fo[5])
);
antishake #(OP_FILTER) OP_IN6
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[6]),
	 .OUTPUT(fo[6])
);
antishake #(OP_FILTER) OP_IN7
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[7]),
	 .OUTPUT(fo[7])
);
antishake #(OP_FILTER) OP_IN8
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[8]),
	 .OUTPUT(fo[8])
);
antishake #(OP_FILTER) OP_IN9
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[9]),
	 .OUTPUT(fo[9])
);
antishake #(OP_FILTER) OP_INA
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[10]),
	 .OUTPUT(fo[10])
);
antishake #(OP_FILTER) OP_INB
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[11]),
	 .OUTPUT(fo[11])
);
antishake #(OP_FILTER) OP_INC
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[12]),
	 .OUTPUT(fo[12])
);
antishake #(OP_FILTER) OP_IND
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[13]),
	 .OUTPUT(fo[13])
);
antishake #(OP_FILTER) OP_INE
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[14]),
	 .OUTPUT(fo[14])
);
antishake #(OP_FILTER) OP_INF
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(OP_IN[15]),
	 .OUTPUT(fo[15])
);

wire bk_lock, bk_unlock;
reg BK_R1, BK_R2, BK_R3;

always @ (posedge CLK_100M)
begin
	  BK_R1 <= BK;
	  BK_R2 <= BK_R1;
	  BK_R3 <= BK_R2;
end

assign bk_lock = (~BK_R3)&(BK_R2)&(BK_R1); 
assign bk_unlock = (BK_R3)&(~BK_R2)&(~BK_R1); 


wire fo_rst;

antishake #(PANEL_FILTER) FO_RST
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(HW_RST),
	 .OUTPUT(fo_rst)
);

reg fo_block = 1'b0;

always @ (posedge CLK_100M)
begin
	reg_FO <= fo & reg_FO_EN;		//FO_EN不使能（等于0）时，无论光纤输入情况如何，FO始终为低
	
	if(~fo_block)
	begin
		 fo_block <= |reg_FO;					//can't auto recovery fault
		 reg_FO_LATCH <= reg_FO;
	end
	else
	    fo_block <= (fo_block & ~fo_rst) | (|reg_FO);		
end

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

//assign LED = reg_LED;
assign LED = reg_LED | {TEST3B_FAULT, 1'b0, TEST3B_FAULT, 1'b0, TEST3B_FAULT, 1'b0, TEST3B_FAULT, 1'b0};
assign FB = fo_block;

wire [3:0]op_state;
assign op_state = 4'b0001;
dual_led led_inst(
    .clk(CLK_100M),
   .rst_n(1'b1),
	.op_state(op_state),
	.led(FPGA_LED)
);
endmodule
