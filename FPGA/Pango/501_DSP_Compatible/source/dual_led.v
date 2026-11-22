//LED module
module dual_led(
	input clk,
	input rst_n, //reset key action
	input [3:0] op_state,
	output reg [1:0] led
);
localparam STATE_RUN = 4'b0001;
localparam STATE_STOP = 4'b0000;
localparam STATE_SYNC = 4'b0010;
localparam STATE_PROTECT = 4'b0011;
localparam [31:0] CLK_FREQ_HZ = 32'd100_000_000;
localparam [31:0] RUN_HALF_PERIOD = CLK_FREQ_HZ / 2;
localparam [31:0] FAST_HALF_PERIOD = CLK_FREQ_HZ / 4;
reg [31:0] cnt_led;

always @(posedge clk or negedge rst_n)
begin
    if(!rst_n)
    begin
        led <= 2'b11;
        cnt_led <= 32'd0;
    end
    else 
    begin
        case(op_state)
            STATE_STOP:begin
                cnt_led <= 32'd0;
                led <= 2'b00;
            end
            
            STATE_RUN:begin
                led[0] <= (cnt_led < RUN_HALF_PERIOD) ? 1'b1 : 1'b0;
                led[1] <= ~led[0];
                if(cnt_led == (2 * RUN_HALF_PERIOD - 1'b1))
                begin
                    cnt_led <= 32'd0;
                end
                else begin
                    cnt_led <= cnt_led + 1'b1;
                end
            end
            
            STATE_PROTECT:begin
                led <= (cnt_led < FAST_HALF_PERIOD) ? 2'b11 : 2'b00;
                if(cnt_led == (2 * FAST_HALF_PERIOD - 1'b1))
                begin
                    cnt_led <= 32'd0;
                end
                else begin
                    cnt_led <= cnt_led + 1'b1;
                end
            end

            STATE_SYNC:begin
                led[0] <= (cnt_led < FAST_HALF_PERIOD) ? 1'b1 : 1'b0;
                led[1] <= ~led[0];
                if(cnt_led == (2 * FAST_HALF_PERIOD - 1'b1))
                begin
                    cnt_led <= 32'd0;
                end
                else begin
                    cnt_led <= cnt_led + 1'b1;
                end
            end
        endcase
    end    
end
endmodule
