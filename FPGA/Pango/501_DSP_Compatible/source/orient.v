`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    10:49:13 10/24/2018 
// Design Name: 
// Module Name:    orient 
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
module orient(
    input clk,
    input START,
	 input [15:0] PM,
    input signed [15:0] IG0,
    input signed [15:0] IG1,
    input signed [15:0] IG2,
    input signed [15:0] IG3,
    input signed [15:0] IG4,
    input signed [15:0] IG5,
    input signed [15:0] IG6,
    input signed [15:0] IG7,
    output ORIENT
    );

reg [7:0] cnt = 8'd0;
reg [3:0] ori[7:0];
reg [3:0] add = 4'b0000;

always @ (posedge clk)
begin
	if(START)	
	begin
		cnt <= 8'd0;
	end
	else
	begin
		if(cnt==8'd0)		//所有模块的IG电流采样进行投票，电流为正投1，为负投-1，没有模块则投0
		begin
			cnt <= 8'd1;
			if(PM[0])	
			begin
				if(IG0>0)	ori[0] <= 4'b0001;	
				else			ori[0] <= 4'b1111;
			end
			else
			begin
				ori[0] <= 4'b0000;
			end
			if(PM[1])	
			begin
				if(IG1>0)	ori[1] <= 4'b0001;	
				else			ori[1] <= 4'b1111;
			end
			else
			begin
				ori[1] <= 4'b0000;
			end
			if(PM[2])	
			begin
				if(IG2>0)	ori[2] <= 4'b0001;	
				else			ori[2] <= 4'b1111;
			end
			else
			begin
				ori[2] <= 4'b0000;
			end
			if(PM[3])	
			begin
				if(IG3>0)	ori[3] <= 4'b0001;	
				else			ori[3] <= 4'b1111;
			end
			else
			begin
				ori[3] <= 4'b0000;
			end
			if(PM[4])	
			begin
				if(IG4>0)	ori[4] <= 4'b0001;	
				else			ori[4] <= 4'b1111;
			end
			else
			begin
				ori[4] <= 4'b0000;
			end
			if(PM[5])	
			begin
				if(IG5>0)	ori[5] <= 4'b0001;	
				else			ori[5] <= 4'b1111;
			end
			else
			begin
				ori[5] <= 4'b0000;
			end
			if(PM[6])	
			begin
				if(IG6>0)	ori[6] <= 4'b0001;	
				else			ori[6] <= 4'b1111;
			end
			else
			begin
				ori[6] <= 4'b0000;
			end
			if(PM[7])	
			begin
				if(IG7>0)	ori[7] <= 4'b0001;	
				else			ori[7] <= 4'b1111;
			end
			else
			begin
				ori[7] <= 4'b0000;
			end
		end
		if(cnt==8'd1)			//投票结果相加，确定电流方向，正数表示电流为正，负数表示电流为负
		begin
			add <= ori[0]+ori[1]+ori[2]+ori[3]+ori[4]+ori[5]+ori[6]+ori[7];
		end
	end
end

assign ORIENT = ~add[3];	//用相加结果最高位表示电流方向

endmodule
