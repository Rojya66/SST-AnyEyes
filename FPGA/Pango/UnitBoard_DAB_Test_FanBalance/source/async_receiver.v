// RS-232 RX module
// (c) fpga4fun.com KNJN LLC - 2003, 2004, 2005, 2006

module async_receiver(clk, RxD, RxD_data_ready, RxD_data);
input clk, RxD;
output RxD_data_ready;  // onc clock pulse when RxD_data is valid
output [15:0] RxD_data;

reg [15:0] cnt;
wire clk_equ;    
parameter BANDRATE = 16'd32;

reg [1:0] RxD_sync_inv = 2'b00;
reg [1:0] RxD_cnt_inv = 2'b00;
reg RxD_bit_inv = 1'b0;
reg [4:0] state = 5'b00000;
reg [4:0] bit_spacing = 5'b00000;

////////////////////////////

always@(posedge clk)          
begin
     if(clk_equ)
      cnt <= 16'd0;
     else 
      cnt<=cnt+1'b1;
end
assign clk_equ = (cnt == ((BANDRATE>>4)-16'd1));		//16±¶Æµ¹ý²ÉÑù

always @(posedge clk) 
RxD_sync_inv <= {RxD_sync_inv[0], RxD};

always @(posedge clk)
begin
   if(clk_equ)
	begin
		if( RxD_sync_inv[1] && RxD_cnt_inv!=2'b11) RxD_cnt_inv <= RxD_cnt_inv + 2'b1;
		else 
		if(~RxD_sync_inv[1] && RxD_cnt_inv!=2'b00) RxD_cnt_inv <= RxD_cnt_inv - 2'b1;

		if(RxD_cnt_inv==2'b00) RxD_bit_inv <= 1'b0;
		else
		if(RxD_cnt_inv==2'b11) RxD_bit_inv <= 1'b1;
	end
end

// "next_bit" controls when the data sampling occurs
// depending on how noisy the RxD is, different values might work better
// with a clean connection, values from 18 to 21 work
wire next_bit = (bit_spacing==5'd20);

always @(posedge clk)
begin
	if(clk_equ)
	begin
	if(state==0)
		bit_spacing <= 5'b00000;
	else
		bit_spacing <= (bit_spacing[3:0] + 5'b00001) | {bit_spacing[4], 4'b0000};
	end
end

always @(posedge clk)
begin
   if(clk_equ)
	begin
	case(state)
		5'b00000: if(~RxD_bit_inv) state <= 5'b10000;  // start bit found?
		5'b10000: if(next_bit) state <= 5'b10001;  // bit 0
		5'b10001: if(next_bit) state <= 5'b10010;  // bit 1
		5'b10010: if(next_bit) state <= 5'b10011;  // bit 2
		5'b10011: if(next_bit) state <= 5'b10100;  // bit 3
		5'b10100: if(next_bit) state <= 5'b10101;  // bit 4
		5'b10101: if(next_bit) state <= 5'b10110;  // bit 5
		5'b10110: if(next_bit) state <= 5'b10111;  // bit 6
		5'b10111: if(next_bit) state <= 5'b11000;  // bit 7
		5'b11000: if(next_bit) state <= 5'b11001;  // bit 0
		5'b11001: if(next_bit) state <= 5'b11010;  // bit 1
		5'b11010: if(next_bit) state <= 5'b11011;  // bit 2
		5'b11011: if(next_bit) state <= 5'b11100;  // bit 3
		5'b11100: if(next_bit) state <= 5'b11101;  // bit 4
		5'b11101: if(next_bit) state <= 5'b11110;  // bit 5
		5'b11110: if(next_bit) state <= 5'b11111;  // bit 6
		5'b11111: if(next_bit) state <= 5'b00001;  // bit 7
		5'b00001: if(next_bit) state <= 5'b00000;  // stop bit
		default: state <= 5'b00000;
	endcase
	end
end

reg [15:0] RxD_data;
always @(posedge clk)
begin
   if(clk_equ)
	begin
		if(next_bit & state[4]) RxD_data <= {RxD_bit_inv, RxD_data[15:1]};
	end
end

reg RxD_data_ready;
always @(posedge clk)
begin
   if(clk_equ)
	begin
		RxD_data_ready <= (next_bit && state==5'b00001 && RxD_bit_inv);  // ready only if the stop bit is received
	end
end

endmodule