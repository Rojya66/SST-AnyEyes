`timescale 1ns / 1ps

module div_32X32(
    output reg         rfd,
    input              clk,
    input              start,
    input      [31:0]  dividend,
    output reg [31:0]  quotient,
    input      [31:0]  divisor,
    output reg [31:0]  fractional
);

reg        busy = 1'b0;
reg [5:0]  bit_count = 6'd0;
reg [31:0] dividend_shift = 32'd0;
reg [31:0] divisor_reg = 32'd1;
reg [31:0] quotient_work = 32'd0;
reg [32:0] remainder_work = 33'd0;

wire [32:0] divisor_ext = {1'b0, divisor_reg};
wire [32:0] remainder_shift = {remainder_work[31:0], dividend_shift[31]};
wire        subtract_en = (remainder_shift >= divisor_ext);
wire [32:0] remainder_next = subtract_en ? (remainder_shift - divisor_ext) : remainder_shift;
wire [31:0] quotient_next = {quotient_work[30:0], subtract_en};

initial begin
    rfd = 1'b1;
    quotient = 32'd0;
    fractional = 32'd0;
end

always @(posedge clk) begin
    if (start && !busy) begin
        if (divisor == 32'd0) begin
            quotient <= 32'd0;
            fractional <= 32'd0;
            rfd <= 1'b1;
        end else begin
            busy <= 1'b1;
            rfd <= 1'b0;
            bit_count <= 6'd32;
            dividend_shift <= dividend;
            divisor_reg <= divisor;
            quotient_work <= 32'd0;
            remainder_work <= 33'd0;
        end
    end else if (busy) begin
        dividend_shift <= {dividend_shift[30:0], 1'b0};
        quotient_work <= quotient_next;
        remainder_work <= remainder_next;
        bit_count <= bit_count - 6'd1;

        if (bit_count == 6'd1) begin
            quotient <= quotient_next;
            fractional <= remainder_next[31:0];
            busy <= 1'b0;
            rfd <= 1'b1;
        end
    end
end

endmodule
