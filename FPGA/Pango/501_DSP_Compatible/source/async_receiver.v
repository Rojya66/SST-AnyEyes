// UART receiver based on fpga4fun.com, (c) KNJN LLC - 2003, 2004, 2005, 2006.
module async_receiver #(
	parameter BANDRATE = 16'd32
)(
	input clk,
	input enabled,
	input RxD,
	output reg RxD_data_ready = 1'b0,
	output reg [15:0] RxD_data = 16'd0
);

reg [15:0] sample_count = 16'd0;
wire sample_tick = (sample_count == (BANDRATE >> 4) - 16'd1);

reg [1:0] rx_sync = 2'b11;
reg [1:0] rx_filter_count = 2'b11;
reg rx_bit = 1'b1;
reg [4:0] state = 5'd0;
reg [4:0] bit_spacing = 5'd0;
wire next_bit = (bit_spacing == 5'd20);

always @(posedge clk) begin
	if(!enabled) begin
		sample_count <= 16'd0;
		rx_sync <= 2'b11;
		rx_filter_count <= 2'b11;
		rx_bit <= 1'b1;
		state <= 5'd0;
		bit_spacing <= 5'd0;
		RxD_data_ready <= 1'b0;
		RxD_data <= 16'd0;
	end
	else begin
		rx_sync <= {rx_sync[0], RxD};
		RxD_data_ready <= 1'b0;
		if(sample_tick) begin
			sample_count <= 16'd0;
			if(rx_sync[1] && rx_filter_count != 2'b11)
				rx_filter_count <= rx_filter_count + 2'd1;
			else if(!rx_sync[1] && rx_filter_count != 2'b00)
				rx_filter_count <= rx_filter_count - 2'd1;

			if(rx_filter_count == 2'b00)
				rx_bit <= 1'b0;
			else if(rx_filter_count == 2'b11)
				rx_bit <= 1'b1;

			if(state == 5'd0)
				bit_spacing <= 5'd0;
			else
				bit_spacing <= (bit_spacing[3:0] + 5'd1) | {bit_spacing[4], 4'b0000};

			// 0: 空闲，16~31: 数据位 0~15，1: 停止位。
			if(state == 5'd0) begin
				if(!rx_bit)
					state <= 5'd16;
			end
			else if(next_bit) begin
				if(state[4]) begin
					RxD_data <= {rx_bit, RxD_data[15:1]};
					state <= (state == 5'd31) ? 5'd1 : state + 5'd1;
				end
				else begin
					RxD_data_ready <= rx_bit;
					state <= 5'd0;
				end
			end
		end
		else begin
			sample_count <= sample_count + 16'd1;
		end
	end
end

endmodule
