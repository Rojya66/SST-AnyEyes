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
module antishake
#(
 parameter FILTER_CNT = 20             			  //20个时钟周期
)
(
	input  CLK,               			//system clock
	input  RESET,               		//system reset 
	input  INPUT,                    //
	output OUTPUT
  );
  
//Filter 
	reg [1:0] sync = 2'b00;
	reg [15:0] cnt = FILTER_CNT;
	reg latch = 1'b1;
	always @ (posedge CLK)
	begin
		if(RESET)
			begin
				sync <= 2'b00;
				cnt <= FILTER_CNT;
				latch <= 1'b1;
			end
		else
			begin
				sync <= {sync[0], INPUT};
				if(sync==2'b11 && cnt!=FILTER_CNT)	cnt <= cnt + 16'h0001;
				else 
				if(sync==2'b00 && cnt!=0) cnt <= cnt - 16'h0001;
				
				if(cnt==0) latch <= 1'b0;
				else
				if(cnt==FILTER_CNT) latch <= 1'b1;
			end
	end

	assign OUTPUT = latch;

endmodule
