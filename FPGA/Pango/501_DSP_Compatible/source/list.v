`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    20:06:05 10/21/2013 
// Design Name: 
// Module Name:    list 
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
module list(
    input clk,
    input START,
    input [15:0] PM,
    input signed [15:0] DC0,
    input signed [15:0] DC1,
    input signed [15:0] DC2,
    input signed [15:0] DC3,
    input signed [15:0] DC4,
    input signed [15:0] DC5,
    input signed [15:0] DC6,
    input signed [15:0] DC7,

	 output [15:0] INDEX0,
	 output [15:0] INDEX1,
	 output [15:0] INDEX2,
	 output [15:0] INDEX3,
	 output [15:0] INDEX4,
	 output [15:0] INDEX5,
	 output [15:0] INDEX6,
	 output [15:0] INDEX7
    );

parameter MAX_PM = 4'd8;

reg [7:0] cnt = 8'd0;

reg signed [15:0] maxDC[7:0];
reg [15:0] index[7:0];

reg [7:0] tmp;

reg [15:0] max0[6:0];
reg [15:0] max1[6:0];
reg [15:0] max2[6:0];
reg [15:0] max3[6:0];
reg [15:0] max4[6:0];
reg [15:0] max5[6:0];
reg [15:0] max6[6:0];
reg [15:0] max7[6:0];


assign INDEX0 = index[0];
assign INDEX1 = index[1];
assign INDEX2 = index[2];
assign INDEX3 = index[3];
assign INDEX4 = index[4];
assign INDEX5 = index[5];
assign INDEX6 = index[6];
assign INDEX7 = index[7];


always @ (posedge clk)
begin
	if(START)	
	begin
		cnt <= 8'd0;
	end
	else
	begin
		if(cnt==8'd0)		//初始化直流电压，填补未连接模块的电压值为-MAX,8000
		begin
			cnt <= 8'd1;
			if(PM[0])	maxDC[0] <= DC0;	else maxDC[0] <= 16'h8000;
			if(PM[1])	maxDC[1] <= DC1;	else maxDC[1] <= 16'h8000;
			if(PM[2])	maxDC[2] <= DC2;	else maxDC[2] <= 16'h8000;
			if(PM[3])	maxDC[3] <= DC3;	else maxDC[3] <= 16'h8000;
			if(PM[4])	maxDC[4] <= DC4;	else maxDC[4] <= 16'h8000;
			if(PM[5])	maxDC[5] <= DC5;	else maxDC[5] <= 16'h8000;
			if(PM[6])	maxDC[6] <= DC6;	else maxDC[6] <= 16'h8000;
			if(PM[7])	maxDC[7] <= DC7;	else maxDC[7] <= 16'h8000;
		end
		if(cnt==8'd1)	
		begin
			cnt <= 8'd2;
			if(maxDC[0]>=maxDC[1])	max0[0]<=0;			else	max0[0]<=1;
			if(maxDC[0]>=maxDC[2])	max0[1]<=0;			else	max0[1]<=1;
			if(maxDC[0]>=maxDC[3])	max0[2]<=0;			else	max0[2]<=1;
			if(maxDC[0]>=maxDC[4])	max0[3]<=0;			else	max0[3]<=1;
			if(maxDC[0]>=maxDC[5])	max0[4]<=0;			else	max0[4]<=1;
			if(maxDC[0]>=maxDC[6])	max0[5]<=0;			else	max0[5]<=1;
			if(maxDC[0]>=maxDC[7])	max0[6]<=0;			else	max0[6]<=1;

			
			if(maxDC[1]>maxDC[0])	max1[0]<=0;			else	max1[0]<=1;
			if(maxDC[1]>=maxDC[2])	max1[1]<=0;			else	max1[1]<=1;
			if(maxDC[1]>=maxDC[3])	max1[2]<=0;			else	max1[2]<=1;
			if(maxDC[1]>=maxDC[4])	max1[3]<=0;			else	max1[3]<=1;
			if(maxDC[1]>=maxDC[5])	max1[4]<=0;			else	max1[4]<=1;
			if(maxDC[1]>=maxDC[6])	max1[5]<=0;			else	max1[5]<=1;
			if(maxDC[1]>=maxDC[7])	max1[6]<=0;			else	max1[6]<=1;

			
			if(maxDC[2]>maxDC[0])	max2[0]<=0;			else	max2[0]<=1;
			if(maxDC[2]>maxDC[1])	max2[1]<=0;			else	max2[1]<=1;
			if(maxDC[2]>=maxDC[3])	max2[2]<=0;			else	max2[2]<=1;
			if(maxDC[2]>=maxDC[4])	max2[3]<=0;			else	max2[3]<=1;
			if(maxDC[2]>=maxDC[5])	max2[4]<=0;			else	max2[4]<=1;
			if(maxDC[2]>=maxDC[6])	max2[5]<=0;			else	max2[5]<=1;
			if(maxDC[2]>=maxDC[7])	max2[6]<=0;			else	max2[6]<=1;

			
			if(maxDC[3]>maxDC[0])	max3[0]<=0;			else	max3[0]<=1;
			if(maxDC[3]>maxDC[1])	max3[1]<=0;			else	max3[1]<=1;
			if(maxDC[3]>maxDC[2])	max3[2]<=0;			else	max3[2]<=1;
			if(maxDC[3]>=maxDC[4])	max3[3]<=0;			else	max3[3]<=1;
			if(maxDC[3]>=maxDC[5])	max3[4]<=0;			else	max3[4]<=1;
			if(maxDC[3]>=maxDC[6])	max3[5]<=0;			else	max3[5]<=1;
			if(maxDC[3]>=maxDC[7])	max3[6]<=0;			else	max3[6]<=1;

			
			if(maxDC[4]>maxDC[0])	max4[0]<=0;			else	max4[0]<=1;
			if(maxDC[4]>maxDC[1])	max4[1]<=0;			else	max4[1]<=1;
			if(maxDC[4]>maxDC[2])	max4[2]<=0;			else	max4[2]<=1;
			if(maxDC[4]>maxDC[3])	max4[3]<=0;			else	max4[3]<=1;
			if(maxDC[4]>=maxDC[5])	max4[4]<=0;			else	max4[4]<=1;
			if(maxDC[4]>=maxDC[6])	max4[5]<=0;			else	max4[5]<=1;
			if(maxDC[4]>=maxDC[7])	max4[6]<=0;			else	max4[6]<=1;

			
			if(maxDC[5]>maxDC[0])	max5[0]<=0;			else	max5[0]<=1;
			if(maxDC[5]>maxDC[1])	max5[1]<=0;			else	max5[1]<=1;
			if(maxDC[5]>maxDC[2])	max5[2]<=0;			else	max5[2]<=1;
			if(maxDC[5]>maxDC[3])	max5[3]<=0;			else	max5[3]<=1;
			if(maxDC[5]>maxDC[4])	max5[4]<=0;			else	max5[4]<=1;
			if(maxDC[5]>=maxDC[6])	max5[5]<=0;			else	max5[5]<=1;
			if(maxDC[5]>=maxDC[7])	max5[6]<=0;			else	max5[6]<=1;

			
			if(maxDC[6]>maxDC[0])	max6[0]<=0;			else	max6[0]<=1;
			if(maxDC[6]>maxDC[1])	max6[1]<=0;			else	max6[1]<=1;
			if(maxDC[6]>maxDC[2])	max6[2]<=0;			else	max6[2]<=1;
			if(maxDC[6]>maxDC[3])	max6[3]<=0;			else	max6[3]<=1;
			if(maxDC[6]>maxDC[4])	max6[4]<=0;			else	max6[4]<=1;
			if(maxDC[6]>maxDC[5])	max6[5]<=0;			else	max6[5]<=1;
			if(maxDC[6]>=maxDC[7])	max6[6]<=0;			else	max6[6]<=1;

			
			if(maxDC[7]>maxDC[0])	max7[0]<=0;			else	max7[0]<=1;
			if(maxDC[7]>maxDC[1])	max7[1]<=0;			else	max7[1]<=1;
			if(maxDC[7]>maxDC[2])	max7[2]<=0;			else	max7[2]<=1;
			if(maxDC[7]>maxDC[3])	max7[3]<=0;			else	max7[3]<=1;
			if(maxDC[7]>maxDC[4])	max7[4]<=0;			else	max7[4]<=1;
			if(maxDC[7]>maxDC[5])	max7[5]<=0;			else	max7[5]<=1;
			if(maxDC[7]>maxDC[6])	max7[6]<=0;			else	max7[6]<=1;

		end
		if(cnt==8'd2)	
		begin
			cnt <= 8'd3;
			index[0] <= max0[0]+max0[1]+max0[2]+max0[3]+max0[4]+max0[5]+max0[6];
			index[1] <= max1[0]+max1[1]+max1[2]+max1[3]+max1[4]+max1[5]+max1[6];
			index[2] <= max2[0]+max2[1]+max2[2]+max2[3]+max2[4]+max2[5]+max2[6];
			index[3] <= max3[0]+max3[1]+max3[2]+max3[3]+max3[4]+max3[5]+max3[6];
			index[4] <= max4[0]+max4[1]+max4[2]+max4[3]+max4[4]+max4[5]+max4[6];
			index[5] <= max5[0]+max5[1]+max5[2]+max5[3]+max5[4]+max5[5]+max5[6];
			index[6] <= max6[0]+max6[1]+max6[2]+max6[3]+max6[4]+max6[5]+max6[6];
			index[7] <= max7[0]+max7[1]+max7[2]+max7[3]+max7[4]+max7[5]+max7[6];
		end
	end
end

endmodule
