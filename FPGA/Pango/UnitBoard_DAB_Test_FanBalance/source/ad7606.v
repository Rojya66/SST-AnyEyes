`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 广州创龙
// Engineer: 
// 
// Create Date:    17:53:06 04/25/2015 
// Design Name: 
// Module Name:    AD7606_Moudle 
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
module ad7606
//#(
// CONV_FREQ=200000,             			  	//采样频率200K
// CONV_LOW =5,         	                 	//convst低电平为50ns
// READCLK_CYCLE=6,						  			//读取控制信号脉冲周期为60ns
// SYSCLK_FREQ=100000000                    //system clock 100MHZ
//)
(
	input  clk,               			 		//system clock
	//input  busy,                        	//AD繁忙
	//input  first_data,                  	//V1通道指示信号
	input  [15:0]data,                   	//AD7606 16bit数据线
	//output [2:0] OS,				 			//过采样模式配置
	//output BYTESEL_N,                    //并串字节选择输入
	//output STBY_N,                       //待机省电控制 
	//output RANGE_AD,                     //模拟输入选择范围 1---10V  0---5V
	output convst,					 				//状态转换输入控制
	output ad_reset,				 				//AD7606复位 
	output rd_n,						 			//读取控制信号
	output cs_n,						 			//片选
	input	 sync,
   output [15:0] channel1_data,    //通道1数据
   output [15:0] channel2_data,    //通道2数据
   output [15:0] channel3_data,    //通道3数据	
	output [15:0] channel4_data     //通道4数据
  );
  
//parameter  CONVL_HIGH=(SYSCLK_FREQ/CONV_FREQ)-CONV_LOW;   //convst_A 和 convst_B 高电平为495个系统时钟
               
						
//定义信号
reg  [15:0] div_cnt = 16'd0;
reg  convst_tick = 1'b1;
reg  read_tick = 1'b1;

reg  [1:0]  reset_state = 2'b00;
reg  [15:0] reset_cnt = 16'd0;
reg  init_reset = 1'b0;

reg[15:0]		  data1=16'd0;
reg[15:0]		  data2=16'd0;
reg[15:0]		  data3=16'd0;
reg[15:0]		  data4=16'd0;
reg[15:0]		  dummy=16'd0;


//200khz计数器
always@(posedge clk)
begin
     //if(div_cnt==((SYSCLK_FREQ/CONV_FREQ)-1)) 
	  if(div_cnt==16'd499 || sync) 
	      div_cnt<=16'd0;
	  else
	       div_cnt<=div_cnt+1'b1;
	      
end

//gen 200khz convst信号 
always@(posedge clk)
begin
		if(div_cnt==16'd0)			convst_tick<=1'b0;  
		if(div_cnt==16'd5)			convst_tick<=1'b1; 
		
		if(div_cnt==16'd420)			read_tick<=1'b0;
		if(div_cnt==16'd423)			data1<=data;
		if(div_cnt==16'd425)			read_tick<=1'b1;
		
		if(div_cnt==16'd430)			read_tick<=1'b0;
		if(div_cnt==16'd433)			data2<=data;
		if(div_cnt==16'd435)			read_tick<=1'b1;
	
		if(div_cnt==16'd440)			read_tick<=1'b0;
		if(div_cnt==16'd443)			dummy<=data;
		if(div_cnt==16'd445)			read_tick<=1'b1;
		
		if(div_cnt==16'd450)			read_tick<=1'b0;
		if(div_cnt==16'd453)			dummy<=data;
		if(div_cnt==16'd455)			read_tick<=1'b1;
		
		if(div_cnt==16'd460)			read_tick<=1'b0;
		if(div_cnt==16'd463)			data3<=16'hFFFF-data;		//Ig 霍尔装反，需要取反(清华注释)
                                                                    //现在是测量Idab,因未用于控制正反无所谓
		if(div_cnt==16'd465)			read_tick<=1'b1;
		
		if(div_cnt==16'd470)			read_tick<=1'b0;
		if(div_cnt==16'd473)			data4<=data;
		if(div_cnt==16'd475)			read_tick<=1'b1;
	
		if(div_cnt==16'd480)			read_tick<=1'b0;
		if(div_cnt==16'd483)			dummy<=data;
		if(div_cnt==16'd485)			read_tick<=1'b1;
		
		if(div_cnt==16'd490)			read_tick<=1'b0;
		if(div_cnt==16'd493)			dummy<=data;
		if(div_cnt==16'd495)			read_tick<=1'b1;
end

assign convst=convst_tick;
assign rd_n=read_tick;
assign cs_n=1'b0;
assign channel1_data=data1;
assign channel2_data=data2;
assign channel3_data=data3;
assign channel4_data=data4;


//gen上电复位信号
always@(posedge clk)
begin
	case (reset_state)
	 2'b00:
			begin
					init_reset<=1'b0;  
				  if(reset_cnt==16'hffff)
						 reset_state<=2'b01;
					else
						 reset_cnt<=reset_cnt+1;
			end
	 2'b01:
			begin
				  init_reset<=1'b1;
				  if(reset_cnt==16'd100)
					  reset_state<=2'b10;
					else
						reset_cnt<=reset_cnt+1;
			end

	 2'b10:
			begin
				  init_reset<=1'b0;
				  reset_cnt<=16'd0;
			end
	  default:
			begin
				  reset_state<=2'b00;
			end
	 endcase
end




//assign OS=3'b000;                      //无过采样，转换频率为 200K
//assign BYTESEL_N=1'b0;                 //设置AD7606位并行接口模式
//assign STBY_N=1'b1;					     //AD正常供电模式
//assign RANGE_AD=1;                     //AD输入范围+-10V	

assign ad_reset = init_reset;
            
endmodule
