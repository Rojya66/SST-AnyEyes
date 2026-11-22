`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    09:54:46 05/17/2017 
// Design Name: 
// Module Name:    deadzone 
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
module deadzone
#(
 parameter DEADZONE=120             			  //120个时钟周期
)
(
    input CLK,
    input RESET,
    input INA,
    input INB,
	 input ENABLE,
	 
    output OUTA,
    output OUTB
    );

reg [15:0] reg_dead = DEADZONE;		
reg [1:0] ina_sync = 2'b00;
reg [1:0] inb_sync = 2'b00;
reg [15:0] ina_cnt = 16'h0000;
reg [15:0] inb_cnt = 16'h0000;
reg outa = 1'b0;
reg outb = 1'b0;

always @ (posedge CLK)
begin
	if (RESET)
	begin
		reg_dead <= DEADZONE;
		ina_sync <= 2'b00;
		inb_sync <= 2'b00;
		ina_cnt <= 16'h0000;
		inb_cnt <= 16'h0000;
		outa <= 1'b0;
		outb <= 1'b0;
	end
	else 
	begin
		reg_dead <= DEADZONE;
		ina_sync <= {ina_sync[0], INA};
		inb_sync <= {inb_sync[0], INB};
		
		if(ina_sync == 2'b00)
		begin
			ina_cnt <= 16'h0000;
			outa <= 1'b0;
		end
		else if(ina_sync == 2'b11)
		begin
			if(ina_cnt < reg_dead) 	ina_cnt <= ina_cnt + 16'h0001;
			else 							outa <= 1'b1;
		end
		else
		begin
			outa <= outa;
		end
		
		if(inb_sync == 2'b00)
		begin
			inb_cnt <= 16'h0000;
			outb <= 1'b0;
		end
		else if(inb_sync == 2'b11)
		begin
			if(inb_cnt < reg_dead) 	inb_cnt <= inb_cnt + 16'h0001;
			else 							outb <= 1'b1;
		end
		else
		begin
			outb <= outb;
		end
	end
end

assign OUTA = outa & ENABLE;
assign OUTB = outb & ENABLE;

endmodule
