module async_transmitter #(
	parameter BANDRATE = 16'd32
)(
	input clk,
	input enabled,
	input [15:0] datain,
	input WR,
	output TXD,
	output BUSY
);

localparam [7:0] FRAME_BITS = 8'd18;
reg [15:0] cnt = 16'd0;
reg [17:0] frame = 18'd0;
reg [7:0] bit_index = FRAME_BITS;
reg tx_bit = 1'b1;
reg idle = 1'b1;
wire bit_tick = (cnt == BANDRATE - 16'd1);

always @(posedge clk) begin
	if(bit_tick)
		cnt <= 16'd0;
	else
		cnt <= cnt + 16'd1;

	if(!enabled) begin
		bit_index <= FRAME_BITS;
		tx_bit <= 1'b1;
		idle <= 1'b1;
	end
	else if(WR) begin
		frame <= {1'b1, datain, 1'b0};
		bit_index <= 8'd0;
	end
	else if(bit_tick) begin
		if(bit_index < FRAME_BITS) begin
			tx_bit <= frame[bit_index];
			bit_index <= bit_index + 8'd1;
			idle <= 1'b0;
		end
		else begin
			tx_bit <= 1'b1;
			idle <= 1'b1;
		end
	end
end

// AFBR-1629Z 的 Data_IN 高电平时不发光。
assign TXD = enabled ? tx_bit : 1'b1;
assign BUSY = ~idle;

endmodule
