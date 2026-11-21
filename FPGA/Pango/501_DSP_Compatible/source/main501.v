`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    15:42:23 05/31/2018 
// Design Name: 
// Module Name:    main501 
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
module main501(
    input CLK20M_IN,
    input [7:0] XA,
    inout [15:0] XD,
    input  WE,
    input  RD,
    input  CS,
	 output DIR,
	 input BK,
	 output FB,
	 output [7:0] LED,
	 input IO_A,
	 input IO_B,
	 input IO_C,
	 input [3:1] GPIO,
	 input [7:0] RX,
	 output [7:0] TX,
	 input SYNC_IN,
    output [1:0] FPGA_LED
	 //SFP
//	 output SFP_FAULT_LED,
//	 output LANE_UP_LED,
//	 output CHANNEL_UP_LED,
//	 output FRAME_ERR_LED,
//	 output SOFT_ERR_LED,
//	 output HARD_ERR_LED
    );


parameter BK_FILTER = 16'd30000;			//BK信号延时300us

wire CLK_100M;
wire CLK_100M_LOCKED;
//clock CLK(.CLK_IN(CLK20M_IN), .CLK_OUT(CLK_100M));
clock clock_100M (
    .clkout0(CLK_100M), // output
    .lock(CLK_100M_LOCKED), // output
    .clkin1(CLK20M_IN) // input
);


wire latch_BK;

antishake #(BK_FILTER) LATCH_BK
(
	 .CLK(CLK_100M),
	 .RESET(1'b0),
	 .INPUT(BK),
	 .OUTPUT(latch_BK)
);

//板上LED测试
/*
reg [7:0] reg_test_led = 8'h01;
reg [31:0] cnt = 32'h00000000;
always @ (posedge CLK_100M)
begin
	if(cnt == 32'd100000000)	
	begin
		cnt <= 32'd0;
		reg_test_led[7] <= reg_test_led[6];
		reg_test_led[6] <= reg_test_led[5];
		reg_test_led[5] <= reg_test_led[4];
		reg_test_led[4] <= reg_test_led[3];
		reg_test_led[3] <= reg_test_led[2];
		reg_test_led[2] <= reg_test_led[1];
		reg_test_led[1] <= reg_test_led[0];
		reg_test_led[0] <= reg_test_led[7];
	end
	else 
	begin
		cnt <= cnt + 32'd1;
	end
end

assign LED = reg_test_led;*/

//同步信号
reg[2:0] sync_r = 3'b000;

always @ (posedge CLK_100M)
begin
	sync_r <= {sync_r[1:0], SYNC_IN};
end

wire 	pos_sync = ~sync_r[2] & sync_r[1];


//注释掉其他插件上的寄存器
reg [15:0] data;
reg [15:0] reg_ID = 16'h0000;
reg [15:0] reg_LED;
reg [15:0] reg_NUM;
reg [15:0] reg_PM = 16'h0000;	// PM 由 DSP 写入，低八位分别配置八路 UNIT。
reg [15:0] reg_REF;
reg [15:0] reg_DEBUG_NO;
reg [15:0] reg_DC_LOG = 16'h0000;
reg [15:0] reg_TAD = 16'd5000;
reg [15:0] reg08;
reg [15:0] reg09;
reg [15:0] reg_DAB_DUTY_L = 16'd2500;
reg [15:0] reg_DAB_DUTY_R = 16'd2500;
reg [15:0] reg_DAB_T = 16'd1250;
reg [15:0] reg_PM_CTL = 16'h0000;
reg [15:0] reg_TX_EN = 16'hFFFF;
reg [15:0] reg_TX_CTL = 16'd0;

wire [1:0] DAB_DIV;
assign DAB_DIV		= reg_PM_CTL[6:5];
reg [15:0] DAB_TAD = 16'd4999;	//DAB的开关周期

reg [15:0] DAB_DUTY_L_0 = 16'd2500;
reg [15:0] DAB_DUTY_L_1 = 16'd2500;
reg [15:0] DAB_DUTY_L_2 = 16'd2500;
reg [15:0] DAB_DUTY_L_3 = 16'd2500;
reg [15:0] DAB_DUTY_L_4 = 16'd2500;
reg [15:0] DAB_DUTY_L_5 = 16'd2500;
reg [15:0] DAB_DUTY_L_6 = 16'd2500;
reg [15:0] DAB_DUTY_L_7 = 16'd2500;

reg [15:0] DAB_DUTY_R_0 = 16'd2500;
reg [15:0] DAB_DUTY_R_1 = 16'd2500;
reg [15:0] DAB_DUTY_R_2 = 16'd2500;
reg [15:0] DAB_DUTY_R_3 = 16'd2500;
reg [15:0] DAB_DUTY_R_4 = 16'd2500;
reg [15:0] DAB_DUTY_R_5 = 16'd2500;
reg [15:0] DAB_DUTY_R_6 = 16'd2500;
reg [15:0] DAB_DUTY_R_7 = 16'd2500;

reg [15:0] DAB_T_0 = 16'd1250;
reg [15:0] DAB_T_1 = 16'd1250;
reg [15:0] DAB_T_2 = 16'd1250;
reg [15:0] DAB_T_3 = 16'd1250;
reg [15:0] DAB_T_4 = 16'd1250;
reg [15:0] DAB_T_5 = 16'd1250;
reg [15:0] DAB_T_6 = 16'd1250;
reg [15:0] DAB_T_7 = 16'd1250;

always @ (posedge CLK_100M)
begin
	if(pos_sync)
	begin
		case(DAB_DIV)
			2'b00:	DAB_TAD <= 16'd4999;
			2'b01:	DAB_TAD <= 16'd2499;
			2'b10:	DAB_TAD <= 16'd1249;
			2'b11:	DAB_TAD <= 16'd624;
		endcase
		// 0x0A~0x0C 为全板 DAB 参数，0xE8~0xEF 为各 UNIT 的移相微调。
		DAB_DUTY_L_0 <= (reg_DAB_DUTY_L>>DAB_DIV);
		DAB_DUTY_L_1 <= (reg_DAB_DUTY_L>>DAB_DIV);
		DAB_DUTY_L_2 <= (reg_DAB_DUTY_L>>DAB_DIV);
		DAB_DUTY_L_3 <= (reg_DAB_DUTY_L>>DAB_DIV);
		DAB_DUTY_L_4 <= (reg_DAB_DUTY_L>>DAB_DIV);
		DAB_DUTY_L_5 <= (reg_DAB_DUTY_L>>DAB_DIV);
		DAB_DUTY_L_6 <= (reg_DAB_DUTY_L>>DAB_DIV);
		DAB_DUTY_L_7 <= (reg_DAB_DUTY_L>>DAB_DIV);
		
		DAB_DUTY_R_0 <= (reg_DAB_DUTY_R>>DAB_DIV);
		DAB_DUTY_R_1 <= (reg_DAB_DUTY_R>>DAB_DIV);
		DAB_DUTY_R_2 <= (reg_DAB_DUTY_R>>DAB_DIV);
		DAB_DUTY_R_3 <= (reg_DAB_DUTY_R>>DAB_DIV);
		DAB_DUTY_R_4 <= (reg_DAB_DUTY_R>>DAB_DIV);
		DAB_DUTY_R_5 <= (reg_DAB_DUTY_R>>DAB_DIV);
		DAB_DUTY_R_6 <= (reg_DAB_DUTY_R>>DAB_DIV);
		DAB_DUTY_R_7 <= (reg_DAB_DUTY_R>>DAB_DIV);
		
		DAB_T_0	<= ((reg_DAB_T+reg_DAB_T_MK_0)>>DAB_DIV);
		DAB_T_1	<= ((reg_DAB_T+reg_DAB_T_MK_1)>>DAB_DIV);
		DAB_T_2	<= ((reg_DAB_T+reg_DAB_T_MK_2)>>DAB_DIV);
		DAB_T_3	<= ((reg_DAB_T+reg_DAB_T_MK_3)>>DAB_DIV);
		DAB_T_4	<= ((reg_DAB_T+reg_DAB_T_MK_4)>>DAB_DIV);
		DAB_T_5	<= ((reg_DAB_T+reg_DAB_T_MK_5)>>DAB_DIV);
		DAB_T_6	<= ((reg_DAB_T+reg_DAB_T_MK_6)>>DAB_DIV);
		DAB_T_7	<= ((reg_DAB_T+reg_DAB_T_MK_7)>>DAB_DIV);
	end
end

reg [15:0] reg_TX_0;
reg [15:0] reg_TX_1;
reg [15:0] reg_TX_2;
reg [15:0] reg_TX_3;
reg [15:0] reg_TX_4;
reg [15:0] reg_TX_5;
reg [15:0] reg_TX_6;
reg [15:0] reg_TX_7;

//reg [15:0] reg_DOUT;
//reg [15:0] reg_AD_PROT_EN;
//reg [15:0] reg_SHUTDOWN;
//reg [15:0] reg_FO_EN;
reg [15:0] reg_INV_SHUTDOWN;
reg [15:0] reg_INV_FO_EN;
reg [15:0] reg_INV_PWM;

reg [15:0] reg27;
reg [15:0] reg28;
reg [15:0] reg29;
reg [15:0] reg2A;
reg [15:0] reg2B;
reg [15:0] reg2C;
reg [15:0] reg2D;
reg [15:0] reg2E;
reg [15:0] reg2F;



reg [15:0] reg32;
reg [15:0] reg33;
reg [15:0] reg34;
reg [15:0] reg35;
reg [15:0] reg36;
reg [15:0] reg37;
//reg [15:0] reg_BKU;
//reg [15:0] reg_FB_EN;
reg [15:0] reg_TEST3A = 16'h52FF;
reg [15:0] reg_TEST3B = 16'h52FF;
reg [15:0] reg3E;
//reg [15:0] reg_DSP_CTL;

/*
reg [15:0] reg_HVAC_STATUS;
reg [15:0] reg_HVAC_D1;
reg [15:0] reg_HVAC_D2;
reg [15:0] reg_HVAC_D3;
reg [15:0] reg_HVDC_STATUS;
reg [15:0] reg_HVDC_D1;
reg [15:0] reg_HVDC_D2;
reg [15:0] reg_HVDC_D3;
reg [15:0] reg_LVDC_STATUS;
reg [15:0] reg_LVDC_D1;
reg [15:0] reg_LVDC_D2;
reg [15:0] reg_LVDC_D3;
reg [15:0] reg_LVAC_STATUS;
reg [15:0] reg_LVAC_D1;
reg [15:0] reg_LVAC_D2;
reg [15:0] reg_LVAC_D3;
*/

/*
reg [15:0] reg_AD_0;
reg [15:0] reg_AD_1;
reg [15:0] reg_AD_2;
reg [15:0] reg_AD_3;
reg [15:0] reg_AD_4;
reg [15:0] reg_AD_5;
reg [15:0] reg_AD_6;
reg [15:0] reg_AD_7;
reg [15:0] reg_AD_8;
reg [15:0] reg_AD_9;
reg [15:0] reg_AD_A;
reg [15:0] reg_AD_B;
reg [15:0] reg_AD_C;
reg [15:0] reg_AD_D;
reg [15:0] reg_AD_E;
reg [15:0] reg_AD_F;

reg [15:0] reg_AD_LATCH_0;
reg [15:0] reg_AD_LATCH_1;
reg [15:0] reg_AD_LATCH_2;
reg [15:0] reg_AD_LATCH_3;
reg [15:0] reg_AD_LATCH_4;
reg [15:0] reg_AD_LATCH_5;
reg [15:0] reg_AD_LATCH_6;
reg [15:0] reg_AD_LATCH_7;
reg [15:0] reg_AD_LATCH_8;
reg [15:0] reg_AD_LATCH_9;
reg [15:0] reg_AD_LATCH_A;
reg [15:0] reg_AD_LATCH_B;
reg [15:0] reg_AD_LATCH_C;
reg [15:0] reg_AD_LATCH_D;
reg [15:0] reg_AD_LATCH_E;
reg [15:0] reg_AD_LATCH_F;
*/
reg [15:0] reg_DC_LATCH_0 = 16'h0000;
reg [15:0] reg_DC_LATCH_1 = 16'h0000;
reg [15:0] reg_DC_LATCH_2 = 16'h0000;
reg [15:0] reg_DC_LATCH_3 = 16'h0000;
reg [15:0] reg_DC_LATCH_4 = 16'h0000;
reg [15:0] reg_DC_LATCH_5 = 16'h0000;
reg [15:0] reg_DC_LATCH_6 = 16'h0000;
reg [15:0] reg_DC_LATCH_7 = 16'h0000;

reg [15:0] reg_IR_LATCH_0 = 16'h0000;
reg [15:0] reg_IR_LATCH_1 = 16'h0000;
reg [15:0] reg_IR_LATCH_2 = 16'h0000;
reg [15:0] reg_IR_LATCH_3 = 16'h0000;
reg [15:0] reg_IR_LATCH_4 = 16'h0000;
reg [15:0] reg_IR_LATCH_5 = 16'h0000;
reg [15:0] reg_IR_LATCH_6 = 16'h0000;
reg [15:0] reg_IR_LATCH_7 = 16'h0000;

reg [15:0] reg_IG_LATCH_0 = 16'h0000;
reg [15:0] reg_IG_LATCH_1 = 16'h0000;
reg [15:0] reg_IG_LATCH_2 = 16'h0000;
reg [15:0] reg_IG_LATCH_3 = 16'h0000;
reg [15:0] reg_IG_LATCH_4 = 16'h0000;
reg [15:0] reg_IG_LATCH_5 = 16'h0000;
reg [15:0] reg_IG_LATCH_6 = 16'h0000;
reg [15:0] reg_IG_LATCH_7 = 16'h0000;

reg [15:0] reg_TEMP_LATCH_0 = 16'h0000;
reg [15:0] reg_TEMP_LATCH_1 = 16'h0000;
reg [15:0] reg_TEMP_LATCH_2 = 16'h0000;
reg [15:0] reg_TEMP_LATCH_3 = 16'h0000;
reg [15:0] reg_TEMP_LATCH_4 = 16'h0000;
reg [15:0] reg_TEMP_LATCH_5 = 16'h0000;
reg [15:0] reg_TEMP_LATCH_6 = 16'h0000;
reg [15:0] reg_TEMP_LATCH_7 = 16'h0000;


reg [15:0] reg_STATUS_DUMMY_0 = 16'd0;
reg [15:0] reg_STATUS_DUMMY_1 = 16'd0;
reg [15:0] reg_STATUS_DUMMY_2 = 16'd0;
reg [15:0] reg_STATUS_DUMMY_3 = 16'd0;
reg [15:0] reg_STATUS_DUMMY_4 = 16'd0;
reg [15:0] reg_STATUS_DUMMY_5 = 16'd0;
reg [15:0] reg_STATUS_DUMMY_6 = 16'd0;
reg [15:0] reg_STATUS_DUMMY_7 = 16'd0;


reg [15:0] reg_DC_index_0 = 16'd0;
reg [15:0] reg_DC_index_1 = 16'd1;
reg [15:0] reg_DC_index_2 = 16'd2;
reg [15:0] reg_DC_index_3 = 16'd3;
reg [15:0] reg_DC_index_4 = 16'd4;
reg [15:0] reg_DC_index_5 = 16'd5;
reg [15:0] reg_DC_index_6 = 16'd6;
reg [15:0] reg_DC_index_7 = 16'd7;

reg [15:0] reg_DAB_T_MK_0 = 16'd0;
reg [15:0] reg_DAB_T_MK_1 = 16'd0;
reg [15:0] reg_DAB_T_MK_2 = 16'd0;
reg [15:0] reg_DAB_T_MK_3 = 16'd0;
reg [15:0] reg_DAB_T_MK_4 = 16'd0;
reg [15:0] reg_DAB_T_MK_5 = 16'd0;
reg [15:0] reg_DAB_T_MK_6 = 16'd0;
reg [15:0] reg_DAB_T_MK_7 = 16'd0;

reg [15:0] reg_Z;
reg [15:0] reg_REF_Right;

/*
reg [15:0] reg_DIN;
reg [15:0] reg_DIN_LATCH;
reg [15:0] reg_AD_PROT;
reg [15:0] reg_AD_PROT_LATCH;
reg [15:0] reg_FO;
reg [15:0] reg_FO_LATCH;
reg [15:0] reg_FB_PROT;
reg [15:0] reg_FB_PROT_LATCH;

reg [15:0] reg_INV_FO;
reg [15:0] reg_INV_FO_LATCH;
*/
//换成wire型
//reg [15:0] reg_SUM_DC = 16'd0;
//reg [15:0] reg_AVG_DC = 16'd0;
//reg [15:0] reg_SUM_IG = 16'd0;
//reg [15:0] reg_AVG_IG = 16'd0;
//reg [15:0] reg_SUM_IR = 16'd0;
//reg [15:0] reg_AVG_IR = 16'd0;

//SVG左桥臂PWM数据
wire [15:0] tx_PWM_L_0;// = 16'b0100000000000000;
wire [15:0] tx_PWM_L_1;// = 16'b0100000000000000;
wire [15:0] tx_PWM_L_2;// = 16'b0100000000000000;
wire [15:0] tx_PWM_L_3;// = 16'b0100000000000000;
wire [15:0] tx_PWM_L_4;// = 16'b0100000000000000;
wire [15:0] tx_PWM_L_5;// = 16'b0100000000000000;
wire [15:0] tx_PWM_L_6;// = 16'b0100000000000000;
wire [15:0] tx_PWM_L_7;// = 16'b0100000000000000;
//SVG右桥臂PWM数据
wire [15:0] tx_PWM_R_0;// = 16'b1000000000000000;
wire [15:0] tx_PWM_R_1;// = 16'b1000000000000000;
wire [15:0] tx_PWM_R_2;// = 16'b1000000000000000;
wire [15:0] tx_PWM_R_3;// = 16'b1000000000000000;
wire [15:0] tx_PWM_R_4;// = 16'b1000000000000000;
wire [15:0] tx_PWM_R_5;// = 16'b1000000000000000;
wire [15:0] tx_PWM_R_6;// = 16'b1000000000000000;
wire [15:0] tx_PWM_R_7;// = 16'b1000000000000000;
//DAB备用数据
reg [15:0] tx_PWM_DAB_0 = 16'b1100000000000000;
reg [15:0] tx_PWM_DAB_1 = 16'b1100000000000000;
reg [15:0] tx_PWM_DAB_2 = 16'b1100000000000000;
reg [15:0] tx_PWM_DAB_3 = 16'b1100000000000000;
reg [15:0] tx_PWM_DAB_4 = 16'b1100000000000000;
reg [15:0] tx_PWM_DAB_5 = 16'b1100000000000000;
reg [15:0] tx_PWM_DAB_6 = 16'b1100000000000000;
reg [15:0] tx_PWM_DAB_7 = 16'b1100000000000000;
//DAB脉冲中间时刻
wire [15:0] tx_PWM_DAB_T_0;// = 16'b1101000000000000;
wire [15:0] tx_PWM_DAB_T_1;// = 16'b1101000000000000;
wire [15:0] tx_PWM_DAB_T_2;// = 16'b1101000000000000;
wire [15:0] tx_PWM_DAB_T_3;// = 16'b1101000000000000;
wire [15:0] tx_PWM_DAB_T_4;// = 16'b1101000000000000;
wire [15:0] tx_PWM_DAB_T_5;// = 16'b1101000000000000;
wire [15:0] tx_PWM_DAB_T_6;// = 16'b1101000000000000;
wire [15:0] tx_PWM_DAB_T_7;// = 16'b1101000000000000;
//DAB左桥臂脉冲宽度
wire [15:0] tx_PWM_DAB_H_L_0;// = 16'b1110000000000000;
wire [15:0] tx_PWM_DAB_H_L_1;// = 16'b1110000000000000;
wire [15:0] tx_PWM_DAB_H_L_2;// = 16'b1110000000000000;
wire [15:0] tx_PWM_DAB_H_L_3;// = 16'b1110000000000000;
wire [15:0] tx_PWM_DAB_H_L_4;// = 16'b1110000000000000;
wire [15:0] tx_PWM_DAB_H_L_5;// = 16'b1110000000000000;
wire [15:0] tx_PWM_DAB_H_L_6;// = 16'b1110000000000000;
wire [15:0] tx_PWM_DAB_H_L_7;// = 16'b1110000000000000;
//DAB右桥臂脉冲宽度
wire [15:0] tx_PWM_DAB_H_R_0;// = 16'b1111000000000000;
wire [15:0] tx_PWM_DAB_H_R_1;// = 16'b1111000000000000;
wire [15:0] tx_PWM_DAB_H_R_2;// = 16'b1111000000000000;
wire [15:0] tx_PWM_DAB_H_R_3;// = 16'b1111000000000000;
wire [15:0] tx_PWM_DAB_H_R_4;// = 16'b1111000000000000;
wire [15:0] tx_PWM_DAB_H_R_5;// = 16'b1111000000000000;
wire [15:0] tx_PWM_DAB_H_R_6;// = 16'b1111000000000000;
wire [15:0] tx_PWM_DAB_H_R_7;// = 16'b1111000000000000;

wire [15:0] tx_PM_CTL;// = 16'h0000;

wire [7:0] TX_busy;
reg [15:0] TX0_buf;
reg [15:0] TX1_buf;
reg [15:0] TX2_buf;
reg [15:0] TX3_buf;
reg [15:0] TX4_buf;
reg [15:0] TX5_buf;
reg [15:0] TX6_buf;
reg [15:0] TX7_buf;
reg [7:0]  TxD_start = 8'd0;
wire [7:0] configured_modules = reg_PM[7:0];
wire [7:0] periodic_tx_mask = reg_TX_EN[7:0] & configured_modules;

// 周期帧按 TX_EN & PM 发送，原始帧按 TX_CTL & PM 发送。

async_transmitter TX0(.clk(CLK_100M), .enabled(configured_modules[0]), .datain(TX0_buf), .TXD(TX[0]), .WR(TxD_start[0]), .BUSY(TX_busy[0]));
async_transmitter TX1(.clk(CLK_100M), .enabled(configured_modules[1]), .datain(TX1_buf), .TXD(TX[1]), .WR(TxD_start[1]), .BUSY(TX_busy[1]));
async_transmitter TX2(.clk(CLK_100M), .enabled(configured_modules[2]), .datain(TX2_buf), .TXD(TX[2]), .WR(TxD_start[2]), .BUSY(TX_busy[2]));
async_transmitter TX3(.clk(CLK_100M), .enabled(configured_modules[3]), .datain(TX3_buf), .TXD(TX[3]), .WR(TxD_start[3]), .BUSY(TX_busy[3]));
async_transmitter TX4(.clk(CLK_100M), .enabled(configured_modules[4]), .datain(TX4_buf), .TXD(TX[4]), .WR(TxD_start[4]), .BUSY(TX_busy[4]));
async_transmitter TX5(.clk(CLK_100M), .enabled(configured_modules[5]), .datain(TX5_buf), .TXD(TX[5]), .WR(TxD_start[5]), .BUSY(TX_busy[5]));
async_transmitter TX6(.clk(CLK_100M), .enabled(configured_modules[6]), .datain(TX6_buf), .TXD(TX[6]), .WR(TxD_start[6]), .BUSY(TX_busy[6]));
async_transmitter TX7(.clk(CLK_100M), .enabled(configured_modules[7]), .datain(TX7_buf), .TXD(TX[7]), .WR(TxD_start[7]), .BUSY(TX_busy[7]));
//assign TX = {FPGA_LED,FPGA_LED,FPGA_LED,FPGA_LED};

reg [15:0] cnt = 16'h0000;
always @ (posedge CLK_100M)
begin
	if((cnt >= 16'd4999) || pos_sync)	cnt <= 16'd0;
	else											cnt <= cnt + 16'd1;
end
		
always @ (posedge CLK_100M)
begin
	if(TxD_start)	TxD_start <= 8'h00;
	else
	begin
		if(cnt == 16'd1)		
		begin
			TX0_buf <= reg_TX_0;
			TX1_buf <= reg_TX_1;
			TX2_buf <= reg_TX_2;
			TX3_buf <= reg_TX_3;
			TX4_buf <= reg_TX_4;
			TX5_buf <= reg_TX_5;
			TX6_buf <= reg_TX_6;
			TX7_buf <= reg_TX_7;
		end
		if(cnt == 16'd5)		TxD_start <= reg_TX_CTL[7:0] & configured_modules;
		
		if(cnt == 16'd601)		
		begin
			TX0_buf <= tx_PM_CTL;
			TX1_buf <= tx_PM_CTL;
			TX2_buf <= tx_PM_CTL;
			TX3_buf <= tx_PM_CTL;
			TX4_buf <= tx_PM_CTL;
			TX5_buf <= tx_PM_CTL;
			TX6_buf <= tx_PM_CTL;
			TX7_buf <= tx_PM_CTL;
		end
		if(cnt == 16'd605)		TxD_start <= periodic_tx_mask;
		
		if(cnt == 16'd1201)		
		begin
			TX0_buf <= tx_PWM_DAB_H_L_0;
			TX1_buf <= tx_PWM_DAB_H_L_1;
			TX2_buf <= tx_PWM_DAB_H_L_2;
			TX3_buf <= tx_PWM_DAB_H_L_3;
			TX4_buf <= tx_PWM_DAB_H_L_4;
			TX5_buf <= tx_PWM_DAB_H_L_5;
			TX6_buf <= tx_PWM_DAB_H_L_6;
			TX7_buf <= tx_PWM_DAB_H_L_7;
		end
		if(cnt == 16'd1205)		TxD_start <= periodic_tx_mask;
		
		if(cnt == 16'd1801)		
		begin
			TX0_buf <= tx_PWM_DAB_H_R_0;
			TX1_buf <= tx_PWM_DAB_H_R_1;
			TX2_buf <= tx_PWM_DAB_H_R_2;
			TX3_buf <= tx_PWM_DAB_H_R_3;
			TX4_buf <= tx_PWM_DAB_H_R_4;
			TX5_buf <= tx_PWM_DAB_H_R_5;
			TX6_buf <= tx_PWM_DAB_H_R_6;
			TX7_buf <= tx_PWM_DAB_H_R_7;
		end
		if(cnt == 16'd1805)		TxD_start <= periodic_tx_mask;
		
		if(cnt == 16'd2501)		
		begin
			TX0_buf <= tx_PWM_L_0;
			TX1_buf <= tx_PWM_L_1;
			TX2_buf <= tx_PWM_L_2;
			TX3_buf <= tx_PWM_L_3;
			TX4_buf <= tx_PWM_L_4;
			TX5_buf <= tx_PWM_L_5;
			TX6_buf <= tx_PWM_L_6;
			TX7_buf <= tx_PWM_L_7;
		end
		if(cnt == 16'd2505)		TxD_start <= periodic_tx_mask;
		
		if(cnt == 16'd3101)		
		begin
			TX0_buf <= tx_PWM_R_0;
			TX1_buf <= tx_PWM_R_1;
			TX2_buf <= tx_PWM_R_2;
			TX3_buf <= tx_PWM_R_3;
			TX4_buf <= tx_PWM_R_4;
			TX5_buf <= tx_PWM_R_5;
			TX6_buf <= tx_PWM_R_6;
			TX7_buf <= tx_PWM_R_7;
		end
		if(cnt == 16'd3105)		TxD_start <= periodic_tx_mask;
		
		if(cnt == 16'd3701)		//备用数据
		begin
			TX0_buf <= tx_PWM_DAB_0;
			TX1_buf <= tx_PWM_DAB_1;
			TX2_buf <= tx_PWM_DAB_2;
			TX3_buf <= tx_PWM_DAB_3;
			TX4_buf <= tx_PWM_DAB_4;
			TX5_buf <= tx_PWM_DAB_5;
			TX6_buf <= tx_PWM_DAB_6;
			TX7_buf <= tx_PWM_DAB_7;
		end
		if(cnt == 16'd3705)		TxD_start <= periodic_tx_mask;
		
		if(cnt == 16'd4301)		
		begin
			TX0_buf <= tx_PWM_DAB_T_0;
			TX1_buf <= tx_PWM_DAB_T_1;
			TX2_buf <= tx_PWM_DAB_T_2;
			TX3_buf <= tx_PWM_DAB_T_3;
			TX4_buf <= tx_PWM_DAB_T_4;
			TX5_buf <= tx_PWM_DAB_T_5;
			TX6_buf <= tx_PWM_DAB_T_6;
			TX7_buf <= tx_PWM_DAB_T_7;
		end
		if(cnt == 16'd4305)		TxD_start <= periodic_tx_mask;
	end
end

assign	tx_PM_CTL = {3'b000, reg_PM_CTL[12:0]};
	
assign	tx_PWM_DAB_H_L_0 = {4'b1110, DAB_DUTY_L_0[11:0]};
assign	tx_PWM_DAB_H_L_1 = {4'b1110, DAB_DUTY_L_1[11:0]};
assign	tx_PWM_DAB_H_L_2 = {4'b1110, DAB_DUTY_L_2[11:0]};
assign	tx_PWM_DAB_H_L_3 = {4'b1110, DAB_DUTY_L_3[11:0]};
assign	tx_PWM_DAB_H_L_4 = {4'b1110, DAB_DUTY_L_4[11:0]};
assign	tx_PWM_DAB_H_L_5 = {4'b1110, DAB_DUTY_L_5[11:0]};
assign	tx_PWM_DAB_H_L_6 = {4'b1110, DAB_DUTY_L_6[11:0]};
assign	tx_PWM_DAB_H_L_7 = {4'b1110, DAB_DUTY_L_7[11:0]};

assign	tx_PWM_DAB_H_R_0 = {4'b1111, DAB_DUTY_R_0[11:0]};
assign	tx_PWM_DAB_H_R_1 = {4'b1111, DAB_DUTY_R_1[11:0]};
assign	tx_PWM_DAB_H_R_2 = {4'b1111, DAB_DUTY_R_2[11:0]};
assign	tx_PWM_DAB_H_R_3 = {4'b1111, DAB_DUTY_R_3[11:0]};
assign	tx_PWM_DAB_H_R_4 = {4'b1111, DAB_DUTY_R_4[11:0]};
assign	tx_PWM_DAB_H_R_5 = {4'b1111, DAB_DUTY_R_5[11:0]};
assign	tx_PWM_DAB_H_R_6 = {4'b1111, DAB_DUTY_R_6[11:0]};
assign	tx_PWM_DAB_H_R_7 = {4'b1111, DAB_DUTY_R_7[11:0]};

assign	tx_PWM_DAB_T_0 = {4'b1101, DAB_T_0[11:0]};
assign	tx_PWM_DAB_T_1 = {4'b1101, DAB_T_1[11:0]};
assign	tx_PWM_DAB_T_2 = {4'b1101, DAB_T_2[11:0]};
assign	tx_PWM_DAB_T_3 = {4'b1101, DAB_T_3[11:0]};
assign	tx_PWM_DAB_T_4 = {4'b1101, DAB_T_4[11:0]};
assign	tx_PWM_DAB_T_5 = {4'b1101, DAB_T_5[11:0]};
assign	tx_PWM_DAB_T_6 = {4'b1101, DAB_T_6[11:0]};
assign	tx_PWM_DAB_T_7 = {4'b1101, DAB_T_7[11:0]};



// AFBR-2529Z 的 Data_OUT 直连 FPGA，空闲和无光均为高电平。
reg [15:0] reg_RX [0:7];
reg [15:0] reg_DC [0:7];
reg [15:0] reg_IR [0:7];
reg [15:0] reg_IG [0:7];
reg [15:0] reg_TEMP [0:7];
reg [15:0] reg_STATUS [0:7];
reg [15:0] reg_DRIVER [0:7];
reg [15:0] reg_SIN_L [0:7];
reg [15:0] reg_SIN_R [0:7];

wire clear_module_data = (reg_LED == 16'h0538);

genvar channel;
generate
	for(channel = 0; channel < 8; channel = channel + 1) begin : unit_rx
		wire [15:0] rx_word;
		wire rx_ready;
		reg rx_pending = 1'b0;

		async_receiver receiver(
			.clk(CLK_100M),
			.enabled(configured_modules[channel]),
			.RxD(RX[channel]),
			.RxD_data_ready(rx_ready),
			.RxD_data(rx_word)
		);

		always @ (posedge CLK_100M) begin
			if(!configured_modules[channel] || clear_module_data) begin
				reg_RX[channel] <= 16'd0;
				reg_DC[channel] <= 16'd0;
				reg_IR[channel] <= 16'd0;
				reg_IG[channel] <= 16'd0;
				reg_TEMP[channel] <= 16'd0;
				reg_STATUS[channel] <= 16'd0;
				reg_DRIVER[channel] <= configured_modules[channel] ? 16'h0FFF : 16'd0;
				reg_SIN_L[channel] <= 16'd0;
				reg_SIN_R[channel] <= 16'd0;
				rx_pending <= 1'b0;
			end
			else begin
				if(rx_ready) begin
					reg_RX[channel] <= rx_word;
					rx_pending <= 1'b1;
				end
				else if(!latch_BK) begin
					rx_pending <= 1'b0;
				end

				if(!latch_BK && rx_pending) begin
					case(reg_RX[channel][15:12])
						4'h0, 4'h1: reg_DC[channel] <= {{3{reg_RX[channel][12]}}, reg_RX[channel][12:0]};
						4'h2, 4'h3: reg_IR[channel] <= {{3{reg_RX[channel][12]}}, reg_RX[channel][12:0]};
						4'h4, 4'h5: reg_IG[channel] <= {{3{reg_RX[channel][12]}}, reg_RX[channel][12:0]};
						4'h6, 4'h7: reg_TEMP[channel] <= {{3{reg_RX[channel][12]}}, reg_RX[channel][12:0]};
						4'h8, 4'h9, 4'hA, 4'hB: reg_STATUS[channel] <= {2'b00, reg_RX[channel][13:0]};
						4'hC: reg_SIN_L[channel] <= {4'b0000, reg_RX[channel][11:0]};
						4'hD: reg_SIN_R[channel] <= {4'b0000, reg_RX[channel][11:0]};
						4'hE, 4'hF: reg_DRIVER[channel] <= {4'b0000, reg_RX[channel][11:0]};
					endcase
				end
			end
		end
	end
endgenerate

wire [15:0] wire_OR_STATUS;
wire [15:0] wire_DEBUG_L;
wire [15:0] wire_DEBUG_R;
wire [15:0] wire_DEBUG_NO_L;
wire [15:0] wire_DEBUG_NO_R;
wire [15:0] wire_NUM_PM;
wire [15:0] wire_SUM_DC;
wire [15:0] wire_SUM_IG;
wire [15:0] wire_SUM_IR;
wire [15:0] wire_AVG_DC;
wire [15:0] wire_AVG_IG;
wire [15:0] wire_AVG_IR;
wire [15:0] wire_SIN_L;		//回送的PWM脉冲
wire [15:0] wire_SIN_R;

reg WE_R1, WE_R2;//, WE_R3, WE_R4;
reg RD_R1, RD_R2;//, RD_R3, RD_R4;
reg CS_R1, CS_R2;//, CS_R3, CS_R4;
reg RD_clk = 1'b0;
reg WE_clk = 1'b0;

always @ (posedge CLK_100M)
begin	
//	WE_R4 <= WE_R3;
//	RD_R4 <= RD_R3;
//	CS_R4 <= CS_R3;
	
//	WE_R3 <= WE_R2;
//	RD_R3 <= RD_R2;
//	CS_R3 <= CS_R2;
	
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
//if(reg_TX_CTL) reg_TX_CTL <= 16'h0000;
//if(cnt == 16'd3006) 		reg_TX_CTL <= 16'h0000;
//else
begin
	if(~CS & RD_clk)	//DSP Read
		begin
			case(XA)
			8'h00: data <= reg_ID;
			8'h01: data <= reg_LED;
			8'h02: data <= reg_NUM;
			8'h03: data <= reg_PM;
			8'h04: data <= reg_REF;
			8'h05: data <= reg_DEBUG_NO;
			8'h06: data <= reg_DC_LOG;
			8'h07: data <= reg_TAD;
			8'h08: data <= reg08;
			8'h09: data <= reg09;
			8'h0A: data <= reg_DAB_DUTY_L;
			8'h0B: data <= reg_DAB_DUTY_R;
			8'h0C: data <= reg_DAB_T;
			8'h0D: data <= reg_PM_CTL;
			8'h0E: data <= reg_TX_EN;
			8'h0F: data <= reg_TX_CTL;	


			8'h10: data <= reg_TX_0;
			8'h11: data <= reg_TX_1;
			8'h12: data <= reg_TX_2;
			8'h13: data <= reg_TX_3;
			8'h14: data <= reg_TX_4;
			8'h15: data <= reg_TX_5;
			8'h16: data <= reg_TX_6;
			8'h17: data <= reg_TX_7;
			8'h18: data <= reg_RX[0];
			8'h19: data <= reg_RX[1];
			8'h1A: data <= reg_RX[2];
			8'h1B: data <= reg_RX[3];
			8'h1C: data <= reg_RX[4];
			8'h1D: data <= reg_RX[5];
			8'h1E: data <= reg_RX[6];
			8'h1F: data <= reg_RX[7];

//			8'h20: data <= reg_DOUT;
//			8'h21: data <= reg21;
//			8'h22: data <= reg22;
//			8'h23: data <= reg23;
//			8'h24: data <= reg24;
//			8'h25: data <= reg25;
//			8'h26: data <= reg26;
//			8'h27: data <= reg27;
//			8'h28: data <= reg_AD_PROT_EN;
//			8'h29: data <= reg29;
//			8'h2A: data <= reg2A;
//			8'h2B: data <= reg2B;
//			8'h2C: data <= reg2C;
//			8'h2D: data <= reg2D;
//			8'h2E: data <= reg2E;
//			8'h2F: data <= reg2F;

			8'h30, 8'h31, 8'h32, 8'h33, 8'h34,
			8'h35, 8'h36, 8'h37, 8'h38, 8'h39: data <= 16'd0;
			8'h3A: data <= reg_TEST3A;
			8'h3B: data <= reg_TEST3B;
			8'h3C: data <= wire_DEBUG_L;
			8'h3D: data <= wire_DEBUG_R;
//			8'h3E: data <= reg3E;
//			8'h3F: data <= reg_DSP_CTL;


//			8'h60: data <= reg_AD_0;
//			8'h61: data <= reg_AD_1;
//			8'h62: data <= reg_AD_2;
//			8'h63: data <= reg_AD_3;
//			8'h64: data <= reg_AD_4;
//			8'h65: data <= reg_AD_5;
//			8'h66: data <= reg_AD_6;
//			8'h67: data <= reg_AD_7;
//			8'h68: data <= reg_AD_8;
//			8'h69: data <= reg_AD_9;
//			8'h6A: data <= reg_AD_A;
//			8'h6B: data <= reg_AD_B;
//			8'h6C: data <= reg_AD_C;
//			8'h6D: data <= reg_AD_D;
//			8'h6E: data <= reg_AD_E;
//			8'h6F: data <= reg_AD_F;

//			8'h70: data <= reg_AD_LATCH_0;
//			8'h71: data <= reg_AD_LATCH_1;
//			8'h72: data <= reg_AD_LATCH_2;
//			8'h73: data <= reg_AD_LATCH_3;
//			8'h74: data <= reg_AD_LATCH_4;
//			8'h75: data <= reg_AD_LATCH_5;
//			8'h76: data <= reg_AD_LATCH_6;
//			8'h77: data <= reg_AD_LATCH_7;
//			8'h78: data <= reg_AD_LATCH_8;
//			8'h79: data <= reg_AD_LATCH_9;
//			8'h7A: data <= reg_AD_LATCH_A;
//			8'h7B: data <= reg_AD_LATCH_B;
//			8'h7C: data <= reg_AD_LATCH_C;
//			8'h7D: data <= reg_AD_LATCH_D;
//			8'h7E: data <= reg_AD_LATCH_E;
//			8'h7F: data <= reg_AD_LATCH_F;

			8'h80: data <= reg_DC[0];
			8'h81: data <= reg_DC[1];
			8'h82: data <= reg_DC[2];
			8'h83: data <= reg_DC[3];
			8'h84: data <= reg_DC[4];
			8'h85: data <= reg_DC[5];
			8'h86: data <= reg_DC[6];
			8'h87: data <= reg_DC[7];
			8'h88: data <= reg_DC_LATCH_0;
			8'h89: data <= reg_DC_LATCH_1;
			8'h8A: data <= reg_DC_LATCH_2;
			8'h8B: data <= reg_DC_LATCH_3;
			8'h8C: data <= reg_DC_LATCH_4;
			8'h8D: data <= reg_DC_LATCH_5;
			8'h8E: data <= reg_DC_LATCH_6;
			8'h8F: data <= reg_DC_LATCH_7;

			8'h90: data <= reg_IR[0];
			8'h91: data <= reg_IR[1];
			8'h92: data <= reg_IR[2];
			8'h93: data <= reg_IR[3];
			8'h94: data <= reg_IR[4];
			8'h95: data <= reg_IR[5];
			8'h96: data <= reg_IR[6];
			8'h97: data <= reg_IR[7];

			//IR无用，换成调试直流电压排序输出
			8'h98: data <= reg_DC_index_0;
			8'h99: data <= reg_DC_index_1;
			8'h9A: data <= reg_DC_index_2;
			8'h9B: data <= reg_DC_index_3;
			8'h9C: data <= reg_DC_index_4;
			8'h9D: data <= reg_DC_index_5;
			8'h9E: data <= reg_DC_index_6;
			8'h9F: data <= reg_DC_index_7;

			8'hA0: data <= reg_IG[0];
			8'hA1: data <= reg_IG[1];
			8'hA2: data <= reg_IG[2];
			8'hA3: data <= reg_IG[3];
			8'hA4: data <= reg_IG[4];
			8'hA5: data <= reg_IG[5];
			8'hA6: data <= reg_IG[6];
			8'hA7: data <= reg_IG[7];
			8'hA8: data <= reg_IG_LATCH_0;
			8'hA9: data <= reg_IG_LATCH_1;
			8'hAA: data <= reg_IG_LATCH_2;
			8'hAB: data <= reg_IG_LATCH_3;
			8'hAC: data <= reg_IG_LATCH_4;
			8'hAD: data <= reg_IG_LATCH_5;
			8'hAE: data <= reg_IG_LATCH_6;
			8'hAF: data <= reg_IG_LATCH_7;

			8'hB0: data <= reg_TEMP[0];
			8'hB1: data <= reg_TEMP[1];
			8'hB2: data <= reg_TEMP[2];
			8'hB3: data <= reg_TEMP[3];
			8'hB4: data <= reg_TEMP[4];
			8'hB5: data <= reg_TEMP[5];
			8'hB6: data <= reg_TEMP[6];
			8'hB7: data <= reg_TEMP[7];
			8'hB8: data <= reg_TEMP_LATCH_0;
			8'hB9: data <= reg_TEMP_LATCH_1;
			8'hBA: data <= reg_TEMP_LATCH_2;
			8'hBB: data <= reg_TEMP_LATCH_3;
			8'hBC: data <= reg_TEMP_LATCH_4;
			8'hBD: data <= reg_TEMP_LATCH_5;
			8'hBE: data <= reg_TEMP_LATCH_6;
			8'hBF: data <= reg_TEMP_LATCH_7;

			8'hC0: data <= reg_STATUS[0];
			8'hC1: data <= reg_STATUS[1];
			8'hC2: data <= reg_STATUS[2];
			8'hC3: data <= reg_STATUS[3];
			8'hC4: data <= reg_STATUS[4];
			8'hC5: data <= reg_STATUS[5];
			8'hC6: data <= reg_STATUS[6];
			8'hC7: data <= reg_STATUS[7];
			
			8'hC8: data <= reg_STATUS_DUMMY_0;
			8'hC9: data <= reg_STATUS_DUMMY_1;
			8'hCA: data <= reg_STATUS_DUMMY_2;
			8'hCB: data <= reg_STATUS_DUMMY_3;
			8'hCC: data <= reg_STATUS_DUMMY_4;
			8'hCD: data <= reg_STATUS_DUMMY_5;
			8'hCE: data <= reg_STATUS_DUMMY_6;
			8'hCF: data <= reg_STATUS_DUMMY_7;
		

			8'hD0: data <= reg_DRIVER[0];
			8'hD1: data <= reg_DRIVER[1];
			8'hD2: data <= reg_DRIVER[2];
			8'hD3: data <= reg_DRIVER[3];
			8'hD4: data <= reg_DRIVER[4];
			8'hD5: data <= reg_DRIVER[5];
			8'hD6: data <= reg_DRIVER[6];
			8'hD7: data <= reg_DRIVER[7];

			8'hD8: data <= reg_Z;
			8'hD9: data <= reg_REF_Right;
			8'hDA: data <= wire_NUM_PM;
			8'hDB: data <= reg_DRIVER[3];
			8'hDC: data <= reg_DRIVER[4];
			8'hDD: data <= reg_DRIVER[5];
			8'hDE: data <= reg_DRIVER[6];
			8'hDF: data <= reg_DRIVER[7];
			
			//DSP写入的DAB均压调整移相角
			8'hE8: data <= reg_DAB_T_MK_0;
			8'hE9: data <= reg_DAB_T_MK_1;
			8'hEA: data <= reg_DAB_T_MK_2;
			8'hEB: data <= reg_DAB_T_MK_3;
			8'hEC: data <= reg_DAB_T_MK_4;
			8'hED: data <= reg_DAB_T_MK_5;
			8'hEE: data <= reg_DAB_T_MK_6;
			8'hEF: data <= reg_DAB_T_MK_7;
			
			8'hF0: data <= wire_OR_STATUS;
			8'hF4: data <= wire_SIN_L;
			8'hF5: data <= wire_SIN_R;
			8'hF6: data <= wire_DEBUG_NO_L;
			8'hF7: data <= wire_DEBUG_NO_R;
//			8'hF8: data <= reg_INV_FO;
//			8'hF9: data <= reg_INV_FO_LATCH;
			8'hFA: data <= wire_SUM_DC;
			8'hFB: data <= wire_AVG_DC;
			8'hFC: data <= wire_SUM_IG;
			8'hFD: data <= wire_AVG_IG;
			8'hFE: data <= wire_SUM_IR;
			8'hFF: data <= wire_AVG_IR;

			default: data <= data;
			endcase
		end
	else if(~CS & WE_clk)	//DSP Write
		begin
			case(XA)	
			8'h00: reg_ID <= XD;
			8'h01: reg_LED <= XD;
			8'h02: reg_NUM <= XD;
			8'h03: reg_PM <= XD;
			8'h04: reg_REF <= XD;
			8'h05: reg_DEBUG_NO <= XD;
			8'h06: reg_DC_LOG <= XD;
			8'h07: reg_TAD <= XD;
			8'h08: reg08 <= XD;
			8'h09: reg09 <= XD;
			8'h0A: reg_DAB_DUTY_L <= XD;
			8'h0B: reg_DAB_DUTY_R <= XD;
			8'h0C: reg_DAB_T <= XD;
			8'h0D: reg_PM_CTL <= XD;
			8'h0E: reg_TX_EN <= XD;
			8'h0F: reg_TX_CTL <= XD;

			8'h10: reg_TX_0 <= XD;
			8'h11: reg_TX_1 <= XD;
			8'h12: reg_TX_2 <= XD;
			8'h13: reg_TX_3 <= XD;
			8'h14: reg_TX_4 <= XD;
			8'h15: reg_TX_5 <= XD;
			8'h16: reg_TX_6 <= XD;
			8'h17: reg_TX_7 <= XD;


			8'h3B: reg_TEST3B <= XD;
			
			//DSP写入的DAB均压调整移相角
			8'hE8: reg_DAB_T_MK_0 <= XD;
			8'hE9: reg_DAB_T_MK_1 <= XD;
			8'hEA: reg_DAB_T_MK_2 <= XD;
			8'hEB: reg_DAB_T_MK_3 <= XD;
			8'hEC: reg_DAB_T_MK_4 <= XD;
			8'hED: reg_DAB_T_MK_5 <= XD;
			8'hEE: reg_DAB_T_MK_6 <= XD;
			8'hEF: reg_DAB_T_MK_7 <= XD;
				
			default: data <= data;
			endcase
		end
	else
		begin
			data <= data;
		end
end

//assign XD = ( ~RD_R3 & ~CS_R3) ? data : 16'hzzzz;
//assign DIR = ( ~RD_R3 & ~CS_R3);
assign XD = ((~RD | ~RD_R2) & (~CS | ~CS_R2)) ? data : 16'hzzzz;	
assign DIR = (~RD | ~RD_R2) & (~CS | ~CS_R2);
//延迟2个clk，保证DSP在OE上升沿的时候，FPGA数据能保持

//计算电压电流的总和及平均值
wire [15:0] AVG_DC_out;
wire [15:0] AVG_IG_out;
wire [15:0] AVG_IR_out;

div_16X16  div_DC (
  .rfd          (           ),
  .clk          (CLK_100M   ),
  .dividend     (wire_SUM_DC),
  .quotient     (AVG_DC_out ),
  .divisor      (wire_NUM_PM),
  .fractional   (           )
);

div_16X16  div_IG (
  .rfd          (           ),
  .clk          (CLK_100M   ),
  .dividend     (wire_SUM_IG),
  .quotient     (AVG_IG_out ),
  .divisor      (wire_NUM_PM),
  .fractional   (           )
);

div_16X16  div_IR (
  .rfd          (           ),
  .clk          (CLK_100M   ),
  .dividend     (wire_SUM_IR),
  .quotient     (AVG_IR_out ),
  .divisor      (wire_NUM_PM),
  .fractional   (           )
);


assign wire_NUM_PM = reg_PM[0]+reg_PM[1]+reg_PM[2]+reg_PM[3]+reg_PM[4]+reg_PM[5]+reg_PM[6]+reg_PM[7];
assign wire_SUM_DC = reg_DC[0]+reg_DC[1]+reg_DC[2]+reg_DC[3]+reg_DC[4]+reg_DC[5]+reg_DC[6]+reg_DC[7];
assign wire_SUM_IG = reg_IG[0]+reg_IG[1]+reg_IG[2]+reg_IG[3]+reg_IG[4]+reg_IG[5]+reg_IG[6]+reg_IG[7];
assign wire_SUM_IR = reg_IR[0]+reg_IR[1]+reg_IR[2]+reg_IR[3]+reg_IR[4]+reg_IR[5]+reg_IR[6]+reg_IR[7];
assign wire_SIN_L = reg_SIN_L[0]+reg_SIN_L[1]+reg_SIN_L[2]+reg_SIN_L[3]+reg_SIN_L[4]+reg_SIN_L[5]+reg_SIN_L[6]+reg_SIN_L[7];
assign wire_SIN_R = reg_SIN_R[0]+reg_SIN_R[1]+reg_SIN_R[2]+reg_SIN_R[3]+reg_SIN_R[4]+reg_SIN_R[5]+reg_SIN_R[6]+reg_SIN_R[7];
//assign wire_SUM_DC = (reg_DC[0]&{16{reg_PM[0]}})+(reg_DC[1]&{16{reg_PM[1]}})+(reg_DC[2]&{16{reg_PM[2]}})+(reg_DC[3]&{16{reg_PM[3]}})+(reg_DC[4]&{16{reg_PM[4]}})+(reg_DC[5]&{16{reg_PM[5]}})+(reg_DC[6]&{16{reg_PM[6]}})+(reg_DC[7]&{16{reg_PM[7]}});
//assign wire_SUM_IG = (reg_IG[0]&{16{reg_PM[0]}})+(reg_IG[1]&{16{reg_PM[1]}})+(reg_IG[2]&{16{reg_PM[2]}})+(reg_IG[3]&{16{reg_PM[3]}})+(reg_IG[4]&{16{reg_PM[4]}})+(reg_IG[5]&{16{reg_PM[5]}})+(reg_IG[6]&{16{reg_PM[6]}})+(reg_IG[7]&{16{reg_PM[7]}});
//assign wire_SUM_IR = (reg_IR[0]&{16{reg_PM[0]}})+(reg_IR[1]&{16{reg_PM[1]}})+(reg_IR[2]&{16{reg_PM[2]}})+(reg_IR[3]&{16{reg_PM[3]}})+(reg_IR[4]&{16{reg_PM[4]}})+(reg_IR[5]&{16{reg_PM[5]}})+(reg_IR[6]&{16{reg_PM[6]}})+(reg_IR[7]&{16{reg_PM[7]}});
assign wire_AVG_DC = ((wire_NUM_PM==0)? 0:AVG_DC_out);
assign wire_AVG_IG = ((wire_NUM_PM==0)? 0:AVG_IG_out);
assign wire_AVG_IR = ((wire_NUM_PM==0)? 0:AVG_IR_out);

assign wire_OR_STATUS = reg_STATUS[0]|reg_STATUS[1]|reg_STATUS[2]|reg_STATUS[3]|reg_STATUS[4]|reg_STATUS[5]|reg_STATUS[6]|reg_STATUS[7];

/*
wire lock, unlock;
reg BK_R1, BK_R2, BK_R3;

always @ (posedge CLK_100M)
begin
	  BK_R1 <= BK;
	  BK_R2 <= BK_R1;
	  BK_R3 <= BK_R2;
end

assign lock = (~BK_R3)&(BK_R2)&(BK_R1); 
assign unlock = (BK_R3)&(~BK_R2)&(~BK_R1); 

always @ (posedge CLK_100M)
begin
	if(lock)
	begin
		 reg_DC_LATCH_0 <= reg_DC[0];
		 reg_DC_LATCH_1 <= reg_DC[1];
		 reg_DC_LATCH_2 <= reg_DC[2];
		 reg_DC_LATCH_3 <= reg_DC[3];
		 reg_DC_LATCH_4 <= reg_DC[4];
		 reg_DC_LATCH_5 <= reg_DC[5];
		 reg_DC_LATCH_6 <= reg_DC[6];
		 reg_DC_LATCH_7 <= reg_DC[7];
		 reg_IR_LATCH_0 <= reg_IR[0];
		 reg_IR_LATCH_1 <= reg_IR[1];
		 reg_IR_LATCH_2 <= reg_IR[2];
		 reg_IR_LATCH_3 <= reg_IR[3];
		 reg_IR_LATCH_4 <= reg_IR[4];
		 reg_IR_LATCH_5 <= reg_IR[5];
		 reg_IR_LATCH_6 <= reg_IR[6];
		 reg_IR_LATCH_7 <= reg_IR[7];
		 reg_IG_LATCH_0 <= reg_IG[0];
		 reg_IG_LATCH_1 <= reg_IG[1];
		 reg_IG_LATCH_2 <= reg_IG[2];
		 reg_IG_LATCH_3 <= reg_IG[3];
		 reg_IG_LATCH_4 <= reg_IG[4];
		 reg_IG_LATCH_5 <= reg_IG[5];
		 reg_IG_LATCH_6 <= reg_IG[6];
		 reg_IG_LATCH_7 <= reg_IG[7];
		 reg_TEMP_LATCH_0 <= reg_TEMP[0];
		 reg_TEMP_LATCH_1 <= reg_TEMP[1];
		 reg_TEMP_LATCH_2 <= reg_TEMP[2];
		 reg_TEMP_LATCH_3 <= reg_TEMP[3];
		 reg_TEMP_LATCH_4 <= reg_TEMP[4];
		 reg_TEMP_LATCH_5 <= reg_TEMP[5];
		 reg_TEMP_LATCH_6 <= reg_TEMP[6];
		 reg_TEMP_LATCH_7 <= reg_TEMP[7];
	end
	else if(unlock)
	begin
		 reg_DC_LATCH_0 <= 16'h0000;
		 reg_DC_LATCH_1 <= 16'h0000;
		 reg_DC_LATCH_2 <= 16'h0000;
		 reg_DC_LATCH_3 <= 16'h0000;
		 reg_DC_LATCH_4 <= 16'h0000;
		 reg_DC_LATCH_5 <= 16'h0000;
		 reg_DC_LATCH_6 <= 16'h0000;
		 reg_DC_LATCH_7 <= 16'h0000;
		 reg_IR_LATCH_0 <= 16'h0000;
		 reg_IR_LATCH_1 <= 16'h0000;
		 reg_IR_LATCH_2 <= 16'h0000;
		 reg_IR_LATCH_3 <= 16'h0000;
		 reg_IR_LATCH_4 <= 16'h0000;
		 reg_IR_LATCH_5 <= 16'h0000;
		 reg_IR_LATCH_6 <= 16'h0000;
		 reg_IR_LATCH_7 <= 16'h0000;
		 reg_IG_LATCH_0 <= 16'h0000;
		 reg_IG_LATCH_1 <= 16'h0000;
		 reg_IG_LATCH_2 <= 16'h0000;
		 reg_IG_LATCH_3 <= 16'h0000;
		 reg_IG_LATCH_4 <= 16'h0000;
		 reg_IG_LATCH_5 <= 16'h0000;
		 reg_IG_LATCH_6 <= 16'h0000;
		 reg_IG_LATCH_7 <= 16'h0000;
		 reg_TEMP_LATCH_0 <= 16'h0000;
		 reg_TEMP_LATCH_1 <= 16'h0000;
		 reg_TEMP_LATCH_2 <= 16'h0000;
		 reg_TEMP_LATCH_3 <= 16'h0000;
		 reg_TEMP_LATCH_4 <= 16'h0000;
		 reg_TEMP_LATCH_5 <= 16'h0000;
		 reg_TEMP_LATCH_6 <= 16'h0000;
		 reg_TEMP_LATCH_7 <= 16'h0000;
	end
	else
	begin
		 reg_DC_LATCH_0 <= reg_DC_LATCH_0;
		 reg_DC_LATCH_1 <= reg_DC_LATCH_1;
		 reg_DC_LATCH_2 <= reg_DC_LATCH_2;
		 reg_DC_LATCH_3 <= reg_DC_LATCH_3;
		 reg_DC_LATCH_4 <= reg_DC_LATCH_4;
		 reg_DC_LATCH_5 <= reg_DC_LATCH_5;
		 reg_DC_LATCH_6 <= reg_DC_LATCH_6;
		 reg_DC_LATCH_7 <= reg_DC_LATCH_7;
		 reg_IR_LATCH_0 <= reg_IR_LATCH_0;
		 reg_IR_LATCH_1 <= reg_IR_LATCH_1;
		 reg_IR_LATCH_2 <= reg_IR_LATCH_2;
		 reg_IR_LATCH_3 <= reg_IR_LATCH_3;
		 reg_IR_LATCH_4 <= reg_IR_LATCH_4;
		 reg_IR_LATCH_5 <= reg_IR_LATCH_5;
		 reg_IR_LATCH_6 <= reg_IR_LATCH_6;
		 reg_IR_LATCH_7 <= reg_IR_LATCH_7;
		 reg_IG_LATCH_0 <= reg_IG_LATCH_0;
		 reg_IG_LATCH_1 <= reg_IG_LATCH_1;
		 reg_IG_LATCH_2 <= reg_IG_LATCH_2;
		 reg_IG_LATCH_3 <= reg_IG_LATCH_3;
		 reg_IG_LATCH_4 <= reg_IG_LATCH_4;
		 reg_IG_LATCH_5 <= reg_IG_LATCH_5;
		 reg_IG_LATCH_6 <= reg_IG_LATCH_6;
		 reg_IG_LATCH_7 <= reg_IG_LATCH_7;
		 reg_TEMP_LATCH_0 <= reg_TEMP_LATCH_0;
		 reg_TEMP_LATCH_1 <= reg_TEMP_LATCH_1;
		 reg_TEMP_LATCH_2 <= reg_TEMP_LATCH_2;
		 reg_TEMP_LATCH_3 <= reg_TEMP_LATCH_3;
		 reg_TEMP_LATCH_4 <= reg_TEMP_LATCH_4;
		 reg_TEMP_LATCH_5 <= reg_TEMP_LATCH_5;
		 reg_TEMP_LATCH_6 <= reg_TEMP_LATCH_6;
		 reg_TEMP_LATCH_7 <= reg_TEMP_LATCH_7;
	end
end
*/

wire [31:0] mult_out;
mul_16X16 mul_16X16(
  .ce   (1'b1      ),
  .rst  (1'b0      ),
  .clk  (CLK_100M  ),
  .a    (reg_NUM   ),
  .b    (reg_TAD   ),
  .p    (mult_out  )
);


always @ (posedge CLK_100M)
begin
	reg_Z <= mult_out[16:1];
	reg_REF_Right <= reg_Z - reg_REF;
end


wire [15:0] DC_index_0;
wire [15:0] DC_index_1;
wire [15:0] DC_index_2;
wire [15:0] DC_index_3;
wire [15:0] DC_index_4;
wire [15:0] DC_index_5;
wire [15:0] DC_index_6;
wire [15:0] DC_index_7;

always @ (posedge CLK_100M)
begin
	reg_DC_index_0 <= DC_index_0;
	reg_DC_index_1 <= DC_index_1;
	reg_DC_index_2 <= DC_index_2;
	reg_DC_index_3 <= DC_index_3;
	reg_DC_index_4 <= DC_index_4;
	reg_DC_index_5 <= DC_index_5;
	reg_DC_index_6 <= DC_index_6;
	reg_DC_index_7 <= DC_index_7;
end

//Index的值从0~7,电压从高到低
list LIST(	.clk(CLK_100M), 
				.START(cnt==16'd600),	//同步后模块最先上送的就是直流电容电压，6us传输完成后进行排序
				.PM(reg_PM),
				.DC0(reg_DC[0]),
				.DC1(reg_DC[1]),
				.DC2(reg_DC[2]),
				.DC3(reg_DC[3]),
				.DC4(reg_DC[4]),
				.DC5(reg_DC[5]),
				.DC6(reg_DC[6]),
				.DC7(reg_DC[7]),

				.INDEX0(DC_index_0),
				.INDEX1(DC_index_1),
				.INDEX2(DC_index_2),
				.INDEX3(DC_index_3),
				.INDEX4(DC_index_4),
				.INDEX5(DC_index_5),
				.INDEX6(DC_index_6),
				.INDEX7(DC_index_7)

				);

wire current_orient;	//1: 电流流入SVG  0：电流流出SVG
wire dc_order;			//DSP比较两块PWM插件上的平均电压高低，0高1低
assign dc_order = reg_DC_LOG[0];
assign current_orient = reg_DC_LOG[8];

/*
orient IG_ORIENT(	.clk(CLK_100M), 
						.START(cnt==16'd1800),	//同步后模块第三个数据是模块电流，18us传输完成后进行方向判断
						.PM(reg_PM),
						.IG0(reg_IG[0]),
						.IG1(reg_IG[1]),
						.IG2(reg_IG[2]),
						.IG3(reg_IG[3]),
						.IG4(reg_IG[4]),
						.IG5(reg_IG[5]),
						.IG6(reg_IG[6]),
						.IG7(reg_IG[7]),

						.ORIENT(current_orient)
						);
*/			

pwm PWM(	.clk(CLK_100M), 
			.START(cnt==16'd2400), 		//同步后控制器在25us向模块传输SVG的PWM数据，因此在24us进行计算
			.N(reg_NUM),
			.ORI(current_orient),
			.ORDER(dc_order),
			.TAD(reg_TAD),
			//.PM(reg_PM),
			.NUM_PM(wire_NUM_PM),
			.DEBUG(reg_DEBUG_NO),
			
			.LREF(reg_REF),
			.RREF(reg_REF_Right),
	
			//.LTotalLayerNum(TotalLayerNum_L),
			//.RTotalLayerNum(TotalLayerNum_R),
			//.LLayerActive(LayerActive_L),
			//.RLayerActive(LayerActive_R),
			//.LIntercept(Intercept_L),
			//.RIntercept(Intercept_R),
			//.LDuty(Duty_L),
			//.RDuty(Duty_R),
			//.LOnNum(OnNum_L),
			//.ROnNum(OnNum_R),
			.LDebug(wire_DEBUG_L),
			.RDebug(wire_DEBUG_R),
			.LDebugNo(wire_DEBUG_NO_L),
			.RDebugNo(wire_DEBUG_NO_R),
			
			.PWML0(tx_PWM_L_0), 
			.PWML1(tx_PWM_L_1), 
			.PWML2(tx_PWM_L_2), 
			.PWML3(tx_PWM_L_3), 
			.PWML4(tx_PWM_L_4), 
			.PWML5(tx_PWM_L_5), 
			.PWML6(tx_PWM_L_6), 
			.PWML7(tx_PWM_L_7), 

			.PWMR0(tx_PWM_R_0), 
			.PWMR1(tx_PWM_R_1), 
			.PWMR2(tx_PWM_R_2), 
			.PWMR3(tx_PWM_R_3), 
			.PWMR4(tx_PWM_R_4), 
			.PWMR5(tx_PWM_R_5), 
			.PWMR6(tx_PWM_R_6), 
			.PWMR7(tx_PWM_R_7), 

			//.orPWM0(ORPWM0), 
			//.orPWM1(ORPWM1),  
			//.orPWM2(ORPWM2), 
			//.orPWM3(ORPWM3), 
			//.orPWM4(ORPWM4), 
			//.orPWM5(ORPWM5),  
			//.orPWM6(ORPWM6), 
			//.orPWM7(ORPWM7), 

			.INDEX0(DC_index_0),
			.INDEX1(DC_index_1),
			.INDEX2(DC_index_2),
			.INDEX3(DC_index_3),
			.INDEX4(DC_index_4),
			.INDEX5(DC_index_5),
			.INDEX6(DC_index_6),
			.INDEX7(DC_index_7)/*,
			
			.portLF(LF),
			.portLT(LT),
			.portRF(RF),
			.portRT(RT)*/
			);
			
assign FB = 1'b0;
assign LED = reg_LED[7:0];

//sfp暂时未启用
//reg [5:0] SFP_LED = 6'b000000;

//读写故障测试
always @ (posedge CLK_100M)
begin
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


wire [3:0]op_state;
assign op_state = 4'b0001;

dual_led led_inst(
    .clk(CLK_100M),
   .rst_n(1'b1),
	.op_state(op_state),
	.led(FPGA_LED)
);

//assign SFP_FAULT_LED = SFP_LED[5] | TEST3B_FAULT;
//assign LANE_UP_LED = SFP_LED[4];
//assign CHANNEL_UP_LED = SFP_LED[3] | TEST3B_FAULT;
//assign FRAME_ERR_LED = SFP_LED[2];
//assign SOFT_ERR_LED = SFP_LED[1] | TEST3B_FAULT;
//assign HARD_ERR_LED = SFP_LED[0];

endmodule
