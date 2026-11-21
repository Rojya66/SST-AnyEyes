`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    14:00:34 09/01/2017 
// Design Name: 
// Module Name:    main 
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
module main(
    input CLK,
    output [7:0] LED,
    output LED_SVG,
    output LED_DAB,

	 //AD
	 input [15:0] AD_DATA,
	 output CONVST,
	 output AD_RST,
	 output AD_RD,
	 output AD_CS,
	 
	 output OP_TX,
	 input  OP_RX,
	 
	 input  FPI_TEMP,
	 
	 output FP_OUT0,
	 output FP_OUT1,
	 
	 input PROT_HI,
	 input PROT_LO,
	 input PROT_Ig,
	 input PROT_Ir,
	 
	 output F_H1T1,
    output F_H1T2,
    output F_H2T1,
    output F_H2T2,
    output F_H3T1,
    output F_H3T2,
    output F_H4T1,
    output F_H4T2,
//    output F_RESET1,
//    output F_RESET2,
//    output F_RESET3,
//    output F_RESET4,
	 
//	 input [3:0] A_SO,
	 input [3:0] L_SO,
	 input [3:0] H_SO,
	 
	 input SYNC_IN,
	 
//    output [3:0] TEST,
	 input  SHUTDOWN,
    output OP_TX2,
	 output OP_FO
	 
	 
    );
	 
//parameter	CMD_SYNC = 16'h0111;
//parameter	CMD_OPEN = 16'h0555;
//parameter 	CMD_CLOZ = 16'h0AAA;

parameter DI_FILTER = 16'd10000;		//温度保护继电器开关滤波 100us
parameter PROT_FILTER = 16'd400;		//过压、过流保护光耦滤波 4us
parameter FO_FILTER = 16'd1000;		//驱动板故障滤波 10us
parameter OP_FILTER = 16'd200;		//光纤信号滤波 2us
parameter MIN_PULSE = 16'd150;		//滤窄脉冲1.5us
parameter SVG_DEADZONE = 16'd120;	//H1/H2死区0.5us，驱动板故障原因改为1.2
parameter DAB_DEADZONE = 16'd150;	//H3/H4死区1.5us

// TEMPORARY BENCH TEST BUILD ONLY:
// 1) DAB/SVG PWM are still controlled by DAB_EN/SVG_EN and retain deadtime.
// 2) External protection, driver feedback and SHUTDOWN do not gate final PWM.
// 3) SYNC_IN remains enabled so 501 and UNIT start each carrier coherently.
// Do not use this build on an energized power stage.
localparam TEST_BYPASS_EXTERNAL_INTERLOCKS = 1'b0;
localparam TEST_BYPASS_SVG_EXTERNAL_INTERLOCKS = 1'b0;
localparam TEST_USE_EXTERNAL_SYNC          = 1'b1;

wire CLK_100M;

//clock clock_100M(.CLK_IN(CLK), .CLK_OUT(CLK_100M)); 
clock clock_100M (
  .clkout0(CLK_100M),    // output
  .lock(),          // output
  .clkin1(CLK)       // input
);


//飞仕德驱动没有A_SO,用L_SO/H_SO组合过来，用于封脉冲保护 驱动板保护信号正常输出0 故障输出1
wire [3:0] A_SO;
assign A_SO[0] = L_SO[0]|H_SO[0];
assign A_SO[1] = L_SO[1]|H_SO[1];
assign A_SO[2] = L_SO[2]|H_SO[2];
assign A_SO[3] = L_SO[3]|H_SO[3];
//没有RESET
wire F_RESET1;
wire F_RESET2;
wire F_RESET3;
wire F_RESET4;

//同步信号
reg[2:0] sync_r = 3'b000;

always @ (posedge CLK_100M)
begin
	sync_r <= {sync_r[1:0], SYNC_IN};
end

wire 	pos_sync = TEST_USE_EXTERNAL_SYNC ? (~sync_r[2] & sync_r[1]) : 1'b0;

reg enable_sync_svg = 1'b0;
reg enable_sync_dab = 1'b0;


//处理接收的串行数据
reg[15:0]	reg_RX;
reg[15:0]	Rx_Fault_Cnt = 16'h0000;
reg[15:0]   reg_COMMAND = 16'h0200;	//PWM_CTL寄存器, 缺省上电时风扇不转
reg[15:0]   reg_MANUAL;		//界面人工下发指令
reg[15:0]	reg_PWM_SVG_L;
reg[15:0]	reg_PWM_SVG_R;
reg[15:0]	reg_PWM_DAB;	//备用数据
reg [15:0]	reg_PWM_DAB_T_RX = 16'd1250;	//串口收到的数据；测试默认相移中心
reg [15:0]	reg_PWM_DAB_T = 16'd1250;
reg [15:0]	reg_PWM_DAB_T_ori = 16'd1250;
reg [15:0]	reg_PWM_DAB_H_L = 16'd2500;
reg [15:0]	reg_PWM_DAB_H_R = 16'd2500;
reg [15:0]  PWM_DAB_T_L = 16'd1250;
reg [15:0]  PWM_DAB_T_R = 16'd3750;
reg [15:0]  PWM_DAB_T_L_ori = 16'd1250;
reg [15:0]  PWM_DAB_T_R_ori = 16'd3750;




wire [15:0] RxD_data;
wire RxD_data_ready;
async_receiver recv(.clk(CLK_100M), .RxD(OP_RX), .RxD_data_ready(RxD_data_ready), .RxD_data(RxD_data));	

always @ (posedge CLK_100M)
begin
	if(RxD_data_ready)	reg_RX <= RxD_data;
	
	case(reg_RX[15:12])
		4'b0000,
		4'b0001:	reg_COMMAND 		<= reg_RX & 16'h1FFF;
		4'b0010,
		4'b0011:	reg_MANUAL	 		<= reg_RX & 16'h1FFF;
		4'b0100,
		4'b0101,
		4'b0110,
		4'b0111:	reg_PWM_SVG_L 		<= reg_RX & 16'h3FFF;
		4'b1000,
		4'b1001,
		4'b1010,
		4'b1011:	reg_PWM_SVG_R 		<= reg_RX & 16'h3FFF;
		4'b1100: reg_PWM_DAB			<= reg_RX & 16'h0FFF;
		4'b1101: reg_PWM_DAB_T_RX	<= reg_RX & 16'h0FFF;
		4'b1110:	reg_PWM_DAB_H_L	<= reg_RX & 16'h0FFF;
		4'b1111:	reg_PWM_DAB_H_R	<= reg_RX & 16'h0FFF;
		default:	Rx_Fault_Cnt 		<= Rx_Fault_Cnt + 16'd1;
	endcase
end	

wire WORK_MODE;
wire DAB_SHUT;
wire SVG_SHUT;
wire FAN_OFF;
wire HW_RST;
wire DAB_RST;
wire [1:0] DAB_DIV;
wire DAB_EN;
wire SVG_RST;
wire [1:0] SVG_DIV;
wire SVG_EN;

reg FPL;
reg FPR;
reg [15:0] PWM_SVG_T_L;
reg [15:0] PWM_SVG_T_R;

reg DAB_FPL;
reg DAB_FPR;
reg  [15:0] DAB_T1_L;
reg  [15:0] DAB_T2_L;
reg  [15:0] DAB_T1_R;
reg  [15:0] DAB_T2_R;

assign WORK_MODE 	= reg_COMMAND[12];
assign DAB_SHUT   = reg_COMMAND[11];
assign SVG_SHUT   = reg_COMMAND[10];
assign FAN_OFF    = reg_COMMAND[9];
assign HW_RST 		= reg_COMMAND[8];
assign DAB_RST 	= reg_COMMAND[7];
assign DAB_DIV		= reg_COMMAND[6:5];
assign DAB_EN	 	= reg_COMMAND[4];
assign SVG_RST 	= reg_COMMAND[3];
assign SVG_DIV		= reg_COMMAND[2:1];
assign SVG_EN 		= reg_COMMAND[0];


//生成PWM脉冲
reg SVG_PL;
reg SVG_PR;
reg DAB_PL;
reg DAB_PR;
reg [15:0] svg_pwm_cnt = 16'd0;
reg [15:0] dab_pwm_cnt = 16'd0;
reg [15:0] SVG_TAD = 16'd5000;
reg [15:0] DAB_TAD = 16'd5000;

always @ (posedge CLK_100M)
begin
	if(pos_sync || !TEST_USE_EXTERNAL_SYNC)
	begin
		case(SVG_DIV)
			2'b00:	SVG_TAD <= 16'd5000;
			2'b01:	SVG_TAD <= 16'd2500;
			2'b10:	SVG_TAD <= 16'd1250;
			2'b11:	SVG_TAD <= 16'd16384;
		endcase
		case(DAB_DIV)
			2'b00:	DAB_TAD <= 16'd5000;
			2'b01:	DAB_TAD <= 16'd2500;
			2'b10:	DAB_TAD <= 16'd1250;
			2'b11:	DAB_TAD <= 16'd16384;
		endcase
	end
end

always @ (posedge CLK_100M)
begin
	if((svg_pwm_cnt >= SVG_TAD) || pos_sync)		
	begin
		enable_sync_svg <= SVG_EN;
		svg_pwm_cnt <= 16'd0;
		FPL <= reg_PWM_SVG_L[13];
		FPR <= reg_PWM_SVG_R[13];
		PWM_SVG_T_L <= reg_PWM_SVG_L & 16'h1FFF;
		PWM_SVG_T_R <= reg_PWM_SVG_R & 16'h1FFF;
	end
	else
	begin
		svg_pwm_cnt <= svg_pwm_cnt + 16'd1;
		if(svg_pwm_cnt == 16'd0)	
		begin
			SVG_PL <= FPL;
			SVG_PR <= FPR;
		end
		if(svg_pwm_cnt == PWM_SVG_T_L)
		begin
			SVG_PL <= ~FPL;
		end
		if(svg_pwm_cnt == PWM_SVG_T_R)
		begin
			SVG_PR <= ~FPR;
		end
		if((svg_pwm_cnt == (SVG_TAD-PWM_SVG_T_L)) && WORK_MODE)
		begin
			SVG_PL <= FPL;
		end
		if((svg_pwm_cnt == (SVG_TAD-PWM_SVG_T_R)) && WORK_MODE)
		begin
			SVG_PR <= FPR;
		end
	end
end

reg [15:0] SIN_L = 16'd0;	//回送SVG左桥臂脉冲
reg [15:0] SIN_R = 16'd0;	//回送SVG右桥臂脉冲

always @ (posedge CLK_100M)
begin
	if(FPL)
	begin
		if(PWM_SVG_T_L>SVG_TAD)		SIN_L<=SVG_TAD;
		else								SIN_L<=PWM_SVG_T_L;
	end
	else
	begin
		if(PWM_SVG_T_L>SVG_TAD)		SIN_L<=0;
		else								SIN_L<=(SVG_TAD-PWM_SVG_T_L);
	end
	
	if(FPR)
	begin
		if(PWM_SVG_T_R>SVG_TAD)		SIN_R<=SVG_TAD;
		else								SIN_R<=PWM_SVG_T_R;
	end
	else
	begin
		if(PWM_SVG_T_R>SVG_TAD)		SIN_R<=0;
		else								SIN_R<=(SVG_TAD-PWM_SVG_T_R);
	end
end

reg [15:0] reg_DAB_ChangeOfPhaseShift;
reg postive = 1'b1;		//1:正数  0：负数

always @ (posedge CLK_100M)
begin
	if((dab_pwm_cnt >= DAB_TAD) || pos_sync)		
	begin
		enable_sync_dab <= DAB_EN;
		dab_pwm_cnt <= 16'd0;
				
		//左桥臂
		if(PWM_DAB_T_L == (reg_PWM_DAB_H_L>>1))	
		//PhaseShift(k)=0
		begin
			if(PWM_DAB_T_L_ori == (reg_PWM_DAB_H_L>>1))
			//PhaseShift(k-1)=0
			begin
				DAB_FPL 	<= 1'b1;
				DAB_T1_L	<= reg_PWM_DAB_H_L;
				DAB_T2_L	<= 16'h7FFF;
			end
			else if(PWM_DAB_T_L_ori > (reg_PWM_DAB_H_L>>1))
			//PhaseShift(k-1)>0
			begin
				DAB_FPL 	<= 1'b0;
				DAB_T1_L	<= PWM_DAB_T_L_ori - (reg_PWM_DAB_H_L>>1);
				if(postive)		DAB_T2_L	<= reg_PWM_DAB_H_L - (reg_DAB_ChangeOfPhaseShift>>1);
				else				DAB_T2_L	<= reg_PWM_DAB_H_L + (reg_DAB_ChangeOfPhaseShift>>1);
			end
			else
			//PhaseShift(k-1)<0
			begin
				DAB_FPL 	<= 1'b1;
				if(postive)		DAB_T1_L	<= reg_PWM_DAB_H_L - (reg_DAB_ChangeOfPhaseShift>>1);
				else				DAB_T1_L	<= reg_PWM_DAB_H_L + (reg_DAB_ChangeOfPhaseShift>>1);
				DAB_T2_L	<= 16'h7FFF;
			end
		end
		else if(PWM_DAB_T_L > (reg_PWM_DAB_H_L>>1))
		//PhaseShift(k)>0
		begin
			if(PWM_DAB_T_L_ori == (reg_PWM_DAB_H_L>>1))
			//PhaseShift(k-1)=0
			begin
				DAB_FPL 	<= 1'b1;
				if(postive)		DAB_T1_L	<= PWM_DAB_T_L + (reg_PWM_DAB_H_L>>1) - (reg_DAB_ChangeOfPhaseShift>>1);
				else				DAB_T1_L	<= PWM_DAB_T_L + (reg_PWM_DAB_H_L>>1) + (reg_DAB_ChangeOfPhaseShift>>1);
				DAB_T2_L	<= 16'h7FFF;
			end
			else if(PWM_DAB_T_L_ori > (reg_PWM_DAB_H_L>>1))
			//PhaseShift(k-1)>0
			begin
				DAB_FPL 	<= 1'b0;
				DAB_T1_L	<= PWM_DAB_T_L_ori - (reg_PWM_DAB_H_L>>1);
				if(postive)		DAB_T2_L	<= PWM_DAB_T_L + (reg_PWM_DAB_H_L>>1) - (reg_DAB_ChangeOfPhaseShift>>1);
				else				DAB_T2_L	<= PWM_DAB_T_L + (reg_PWM_DAB_H_L>>1) + (reg_DAB_ChangeOfPhaseShift>>1);
			end
			else
			//PhaseShift(k-1)<0, 不存在此情况
			begin
			end
		end
		else
		//PhaseShift(k)<0
		begin
			//if(PWM_DAB_T_L_ori == (reg_PWM_DAB_H_L>>1))
			//PhaseShift(k-1)=0
			begin
				DAB_FPL 	<= 1'b1;
				if(postive)		DAB_T1_L	<= PWM_DAB_T_L + (reg_PWM_DAB_H_L>>1) - (reg_DAB_ChangeOfPhaseShift>>1);
				else				DAB_T1_L	<= PWM_DAB_T_L + (reg_PWM_DAB_H_L>>1) + (reg_DAB_ChangeOfPhaseShift>>1);
				DAB_T2_L	<= DAB_TAD - (reg_PWM_DAB_H_L>>1) + PWM_DAB_T_L;
			end
			//else if(PWM_DAB_T_L_ori > (reg_PWM_DAB_H_L>>1))
			//PhaseShift(k-1)>0, 不存在此情况
			//begin
			//end
			//else
			//PhaseShift(k-1)<0
			//begin
			//	DAB_FPL 	<= 1'b1;
			//	DAB_T1_L	<= PWM_DAB_T_L + (reg_PWM_DAB_H_L>>1) - (reg_DAB_ChangeOfPhaseShift>>1);
			//	DAB_T2_L	<= DAB_TAD - (reg_PWM_DAB_H_L>>1) + PWM_DAB_T_L;
			//end
		end
		
		//右桥臂
		if((PWM_DAB_T_R + (reg_PWM_DAB_H_R>>1)) == DAB_TAD)
		//PhaseShift(k)=0
		begin
			if((PWM_DAB_T_R_ori + (reg_PWM_DAB_H_R>>1)) == DAB_TAD)
			//PhaseShift(k-1)=0
			begin
				DAB_FPR 	<= 1'b0;
				DAB_T1_R	<= DAB_TAD - reg_PWM_DAB_H_R;
				DAB_T2_R	<= 16'h7FFF;
			end
			else if((PWM_DAB_T_R_ori + (reg_PWM_DAB_H_R>>1)) > DAB_TAD)
			//PhaseShift(k-1)>0
			begin
				DAB_FPR 	<= 1'b1;
				DAB_T1_R	<= (PWM_DAB_T_R_ori + (reg_PWM_DAB_H_R>>1) - DAB_TAD);
				if(postive)		DAB_T2_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) - (reg_DAB_ChangeOfPhaseShift>>1));
				else				DAB_T2_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) + (reg_DAB_ChangeOfPhaseShift>>1));
			end
			else
			//PhaseShift(k-1)<0
			begin
				DAB_FPR 	<= 1'b0;
				if(postive)		DAB_T1_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) - (reg_DAB_ChangeOfPhaseShift>>1));
				else				DAB_T1_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) + (reg_DAB_ChangeOfPhaseShift>>1));
				DAB_T2_R	<= 16'h7FFF;
			end
		end
		else if((PWM_DAB_T_R + (reg_PWM_DAB_H_R>>1)) > DAB_TAD)
		//PhaseShift(k)>0
		begin
			if((PWM_DAB_T_R_ori + (reg_PWM_DAB_H_R>>1)) == DAB_TAD)
			//PhaseShift(k-1)=0
			begin
				DAB_FPR 	<= 1'b0;
				if(postive)		DAB_T1_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) - (reg_DAB_ChangeOfPhaseShift>>1));
				else				DAB_T1_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) + (reg_DAB_ChangeOfPhaseShift>>1));
				DAB_T2_R	<= 16'h7FFF;
			end
			else if((PWM_DAB_T_R_ori + (reg_PWM_DAB_H_R>>1)) > DAB_TAD)
			//PhaseShift(k-1)>0
			begin
				DAB_FPR 	<= 1'b1;
				DAB_T1_R	<= (PWM_DAB_T_R_ori + (reg_PWM_DAB_H_R>>1) - DAB_TAD);
				if(postive)		DAB_T2_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) - (reg_DAB_ChangeOfPhaseShift>>1));
				else				DAB_T2_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) + (reg_DAB_ChangeOfPhaseShift>>1));
			end
			else
			//PhaseShift(k-1)<0, 不存在此情况
			begin
			end
		end
		else
		//PhaseShift(k)<0
		begin
				DAB_FPR 	<= 1'b0;
				if(postive)		DAB_T1_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) - (reg_DAB_ChangeOfPhaseShift>>1));
				else				DAB_T1_R	<= (PWM_DAB_T_R - (reg_PWM_DAB_H_R>>1) + (reg_DAB_ChangeOfPhaseShift>>1));
				DAB_T2_R	<= (PWM_DAB_T_R + (reg_PWM_DAB_H_R>>1));
		end
		
		reg_PWM_DAB_T_ori <= reg_PWM_DAB_T;
		PWM_DAB_T_L_ori <= reg_PWM_DAB_T;
		PWM_DAB_T_R_ori <= reg_PWM_DAB_T + (DAB_TAD>>1);
	end
	
	else
	begin
		dab_pwm_cnt <= dab_pwm_cnt + 16'd1;
		if(dab_pwm_cnt == 16'd0)	
		begin
			DAB_PL <= DAB_FPL;					
			DAB_PR <= DAB_FPR;					
		end
		if(dab_pwm_cnt == DAB_T1_L)	
		begin
			DAB_PL <= ~DAB_FPL;
		end
		if(dab_pwm_cnt == DAB_T1_R)	
		begin
			DAB_PR <= ~DAB_FPR;
		end
		if(dab_pwm_cnt == DAB_T2_L)	
		begin
			DAB_PL <= DAB_FPL;
		end
		if(dab_pwm_cnt == DAB_T2_R)	
		begin
			DAB_PR <= DAB_FPR;
		end
		if(dab_pwm_cnt == (DAB_TAD-4))		//收到控制器发送的PWM_DAB_T(reg_PWM_DAB_T大概在cnt=4900左右更新)，处理跨越04E2的问题
		begin
			reg_PWM_DAB_T <= reg_PWM_DAB_T_RX;
		end
		if(dab_pwm_cnt == (DAB_TAD-3))
		begin
			if(((reg_PWM_DAB_T_ori > (DAB_TAD>>2)) && (reg_PWM_DAB_T > (DAB_TAD>>2))) || ((reg_PWM_DAB_T_ori < (DAB_TAD>>2)) && (reg_PWM_DAB_T < (DAB_TAD>>2))) || (reg_PWM_DAB_T_ori == (DAB_TAD>>2)))	
			//PhaseShift(k)和PhaseShift(k-1)在同一侧
			begin
				if((reg_PWM_DAB_T > reg_PWM_DAB_T_ori) || (reg_PWM_DAB_T == reg_PWM_DAB_T_ori))
				begin
					reg_DAB_ChangeOfPhaseShift <= reg_PWM_DAB_T - reg_PWM_DAB_T_ori;
					postive <= 1'b1;
				end
				else
				begin
					reg_DAB_ChangeOfPhaseShift <= reg_PWM_DAB_T_ori - reg_PWM_DAB_T;
					postive <= 1'b0;
				end
			end
			else	
			//将接收到的reg_PWM_DAB_T数据改为04E2
			begin
				reg_PWM_DAB_T <= (DAB_TAD>>2);
				if(((DAB_TAD>>2) > reg_PWM_DAB_T_ori) || ((DAB_TAD>>2) == reg_PWM_DAB_T_ori))
				begin
					reg_DAB_ChangeOfPhaseShift <= (DAB_TAD>>2) - reg_PWM_DAB_T_ori;
					postive <= 1'b1;
				end
				else
				begin
					reg_DAB_ChangeOfPhaseShift <= reg_PWM_DAB_T_ori - (DAB_TAD>>2);
					postive <= 1'b0;
				end
			end
		end
		if(dab_pwm_cnt == (DAB_TAD-2))
		begin
			PWM_DAB_T_L <= reg_PWM_DAB_T;
			PWM_DAB_T_R <= reg_PWM_DAB_T + (DAB_TAD>>1);
		end
	end
end

//生成上下管脉冲及死区
wire SVG_PLU;
wire SVG_PRU;
wire DAB_PLU;
wire DAB_PRU;
wire SVG_PLD;
wire SVG_PRD;
wire DAB_PLD;
wire DAB_PRD;

deadzone #(SVG_DEADZONE) H1
(
    .CLK(CLK_100M),
    .RESET(SVG_RST),
    .INA(SVG_PL),
    .INB(~SVG_PL),
	 .ENABLE(SVG_EN&enable_sync_svg),
	 
    .OUTA(SVG_PLU),
    .OUTB(SVG_PLD)
    );
deadzone #(SVG_DEADZONE) H2
(
    .CLK(CLK_100M),
    .RESET(SVG_RST),
    .INA(SVG_PR),
    .INB(~SVG_PR),
	 .ENABLE(SVG_EN&enable_sync_svg),
	 
    .OUTA(SVG_PRU),
    .OUTB(SVG_PRD)
    );
deadzone #(DAB_DEADZONE) H3
(
    .CLK(CLK_100M),
    .RESET(DAB_RST),
    .INA(DAB_PL),
    .INB(~DAB_PL),
	 .ENABLE(DAB_EN&enable_sync_dab),
	 
    .OUTA(DAB_PLU),
    .OUTB(DAB_PLD)
    );
deadzone #(DAB_DEADZONE) H4
(
    .CLK(CLK_100M),
    .RESET(DAB_RST),
    .INA(DAB_PR),
    .INB(~DAB_PR),
	 .ENABLE(DAB_EN&enable_sync_dab),
	 
    .OUTA(DAB_PRU),
    .OUTB(DAB_PRD)
    );

//消除窄脉冲
wire PWM_H1T1;
wire PWM_H1T2;
wire PWM_H2T1;
wire PWM_H2T2;
wire PWM_H3T1;
wire PWM_H3T2;
wire PWM_H4T1;
wire PWM_H4T2;

//模块倒接，SVG左右桥臂互换（清华原注释）
//20261008 新模块设计SVG未倒接，无需互换
antishake #(MIN_PULSE) H2T1
(
	 .CLK(CLK_100M),
	 .INPUT(SVG_PRU),
	 .OUTPUT(PWM_H2T1)
);
antishake #(MIN_PULSE) H2T2
(
	 .CLK(CLK_100M),
	 .INPUT(SVG_PRD),
	 .OUTPUT(PWM_H2T2)
);
antishake #(MIN_PULSE) H1T1
(
	 .CLK(CLK_100M),
	 .INPUT(SVG_PLU),
	 .OUTPUT(PWM_H1T1)
);
antishake #(MIN_PULSE) H1T2
(
	 .CLK(CLK_100M),
	 .INPUT(SVG_PLD),
	 .OUTPUT(PWM_H1T2)
);

//DAB不受影响
antishake #(MIN_PULSE) H3T1
(
	 .CLK(CLK_100M),
	 .INPUT(DAB_PLU),
	 .OUTPUT(PWM_H3T1)
);
antishake #(MIN_PULSE) H3T2
(
	 .CLK(CLK_100M),
	 .INPUT(DAB_PLD),
	 .OUTPUT(PWM_H3T2)
);
antishake #(MIN_PULSE) H4T1
(
	 .CLK(CLK_100M),
	 .INPUT(DAB_PRU),
	 .OUTPUT(PWM_H4T1)
);
antishake #(MIN_PULSE) H4T2
(
	 .CLK(CLK_100M),
	 .INPUT(DAB_PRD),
	 .OUTPUT(PWM_H4T2)
);
//保护输出
wire fault_svg;
wire fault_dab;
wire shutdown_svg;
wire shutdown_dab;
// CHB/SVG bench-test path: bypass only the external/latching final gate.
// SVG_EN, synchronized enable, deadtime and narrow-pulse filtering above remain active.
assign F_H1T1 = PWM_H1T1 & (TEST_BYPASS_SVG_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_svg & ~shutdown_svg));
assign F_H1T2 = PWM_H1T2 & (TEST_BYPASS_SVG_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_svg & ~shutdown_svg));
assign F_H2T1 = PWM_H2T1 & (TEST_BYPASS_SVG_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_svg & ~shutdown_svg));
assign F_H2T2 = PWM_H2T2 & (TEST_BYPASS_SVG_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_svg & ~shutdown_svg));
// Test build: retain PWM generation/deadtime/min-pulse filtering, but bypass
// every external interlock at the final DAB output gate. DAB_EN remains the
// only run command through the deadzone ENABLE inputs above.
assign F_H3T1 = PWM_H3T1 & (TEST_BYPASS_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_dab & ~shutdown_dab));
assign F_H3T2 = PWM_H3T2 & (TEST_BYPASS_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_dab & ~shutdown_dab));
assign F_H4T1 = PWM_H4T1 & (TEST_BYPASS_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_dab & ~shutdown_dab));
assign F_H4T2 = PWM_H4T2 & (TEST_BYPASS_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_dab & ~shutdown_dab));

assign F_RESET1 = SVG_RST;
assign F_RESET2 = SVG_RST;
assign F_RESET3 = DAB_RST;
assign F_RESET4 = DAB_RST;

assign OP_FO = TEST_BYPASS_EXTERNAL_INTERLOCKS ? 1'b0 : (fault_svg | fault_dab);


reg  [7:0]	reg_fault = 8'h00;
wire [7:0]  wire_fault;
//0:OV;1:OC2;2:OC1;3:OT;4:SVG_H;5:DAB_H;6:UV;7:SHUTDOWN
reg  [5:0] reg_drv_svg = 6'b111111;	
reg  [5:0] reg_drv_dab = 6'b111111;		//反相，与驱动板厂家文档一致
wire [11:0] wire_drv_so;
reg block_dc = 1'b0;	//bit8
reg block_ig = 1'b0;	//bit9
reg block_ir = 1'b0;	//bit10
reg block_ot = 1'b0;	//bit11
reg block_svg = 1'b0;//bit12
reg block_dab = 1'b0;//bit13

//SHUTDOWN信号
antishake #(OP_FILTER) OP_SHUT
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
//	 .INPUT(~SHUTDOWN),
    .INPUT(SHUTDOWN),
//20260919，此型号光纤灭时输出高电平，因此输入改为不取反，但是必须有光输入才不报故障
//TEST_BYPASS_SVG_EXTERNAL_INTERLOCKS/TEST_BYPASS_EXTERNAL_INTERLOCKS=1,无视外部SHUTDOWN光纤
	 .OUTPUT(wire_fault[7])
);


assign wire_fault[4] = ~(wire_drv_so[0]&wire_drv_so[3]);
assign wire_fault[5] = ~(wire_drv_so[6]&wire_drv_so[9]);
always @ (posedge CLK_100M)
begin
	reg_fault <= wire_fault;

	if(~block_dc)	block_dc <= reg_fault[0];		//can't auto recovery fault
	else	block_dc <= (block_dc & ~HW_RST) | reg_fault[0];
	if(~block_ig)	block_ig <= reg_fault[1];		//can't auto recovery fault
	else	block_ig <= (block_ig & ~HW_RST) | reg_fault[1];
	if(~block_ir)	block_ir <= reg_fault[2];		//can't auto recovery fault
	else	block_ir <= (block_ir & ~HW_RST) | reg_fault[2];
	if(~block_ot)	block_ot <= reg_fault[3];		//can't auto recovery fault
	else	block_ot <= (block_ot & ~HW_RST) | reg_fault[3];
	if(~block_svg)	block_svg <= reg_fault[4];		//can't auto recovery fault
	else	block_svg <= (block_svg & ~HW_RST) | reg_fault[4];
	if(~block_dab)	block_dab <= reg_fault[5];		//can't auto recovery fault
	else	block_dab <= (block_dab & ~HW_RST) | reg_fault[5];
	
	if(~block_svg)	reg_drv_svg <= wire_drv_so[5:0];			//latch svg driver
	else reg_drv_svg <= reg_drv_svg;
	if(~block_dab)	reg_drv_dab <= wire_drv_so[11:6];		//latch dab driver
	else reg_drv_dab <= reg_drv_dab;
	
end

assign fault_svg = block_svg|block_ot|block_ir|block_ig|block_dc|reg_fault[0]|reg_fault[1]|reg_fault[2]|reg_fault[3]|reg_fault[4];
assign fault_dab = block_dab|block_ot|block_ir|block_ig|block_dc|reg_fault[0]|reg_fault[1]|reg_fault[2]|reg_fault[3]|reg_fault[5];
assign shutdown_svg = reg_fault[7];	// & SVG_SHUT;	//SVG_SHUT位为0时，SHUTDOWN光纤对级联H桥无效(2019.03.25, 此功能取消，SHUTDOWN光纤熄灭时，立刻闭锁SVG和DAB脉冲)
assign shutdown_dab = reg_fault[7];	// & DAB_SHUT;	//DAB_SHUT位为0时，SHUTDOWN光纤对DAB H桥无效(2019.03.25, 此功能取消，SHUTDOWN光纤熄灭时，立刻闭锁SVG和DAB脉冲)

//温度保护继电器开关滤波
antishake #(DI_FILTER) DI_TEMP
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(FPI_TEMP),
	 .OUTPUT(wire_fault[3])
);

//过压、过流保护光耦滤波
antishake #(PROT_FILTER) PROT_UV_DC
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~PROT_LO),
	 .OUTPUT(wire_fault[6])
);
antishake #(PROT_FILTER) PROT_OV_DC
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(PROT_HI),
	 .OUTPUT(wire_fault[0])
);
antishake #(PROT_FILTER) PROT_OC_IG
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(PROT_Ig),
	 .OUTPUT(wire_fault[1])
);
antishake #(PROT_FILTER) PROT_OC_IR
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(PROT_Ir),
	 .OUTPUT(wire_fault[2])
);
//驱动板故障滤波
antishake #(FO_FILTER) DRV_FAULT_A0
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~A_SO[0]),
	 .OUTPUT(wire_drv_so[0])
);
antishake #(FO_FILTER) DRV_FAULT_L0
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~L_SO[0]),
	 .OUTPUT(wire_drv_so[1])
);
antishake #(FO_FILTER) DRV_FAULT_H0
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~H_SO[0]),
	 .OUTPUT(wire_drv_so[2])
);
antishake #(FO_FILTER) DRV_FAULT_A1
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~A_SO[1]),
	 .OUTPUT(wire_drv_so[3])
);
antishake #(FO_FILTER) DRV_FAULT_L1
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~L_SO[1]),
	 .OUTPUT(wire_drv_so[4])
);
antishake #(FO_FILTER) DRV_FAULT_H1
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~H_SO[1]),
	 .OUTPUT(wire_drv_so[5])
);
antishake #(FO_FILTER) DRV_FAULT_A2
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~A_SO[2]),
	 .OUTPUT(wire_drv_so[6])
);
antishake #(FO_FILTER) DRV_FAULT_L2
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~L_SO[2]),
	 .OUTPUT(wire_drv_so[7])
);
antishake #(FO_FILTER) DRV_FAULT_H2
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~H_SO[2]),
	 .OUTPUT(wire_drv_so[8])
);
antishake #(FO_FILTER) DRV_FAULT_A3
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~A_SO[3]),
	 .OUTPUT(wire_drv_so[9])
);
antishake #(FO_FILTER) DRV_FAULT_L3
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~L_SO[3]),
	 .OUTPUT(wire_drv_so[10])
);
antishake #(FO_FILTER) DRV_FAULT_H3
(
	 .CLK(CLK_100M),
    //.RESET(~RESET_IN),
	 .INPUT(~H_SO[3]),
	 .OUTPUT(wire_drv_so[11])
);

//AD采集
wire [15:0] Udc;
wire [15:0] Ir;
wire [15:0] Ig;
wire [15:0] Temp;

ad7606 /*#(
 .CONV_FREQ(100000),             			  	//采样频率100K
 .CONV_LOW(5),         	                  	//convst低电平为50ns
 .READCLK_CYCLE(6),						  			//读取控制信号脉冲周期为60ns
 .SYSCLK_FREQ(100000000)                    	//system clock 100MHZ
)*/ AD
( 
	.clk(CLK_100M),               			//system clock
	.data(AD_DATA),	                   		//AD7606 16bit数据线
	.convst(CONVST),					 	//状态转换输入控制
	.ad_reset(AD_RST),					 	//AD7606复位 
	.rd_n(AD_RD),						 	//读取控制信号
	.cs_n(AD_CS),						 	//片选
	.channel1_data(Udc),    				//通道1数据
//	.channel2_data(Ir),    					//通道2数据
//	.channel3_data(Ig),    					//通道3数据	//清华配置
	.channel2_data(Ig),    					//通道2数据  //20261010 根据实际接线修改
	.channel3_data(Ir),    					//通道3数据
	.channel4_data(Temp)    				//通道4数据	
  );

// The legacy UnitBoard FIR is a 32-tap signed Q18 core (the generated
// filter.mif coefficients sum to 2^18). FILTER_IN is combinational so the FIR
// samples the selected channel on the same edge.
wire [15:0] FILTER_IN;
wire signed [34:0] FILTER_OUT;
reg [15:0] RESULT0;
reg [15:0] RESULT1;
reg [15:0] RESULT2;
reg [15:0] RESULT3;

wire [1:0] CHAIN_IN;
wire [1:0] CHAIN_OUT;

filter fir(.clk(CLK_100M), .din(FILTER_IN), .dout(FILTER_OUT), .chan_in(CHAIN_IN), .chan_out(CHAIN_OUT));

assign FILTER_IN = (CHAIN_IN == 2'b00) ? Udc  :
			   (CHAIN_IN == 2'b01) ? Ir   :
			   (CHAIN_IN == 2'b10) ? Ig   : Temp;

always @ (posedge CLK_100M)
begin
	case(CHAIN_OUT)
		2'b00: RESULT0 <= {FILTER_OUT[34],FILTER_OUT[32:18]};
		2'b01: RESULT1 <= {FILTER_OUT[34],FILTER_OUT[32:18]};
		2'b10: RESULT2 <= {FILTER_OUT[34],FILTER_OUT[32:18]};
		2'b11: RESULT3 <= {FILTER_OUT[34],FILTER_OUT[32:18]};
	endcase
end
//电压电流变比处理
wire [31:0] mult_Udc;
wire [31:0] mult_Ir;
wire [31:0] mult_Ig;

mul_16X16 Mul_Udc( 
  .ce   (1'b1      ),
  .rst  (1'b0      ),
  .clk  (CLK_100M ),
  .a    (RESULT0  ),
  .b     (16'd3968 ),//62K采样电阻用
//旧系数 40K采样电阻用(16'd2065 ),
  .p    (mult_Udc )
);

mul_16X16 Mul_Ir(
  .ce   (1'b1      ),
  .rst  (1'b0      ),
  .clk  (CLK_100M ),
  .a    (RESULT1  ),
//  .b    (16'd4000 ), //旧系数 对应两个100Ω采样电阻并联
  .b    (16'd6000 ), //20261016改 新版三个采样电阻并联
  .p    (mult_Ir  )
);		//10*Ir

mul_16X16 Mul_Ig(
  .ce   (1'b1      ),
  .rst  (1'b0      ),
  .clk  (CLK_100M ),
  .a    (RESULT2  ),
//  .b    (16'd4000 ), //旧系数 对应两个100Ω采样电阻并联
  .b    (16'd6000 ), //20261016改 新版三个采样电阻并联
  .p    (mult_Ig  )
);		//10*Ig

//计算温度

wire [31:0] mult1_out;
wire [31:0] mult2_out;
wire [31:0] divide_out;
wire        divide_rfd;

reg  [15:0] SCI_UDC = 13'd0;
reg  [15:0] SCI_IR = 13'd0;  
reg  [15:0] SCI_IG = 13'd0;
reg  [31:0] Sum_TX10 = 32'd0;
reg  [15:0] PT100 = 16'd0;
reg  [12:0] AVG_TEMP = 13'd0;
reg  [31:0] div_up   = 32'd0;
reg  [31:0] div_down = 32'd1;
reg  [15:0] Res10 = 16'd0;		//10倍电阻值
reg         div_start = 1'b0;
reg         div_wait = 1'b0;
reg         div_busy_seen = 1'b0;
reg [1:0]   pt100_delay = 2'd0;

mul_16X16 Mul_Temp(
  .ce   (1'b1      ),
  .rst  (1'b0      ),
  .clk  (CLK_100M ),
  .a    (16'd10000),
  .b    (Temp     ),
  .p    (mult1_out)
);


mul_16X16 Mul_Res(
  .ce   (1'b1              ),
  .rst  (1'b0              ),
  .clk  (CLK_100M	 		),
  .a    (16'd2659			),
  .b    (Res10-16'd1000	),
  .p    (mult2_out 		)
);


div_32X32  ResX10 (
  .rfd          (divide_rfd ),
  .clk          (CLK_100M   ),
  .start        (div_start  ),
  .dividend     (div_up	 ),
  .quotient     (divide_out),
  .divisor      (div_down	 ),
  .fractional   (           )
);

reg [15:0]	sample_cnt = 16'd0;
wire   clk_equ;  

always @ (posedge CLK_100M)
begin
	if(clk_equ)
	begin	
		if(sample_cnt == 16'd8191)	
		begin
			sample_cnt <= 16'd0;
			AVG_TEMP <= (Sum_TX10>>13);
			Sum_TX10 <= 32'd0;
		end
		else								
		begin
			sample_cnt <= sample_cnt + 16'd1;
			Sum_TX10 <= Sum_TX10 + PT100;
		end
	end
end

reg [15:0] 	sci_cnt = 16'h0000;
reg [15:0]	sci_out;
reg			TxD_start;
wire			TxD_busy;

assign clk_equ = (sci_cnt == 16'd1000);

always @ (posedge CLK_100M)
begin
	if(TxD_start)	TxD_start <= 1'b0;
	div_start <= 1'b0;
	if((sci_cnt == 16'd4999) || pos_sync)
	begin
		sci_cnt <= 16'd0;
		div_wait <= 1'b0;
		div_busy_seen <= 1'b0;
		pt100_delay <= 2'd0;
	end
	else
	begin
		sci_cnt <= sci_cnt + 16'd1;
		
		//if(sci_cnt == 16'd1)		sci_out <= {3'b000, RESULT0};
		if(sci_cnt == 16'd1)		sci_out <= {3'b000, SCI_UDC[12:0]};
		if(sci_cnt == 16'd5)		TxD_start <= 1'b1;
		
		//if(sci_cnt == 16'd601)	sci_out <= {3'b001, RESULT1};
		if(sci_cnt == 16'd601)	sci_out <= {3'b001, SCI_IR[12:0]};
		if(sci_cnt == 16'd605)	TxD_start <= 1'b1;
		
		//if(sci_cnt == 16'd1201)	sci_out <= {3'b010, RESULT2};
		if(sci_cnt == 16'd1201)	sci_out <= {3'b010, SCI_IG[12:0]};
		if(sci_cnt == 16'd1205)	TxD_start <= 1'b1;

		//if(sci_cnt == 16'd1801)	sci_out <= {3'b011, RESULT3};
		if(sci_cnt == 16'd1801)	sci_out <= {3'b011, AVG_TEMP};
		if(sci_cnt == 16'd1805)	TxD_start <= 1'b1;
		
		if(sci_cnt == 16'd2401)	sci_out <= TEST_BYPASS_EXTERNAL_INTERLOCKS ? {2'b10, 14'h0000} : {2'b10, block_dab, block_svg, block_ot, block_ir, block_ig, block_dc, reg_fault};
		if(sci_cnt == 16'd2405)	TxD_start <= 1'b1;
		
		if(sci_cnt == 16'd3001)	sci_out <= {4'b1110, reg_drv_dab, reg_drv_svg};
		if(sci_cnt == 16'd3005)	TxD_start <= 1'b1;
		
		if(sci_cnt == 16'd3601)	sci_out <= {4'b1100, SIN_L[12:1]};
		if(sci_cnt == 16'd3605)	TxD_start <= 1'b1;

		if(sci_cnt == 16'd4201)	sci_out <= {4'b1101, SIN_R[12:1]};
		if(sci_cnt == 16'd4205)	TxD_start <= 1'b1;
		
		if(sci_cnt == 16'd4800) SCI_UDC  <= (mult_Udc>>16);
		if(sci_cnt == 16'd4810) SCI_IR   <= (mult_Ir>>16);
		if(sci_cnt == 16'd4820) SCI_IG   <= (mult_Ig>>16);
		if(sci_cnt == 16'd4830) div_up   <= (mult1_out + 32'd312891538);
		if(sci_cnt == 16'd4840) div_down <= (32'd381575 - Temp);
		if(sci_cnt == 16'd4841)
		begin
			div_start <= 1'b1;
			div_wait <= 1'b1;
			div_busy_seen <= 1'b0;
		end
		if(div_wait && !divide_rfd)
		begin
			div_busy_seen <= 1'b1;
		end
		if(div_wait && div_busy_seen && divide_rfd)
		begin
			Res10 <= divide_out[15:0];
			div_wait <= 1'b0;
			div_busy_seen <= 1'b0;
			pt100_delay <= 2'd2;
		end
		else if(pt100_delay != 2'd0)
		begin
			pt100_delay <= pt100_delay - 2'd1;
			if(pt100_delay == 2'd1)	PT100 <= (mult2_out>>10);
		end
	end
end

async_transmitter TX(.clk(CLK_100M), .datain(sci_out), .TXD(OP_TX), .WR(TxD_start), .BUSY(TxD_busy));


wire [3:0]TEST;
assign TEST[0] = RxD_data_ready & (RxD_data[15:12]==4'b1101);//OP_TX;			//TEST IO4	C15
assign TEST[1] = TxD_start & (sci_out[15:12]==4'b1100);//AD_RST;			//TEST IO6	D14
assign TEST[2] = 1'b0;//AD_RD;			//TEST IO8	E15
assign TEST[3] = 1'b0;//AD_CS;			//TEST IO10 F15

assign LED = reg_fault;
// Bridge-running status is also the source for the active-low status LEDs.
wire svg_running;
wire dab_running;

assign svg_running = SVG_EN & (TEST_BYPASS_SVG_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_svg & ~shutdown_svg));
assign dab_running = DAB_EN & (TEST_BYPASS_EXTERNAL_INTERLOCKS ? 1'b1 : (~fault_dab & ~shutdown_dab));

assign LED_SVG = ~svg_running;
assign LED_DAB = ~dab_running;

// During precharge neither bridge is healthy-running, so the fan follows
// the local DC-link voltage hysteresis and overvoltage input. Once either bridge
// is running, FAN_OFF directly controls both fan outputs.
// FP_OUT0 and FP_OUT1 are high-active and drive the two fan loads together.
reg FAN_STM = 1'b0;
reg fan_out = 1'b0;

always @ (posedge CLK_100M)
begin
	if(svg_running || dab_running)
	begin
		fan_out <= ~FAN_OFF;
	end
	else
	begin
		fan_out <= FAN_STM;
		case(FAN_STM)
//		1'b0: if(((SCI_UDC > 16'd280) && (reg_fault[6] == 1'b0)) || (reg_fault[0] == 1'b1)) FAN_STM <= 1'b1;
//		1'b1: if((SCI_UDC < 16'd130) || (reg_fault[6] == 1'b1)) FAN_STM <= 1'b0;
        1'b0: if((SCI_UDC > 16'd235) || (reg_fault[0] == 1'b1)) FAN_STM <= 1'b1;
		1'b1: if((SCI_UDC < 16'd170)) FAN_STM <= 1'b0;
		endcase
	end
end

assign FP_OUT0 = fan_out;
assign FP_OUT1 = fan_out;

wire [3:0]op_state;
assign op_state = 4'b0001;
wire[2:0] FPGA_LED;
assign OP_TX2 = FPGA_LED[0];
//assign OP_FO = FPGA_LED[1];
dual_led led_inst(
    .clk(CLK_100M),
   .rst_n(1'b1),
	.op_state(op_state),
	.led(FPGA_LED)
);

endmodule
