`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    14:41:08 10/14/2013 
// Design Name: 
// Module Name:    calc 
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
module pwm(
    input clk,
    input START,
	 input [15:0] N,		//A/B/C每相功率模块的数量
	 input ORI,				//1: 电流正向  0：电流反向
	 input ORDER,			//1：平均直流电压低  0：平均直流电压高
	 input [15:0] TAD,	//开关频率对应的clock数
	 //input [15:0] PM,		//此PWM插件上对应的模块配置
	 input [15:0] NUM_PM,//此PWM插件上的模块数量，可以由PM计算得到
	 input [15:0] DEBUG,	//设置输出的中间变量，在F6/F7位置
	 
	 input [15:0] LREF,	//左桥臂参考电压
	 input [15:0] RREF,	//右桥臂参考电压, RREF=Z-LREF
	 
	 //output [15:0] LTotalLayerNum,	
	 //output [15:0] RTotalLayerNum,
	 //output [15:0] LLayerActive,
	 //output [15:0] RLayerActive,
	 //output [15:0] LIntercept,
	 //output [15:0] RIntercept,
	 //output [15:0] LDuty,
	 //output [15:0] RDuty,
	 //output [15:0] LOnNum,
	 //output [15:0] ROnNum,
	 output [15:0] LDebug,
	 output [15:0] RDebug,
	 output [15:0] LDebugNo,
	 output [15:0] RDebugNo,
	 

    output [15:0] PWML0,
	 output [15:0] PWML1,
	 output [15:0] PWML2,
	 output [15:0] PWML3,
	 output [15:0] PWML4,
	 output [15:0] PWML5,
	 output [15:0] PWML6,
	 output [15:0] PWML7,

    output [15:0] PWMR0,
	 output [15:0] PWMR1,
	 output [15:0] PWMR2,
	 output [15:0] PWMR3,
	 output [15:0] PWMR4,
	 output [15:0] PWMR5,
	 output [15:0] PWMR6,
	 output [15:0] PWMR7,

/*  output [3:0] orPWM0,
	 output [3:0] orPWM1,
	 output [3:0] orPWM2,
	 output [3:0] orPWM3,
	 output [3:0] orPWM4,
	 output [3:0] orPWM5,
	 output [3:0] orPWM6,
	 output [3:0] orPWM7,

	 
	 output [3:0] PWM0,
	 output [3:0] PWM1,
	 output [3:0] PWM2,
	 output [3:0] PWM3,
	 output [3:0] PWM4,
	 output [3:0] PWM5,
	 output [3:0] PWM6,
	 output [3:0] PWM7,

	
	 input [15:0] exPWM0,
	 input [15:0] exPWM1,
	 input [15:0] exPWM2,
	 input [15:0] exPWM3,
	 input [15:0] exPWM4,
	 input [15:0] exPWM5,
	 input [15:0] exPWM6,
	 input [15:0] exPWM7,
	*/
	 input [15:0] INDEX0,
	 input [15:0] INDEX1,
	 input [15:0] INDEX2,
	 input [15:0] INDEX3,
	 input [15:0] INDEX4,
	 input [15:0] INDEX5,
	 input [15:0] INDEX6,
	 input [15:0] INDEX7/*,

	 output [15:0] portLF,
	 output [15:0] portLT,
	 output [15:0] portRF,
	 output [15:0] portRT*/
    );

parameter MAX_N = 8'd16;
parameter MAX_PM = 8'd8;

wire 	N_is_even;
reg [15:0] TotalLayerNum_L;
reg [15:0] TotalLayerNum_R;
reg [15:0] LayerActive_L;
reg [15:0] LayerActive_R;
reg [15:0] Intercept_L;
reg [15:0] Intercept_R;
reg [15:0] Duty_L;
reg [15:0] Duty_R;
reg [15:0] OnNum_L;
reg [15:0] OnNum_R;
reg [15:0] Debug_L;
reg [15:0] Debug_R;
reg [15:0] Debug_L_No;
reg [15:0] Debug_R_No;

reg [7:0] cnt;
reg [7:0] tmp;
reg [7:0] index;
reg [7:0] point;
reg [15:0] LF;
reg [15:0] LT;
reg [15:0] RF;
reg [15:0] RT;

reg [3:0] PWM[7:0];		//当前
//reg [15:0] exPWM[7:0];	//上一次
reg [3:0] orPWM[7:0];
reg [15:0] PWML[7:0];
reg [15:0] PWMR[7:0];

assign N_is_even = (N == ((N>>1)<<1));

//assign LTotalLayerNum = TotalLayerNum_L;
//assign RTotalLayerNum = TotalLayerNum_R;
//assign LLayerActive = LayerActive_L;
//assign RLayerActive = LayerActive_R;
//assign LIntercept = Intercept_L;
//assign RIntercept = Intercept_R;
//assign LDuty = Duty_L;
//assign RDuty = Duty_R;
//assign LOnNum = OnNum_L;
//assign ROnNum = OnNum_R;
assign LDebug = Debug_L;
assign RDebug = Debug_R;
assign LDebugNo = Debug_L_No;
assign RDebugNo = Debug_R_No;

always @ (posedge clk)
begin
	case(DEBUG)
	16'h0001: 
		begin
			Debug_L_No <= TotalLayerNum_L;
			Debug_R_No <= TotalLayerNum_R;
		end
	16'h0002:
		begin
			Debug_L_No <= LayerActive_L;
			Debug_R_No <= LayerActive_R;
		end
	16'h0003:
		begin
			Debug_L_No <= Intercept_L;
			Debug_R_No <= Intercept_R;
		end
	16'h0004:
		begin
			Debug_L_No <= Duty_L;
			Debug_R_No <= Duty_R;
		end
	16'h0005:
		begin
			Debug_L_No <= OnNum_L;
			Debug_R_No <= OnNum_R;
		end
	16'h0006:
		begin
			Debug_L_No <= {LF[7:0],RF[7:0]};
			Debug_R_No <= {LT[7:0],RT[7:0]};
		end
	default:
		begin
			Debug_L_No <= {LF[7:0],RF[7:0]};
			Debug_R_No <= {LT[7:0],RT[7:0]};
		end
	endcase
end

//assign orPWM0 = orPWM[0];
//assign orPWM1 = orPWM[1];
//assign orPWM2 = orPWM[2];
//assign orPWM3 = orPWM[3];
//assign orPWM4 = orPWM[4];
//assign orPWM5 = orPWM[5];
//assign orPWM6 = orPWM[6];
//assign orPWM7 = orPWM[7];


//assign PWM0 = PWM[0];
//assign PWM1 = PWM[1];
//assign PWM2 = PWM[2];
//assign PWM3 = PWM[3];
//assign PWM4 = PWM[4];
//assign PWM5 = PWM[5];
//assign PWM6 = PWM[6];
//assign PWM7 = PWM[7];

assign PWML0 = PWML[0];
assign PWML1 = PWML[1];
assign PWML2 = PWML[2];
assign PWML3 = PWML[3];
assign PWML4 = PWML[4];
assign PWML5 = PWML[5];
assign PWML6 = PWML[6];
assign PWML7 = PWML[7];

assign PWMR0 = PWMR[0];
assign PWMR1 = PWMR[1];
assign PWMR2 = PWMR[2];
assign PWMR3 = PWMR[3];
assign PWMR4 = PWMR[4];
assign PWMR5 = PWMR[5];
assign PWMR6 = PWMR[6];
assign PWMR7 = PWMR[7];

assign portLF = LF;
assign portLT = LT;
assign portRF = RF;
assign portRT = RT;

wire [15:0] divide1_out;
wire [15:0] divide2_out;
wire [15:0] divide3_out;
wire [15:0] frac1_out;
wire [15:0] frac2_out;
wire [15:0] frac3_out;
wire div1_rfd;
wire div2_rfd;
wire div3_rfd;
wire div_ready;

assign div_ready = div1_rfd & div2_rfd & div3_rfd;

unsigned_div_16X16  div_16X16_1 (
  .rfd          (div1_rfd  ),
  .clk          (clk  ),
  .dividend     (LREF ),
  .quotient     (divide1_out),
  .divisor      (TAD ),
  .fractional   (frac1_out  )
);

unsigned_div_16X16  div_16X16_2 (
  .rfd          (div2_rfd  ),
  .clk          (clk  ),
  .dividend     (RREF-(TAD>>1) ),
  .quotient     (divide2_out),
  .divisor      (TAD ),
  .fractional   (frac2_out  )
);

unsigned_div_16X16  div_16X16_3 (
  .rfd          (div3_rfd  ),
  .clk          (clk  ),
  .dividend     (RREF ),
  .quotient     (divide3_out),
  .divisor      (TAD ),
  .fractional   (frac3_out  )
);

wire [7:0] CARD_NUM;
assign CARD_NUM = (N==NUM_PM)? 8'd1:8'd2;		//插件上的模块数量是否与模块总数量相等，相等的话只有一块插件，否则有两块插件


always @ (posedge clk)
begin
	if(START)	cnt <= 8'd0;
	else
	begin
		// Wait for the register-based dividers before using quotient/remainder.
		if(cnt==8'd0)
		begin
			if(div_ready)
			begin
				cnt <= 8'd1;
				if(N_is_even)
				begin
					TotalLayerNum_L <= (N>>1);
					TotalLayerNum_R <= (N>>1)+1;	
					LayerActive_L <= divide1_out;
					Intercept_L <= frac1_out;	
					if(RREF<(TAD>>1))
					begin
						LayerActive_R <= 0;	//层定义从0～N-1
						Intercept_R <= RREF;
					end
					else
					begin
						LayerActive_R <= divide2_out+1;
						Intercept_R <= frac2_out; 
					end
				end
				else
				begin
					TotalLayerNum_L <= ((N+1)>>1);
					TotalLayerNum_R <= ((N+1)>>1);
					LayerActive_L <= divide1_out;
					LayerActive_R <= divide3_out;
					Intercept_L <= frac1_out;
					Intercept_R <= frac3_out;
				end
			end
			else
			begin
				cnt <= 8'd0;
			end
		end
		if(cnt==8'd1)
		begin
			cnt <= 8'd2;
			if(N_is_even)	//N为偶数
			begin
				Duty_L <= Intercept_L;
				//计算左桥臂
				if(LayerActive_L==0)	
				begin
					OnNum_L <= 0;
				end
				else
				begin
					OnNum_L <= (LayerActive_L<<1);
				end
				//计算右桥臂
				if(LayerActive_R==0)	
				begin
					OnNum_R <= 0;
					Duty_R <= Intercept_R;
				end
				else if(LayerActive_R==(N>>1))
				begin
					OnNum_R <= N-2;
					Duty_R <= Intercept_R+(TAD>>1);
				end
				else
				begin
					OnNum_R <= (LayerActive_R<<1)-1;
					Duty_R <= Intercept_R;
				end
			end
			else				//N为奇数
			begin		
				//计算左桥臂
				if(LayerActive_L==0)	
				begin
					OnNum_L <= 0;
					Duty_L <= Intercept_L;
				end
				else if(LayerActive_L==((N+1)>>1)-1)
				begin
					OnNum_L <= N-2;
					Duty_L <= Intercept_L+(TAD>>1);
				end
				else
				begin
					OnNum_L <= (LayerActive_L<<1);
					Duty_L <= Intercept_L;
				end
				//计算右桥臂
				if(LayerActive_R==0)	
				begin
					OnNum_R <= 0;
					Duty_R <= Intercept_R;
				end
				else if(LayerActive_R==((N+1)>>1)-1)
				begin
					OnNum_R <= N-2;
					Duty_R <= Intercept_R+(TAD>>1);
				end
				else
				begin
					OnNum_R <= (LayerActive_R<<1);
					Duty_R <= Intercept_R;
				end
			end
		end
		if(cnt==8'd2)
		begin
			cnt <= 8'd3;
			//生成DEBUG数据
			//Debug_L <= OnNum_L*TAD + (Duty_L<<1);
			//Debug_R <= OnNum_R*TAD + (Duty_R<<1);
			case(OnNum_L)
				16'd0:	Debug_L <= (Duty_L<<1);
				16'd1:	Debug_L <= TAD + (Duty_L<<1);
				16'd2:	Debug_L <= (TAD<<1) + (Duty_L<<1);
				16'd3:	Debug_L <= TAD + (TAD<<1) + (Duty_L<<1);
				16'd4:	Debug_L <= (TAD<<2) + (Duty_L<<1);
				16'd5:	Debug_L <= TAD + (TAD<<2) + (Duty_L<<1);
				16'd6:	Debug_L <= (TAD<<1) + (TAD<<2) + (Duty_L<<1);
				16'd7:	Debug_L <= TAD + (TAD<<1) + (TAD<<2) + (Duty_L<<1);
				16'd8:	Debug_L <= (TAD<<3) + (Duty_L<<1);
				16'd9:	Debug_L <= TAD + (TAD<<3) + (Duty_L<<1);
				16'd10:	Debug_L <= (TAD<<1) + (TAD<<3) + (Duty_L<<1);
				16'd11:	Debug_L <= TAD + (TAD<<1) + (TAD<<3) + (Duty_L<<1);
				16'd12:	Debug_L <= (TAD<<2) + (TAD<<3) + (Duty_L<<1);
				default:	Debug_L <= 16'hFFFF;
			endcase

			case(OnNum_R)
				16'd0:	Debug_R <= (Duty_R<<1);
				16'd1:	Debug_R <= TAD + (Duty_R<<1);
				16'd2:	Debug_R <= (TAD<<1) + (Duty_R<<1);
				16'd3:	Debug_R <= TAD + (TAD<<1) + (Duty_R<<1);
				16'd4:	Debug_R <= (TAD<<2) + (Duty_R<<1);
				16'd5:	Debug_R <= TAD + (TAD<<2) + (Duty_R<<1);
				16'd6:	Debug_R <= (TAD<<1) + (TAD<<2) + (Duty_R<<1);
				16'd7:	Debug_R <= TAD + (TAD<<1) + (TAD<<2) + (Duty_R<<1);
				16'd8:	Debug_R <= (TAD<<3) + (Duty_R<<1);
				16'd9:	Debug_R <= TAD + (TAD<<3) + (Duty_R<<1);
				16'd10:	Debug_R <= (TAD<<1) + (TAD<<3) + (Duty_R<<1);
				16'd11:	Debug_R <= TAD + (TAD<<1) + (TAD<<3) + (Duty_R<<1);
				16'd12:	Debug_R <= (TAD<<2) + (TAD<<3) + (Duty_R<<1);
				default:	Debug_R <= 16'hFFFF;
			endcase
			
			if(ORI)		//电流正向
			begin
				for(tmp=0; tmp<MAX_N; tmp=tmp+1)
				begin
					//生成左桥臂脉冲
					if(tmp>=0 && tmp<(N-2-OnNum_L))
					begin
						LF[tmp] <= 0;
						LT[tmp] <= 0;
					end
					else if(tmp==(N-2-OnNum_L))
					begin
						LF[tmp] <= 1;
						LT[tmp] <= 0;
					end
					else if(tmp==(N-1-OnNum_L))
					begin
						LF[tmp] <= 0;
						LT[tmp] <= 1;
					end
					else if(tmp>=(N-OnNum_L) && tmp<N)
					begin
						LF[tmp] <= 1;
						LT[tmp] <= 1;
					end
					else
					begin
						LF[tmp] <= 0;
						LT[tmp] <= 0;
					end
					//生成右桥臂脉冲
					if(tmp>=0 && tmp<OnNum_R)
					begin
						RF[tmp] <= 1;
						RT[tmp] <= 1;
					end
					else if(tmp==OnNum_R)
					begin
						RF[tmp] <= 1;
						RT[tmp] <= 0;
					end
					else if(tmp==OnNum_R+1)
					begin
						RF[tmp] <= 0;
						RT[tmp] <= 1;
					end
					else if(tmp>=(OnNum_R+2) && tmp<N)
					begin
						RF[tmp] <= 0;
						RT[tmp] <= 0;
					end
					else
					begin
						RF[tmp] <= 0;
						RT[tmp] <= 0;
					end
				end
			end
			else				//电流负向
			begin
				for(tmp=0; tmp<MAX_N; tmp=tmp+1)
				begin
					//生成左桥臂脉冲
					if(tmp>=0 && tmp<OnNum_L)
					begin
						LF[tmp] <= 1;
						LT[tmp] <= 1;
					end
					else if(tmp==OnNum_L)
					begin
						LF[tmp] <= 1;
						LT[tmp] <= 0;
					end
					else if(tmp==OnNum_L+1)
					begin
						LF[tmp] <= 0;
						LT[tmp] <= 1;
					end
					else if(tmp>=(OnNum_L+2) && tmp<N)
					begin
						LF[tmp] <= 0;
						LT[tmp] <= 0;
					end
					else
					begin
						LF[tmp] <= 0;
						LT[tmp] <= 0;
					end
					//生成右桥臂脉冲
					if(tmp>=0 && tmp<(N-2-OnNum_R))
					begin
						RF[tmp] <= 0;
						RT[tmp] <= 0;
					end
					else if(tmp==(N-2-OnNum_R))
					begin
						RF[tmp] <= 1;
						RT[tmp] <= 0;
					end
					else if(tmp==(N-1-OnNum_R))
					begin
						RF[tmp] <= 0;
						RT[tmp] <= 1;
					end
					else if(tmp>=(N-OnNum_R) && tmp<N)
					begin
						RF[tmp] <= 1;
						RT[tmp] <= 1;
					end
					else
					begin
						RF[tmp] <= 0;
						RT[tmp] <= 0;
					end
				end
			end
		end
		if(cnt==8'd3)
		begin
			cnt <= 8'd4;
			//LF_LT_RF_RT组成的脉冲序列中，0的充电面积最小，N-1的充电面积最大
			if((ORDER == 1'b0) && (NUM_PM != 0))		//平均直流电压高
			begin
				for(index=0; index<MAX_PM; index=index+1)
				begin
					if(index==0)								orPWM[0] <= {LF[0],LT[0],RF[0],RT[0]};
					else if(index>0 && index<NUM_PM-1)	orPWM[index] <= {LF[CARD_NUM+index-1],LT[CARD_NUM+index-1],RF[CARD_NUM+index-1],RT[CARD_NUM+index-1]};
					else if(index==NUM_PM-1) 				orPWM[index] <= {LF[N-1],LT[N-1],RF[N-1],RT[N-1]};
					else 											orPWM[index] <= 4'h0;
				end
			end
			else if((ORDER == 1'b1) && (NUM_PM != 0))		//平均直流电压低
			begin
				for(index=0; index<MAX_PM; index=index+1)
				begin
					if(index==0)								orPWM[0] <= {LF[1],LT[1],RF[1],RT[1]};
					else if(index>0 && index<NUM_PM-1)	orPWM[index] <= {LF[CARD_NUM+index+N-NUM_PM-3],LT[CARD_NUM+index+N-NUM_PM-3],RF[CARD_NUM+index+N-NUM_PM-3],RT[CARD_NUM+index+N-NUM_PM-3]};
					else if(index==NUM_PM-1) 				orPWM[index] <= {LF[N-2],LT[N-2],RF[N-2],RT[N-2]};
					else 											orPWM[index] <= 4'h0;
				end
			end
			else
			begin
				for(index=0; index<MAX_PM; index=index+1)	
				begin
					orPWM[index] <= 4'h0;
				end
			end
		end
		if(cnt==8'd4)
		begin
			cnt <= 8'd5;
			PWM[0] <= orPWM[INDEX0];
			PWM[1] <= orPWM[INDEX1];
			PWM[2] <= orPWM[INDEX2];
			PWM[3] <= orPWM[INDEX3];
			PWM[4] <= orPWM[INDEX4];
			PWM[5] <= orPWM[INDEX5];
			PWM[6] <= orPWM[INDEX6];
			PWM[7] <= orPWM[INDEX7];
			/*
			exPWM[0] <= exPWM0;
			exPWM[1] <= exPWM1;
			exPWM[2] <= exPWM2;
			exPWM[3] <= exPWM3;
			exPWM[4] <= exPWM4;
			exPWM[5] <= exPWM5;
			exPWM[6] <= exPWM6;
			exPWM[7] <= exPWM7;
			*/
		end
		if(cnt==8'd5)
		begin
			cnt <= 8'd6;
			for(point=0; point<MAX_PM; point=point+1)
			begin
				//PWML[point]按照PWM[point]来发,point从0开始
				if(PWM[point]==4'b1111)		//左桥臂脉冲11, 右桥臂脉冲11
				begin
					PWML[point] <= 16'h7FFF;//{1'b0,1'b1,1'b1,13'h1FFF};
					PWMR[point] <= 16'hBFFF;//{1'b1,1'b0,1'b1,13'h1FFF};
				end
				else if(PWM[point]==4'b1110)		//左桥臂脉冲11, 右桥臂脉冲10 
				begin
					PWML[point] <= 16'h7FFF;
					PWMR[point] <= (Duty_R|16'hA000);//{1'b1,1'b0,1'b1,13'h0000};
				end
				else if(PWM[point]==4'b1101)		//左桥臂脉冲11, 右桥臂脉冲01
				begin
					PWML[point] <= 16'h7FFF;
					PWMR[point] <= ((TAD-Duty_R)|16'h8000);
				end
				else if(PWM[point]==4'b1100)		//左桥臂脉冲11, 右桥臂脉冲00
				begin
					PWML[point] <= 16'h7FFF;
					PWMR[point] <= 16'h9FFF;//{1'b1,1'b0,1'b0,13'h1FFF};
				end
				
				else if(PWM[point]==4'b1011)		//左桥臂脉冲10, 右桥臂脉冲11
				begin
					PWML[point] <= (Duty_L|16'h6000);//{1'b0,1'b1,1'b1,13'h0000};
					PWMR[point] <= 16'hBFFF;//{1'b1,1'b0,1'b1,13'h1FFF};
				end
				else if(PWM[point]==4'b1010)		//左桥臂脉冲10, 右桥臂脉冲10 
				begin
					PWML[point] <= (Duty_L|16'h6000);
					PWMR[point] <= (Duty_R|16'hA000);
				end
				else if(PWM[point]==4'b1001)		//左桥臂脉冲10, 右桥臂脉冲01
				begin
					PWML[point] <= (Duty_L|16'h6000);
					PWMR[point] <= ((TAD-Duty_R)|16'h8000);
				end
				else if(PWM[point]==4'b1000)		//左桥臂脉冲10, 右桥臂脉冲00
				begin
					PWML[point] <= (Duty_L|16'h6000);
					PWMR[point] <= 16'h9FFF;//{1'b1,1'b0,1'b0,13'h1FFF};
				end
				
				else if(PWM[point]==4'b0111)		//左桥臂脉冲01, 右桥臂脉冲11
				begin
					PWML[point] <= ((TAD-Duty_L)|16'h4000);//{1'b0,1'b1,1'b0,13'h0000};
					PWMR[point] <= 16'hBFFF;//{1'b1,1'b0,1'b1,13'h1FFF};
				end
				else if(PWM[point]==4'b0110)		//左桥臂脉冲01, 右桥臂脉冲10 
				begin
					PWML[point] <= ((TAD-Duty_L)|16'h4000);//{1'b0,1'b1,1'b0,13'h0000};
					PWMR[point] <= (Duty_R|16'hA000);
				end
				else if(PWM[point]==4'b0101)		//左桥臂脉冲01, 右桥臂脉冲01
				begin
					PWML[point] <= ((TAD-Duty_L)|16'h4000);//{1'b0,1'b1,1'b0,13'h0000};
					PWMR[point] <= ((TAD-Duty_R)|16'h8000);
				end
				else if(PWM[point]==4'b0100)		//左桥臂脉冲01, 右桥臂脉冲00
				begin
					PWML[point] <= ((TAD-Duty_L)|16'h4000);//{1'b0,1'b1,1'b0,13'h0000};
					PWMR[point] <= 16'h9FFF;//{1'b1,1'b0,1'b0,13'h1FFF};
				end
				
				else if(PWM[point]==4'b0011)		//左桥臂脉冲00, 右桥臂脉冲11
				begin
					PWML[point] <= 16'h5FFF;//{1'b0,1'b1,1'b0,13'h1FFF};
					PWMR[point] <= 16'hBFFF;//{1'b1,1'b0,1'b1,13'h1FFF};
				end
				else if(PWM[point]==4'b0010)		//左桥臂脉冲00, 右桥臂脉冲10 
				begin
					PWML[point] <= 16'h5FFF;//{1'b0,1'b1,1'b0,13'h1FFF};
					PWMR[point] <= (Duty_R|16'hA000);
				end
				else if(PWM[point]==4'b0001)		//左桥臂脉冲00, 右桥臂脉冲01
				begin
					PWML[point] <= 16'h5FFF;//{1'b0,1'b1,1'b0,13'h1FFF};
					PWMR[point] <= ((TAD-Duty_R)|16'h8000);
				end
				else if(PWM[point]==4'b0000)		//左桥臂脉冲00, 右桥臂脉冲00
				begin
					PWML[point] <= 16'h5FFF;//{1'b0,1'b1,1'b0,13'h1FFF};
					PWMR[point] <= 16'h9FFF;//{1'b1,1'b0,1'b0,13'h1FFF};
				end
				else
				begin
					PWML[point] <= 16'h5FFF;		//始终为0
					PWMR[point] <= 16'h9FFF;		//始终为0
				end		
				
				
/*				if((PWM[point]&4'b1100)==4'b1100)				//左桥臂脉冲11
				begin
					PWML[point] <= {1'b0,1'b1,1'b1,1'b1,12'hFFF};
				end
				else if((PWM[point]&4'b1100)==4'b1000)			//左桥臂脉冲10
				begin
					PWML[point] <= {1'b0,1'b1,1'b1,Duty_L[12:0]};
				end
				else if((PWM[point]&4'b1100)==4'b0100)			//左桥臂脉冲01
				begin
					PWML[point] <= (TAD-Duty_L)|16'h4000;	//{1'b0,1'b1,1'b0,1'b0,12'h000};
				end
				else if((PWM[point]&4'b1100)==4'b0000)			//左桥臂脉冲00
				begin
					PWML[point] <= {1'b0,1'b1,1'b0,1'b1,12'hFFF};
				end
				else
				begin
					PWML[point] <= 16'h5FFF;
				end
				
				if((PWM[point]&4'b0011)==4'b0011)				//右桥臂脉冲11
				begin
					PWMR[point] <= {1'b1,1'b0,1'b1,1'b1,12'hFFF};
				end
				else if((PWM[point]&4'b0011)==4'b0010)			//右桥臂脉冲10
				begin
					PWMR[point] <= {1'b1,1'b0,1'b1,Duty_R[12:0]};
				end
				else if((PWM[point]&4'b0011)==4'b0001)			//右桥臂脉冲01
				begin
					PWMR[point] <= (TAD-Duty_R)|16'h8000;	//{1'b1,1'b0,1'b0,1'b0,12'h000};
				end
				else if((PWM[point]&4'b0011)==4'b0000)			//右桥臂脉冲00
				begin
					PWMR[point] <= {1'b1,1'b0,1'b0,1'b1,12'hFFF};
				end
				else
				begin
					PWMR[point] <= 16'h9FFF;
				end
*/				
			end
		end
	end
end
endmodule
