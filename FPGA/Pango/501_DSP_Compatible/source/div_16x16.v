// Cycle-compatible replacement for the Xilinx Divider Generator 3.0 cores.
// The radix-2 datapath calculates two quotient bits per clock, accepting a
// new input every eight clocks. Output pipeline lengths reproduce the original
// signed 21-cycle and unsigned 19-cycle configurations.
module div_16x16_compatible #(
  parameter IS_SIGNED = 0,
  parameter POST_DELAY = 10,
  parameter [2:0] RFD_PHASE = 3'd7
)(
  output reg        rfd,
  input             clk,
  input      [15:0] dividend,
  output reg [15:0] quotient,
  input      [15:0] divisor,
  output reg [15:0] fractional
);

reg [2:0] cadence;
reg       busy;
reg [2:0] iteration;
reg [15:0] dividend_shift;
reg [15:0] divisor_reg;
reg [15:0] quotient_work;
reg [16:0] remainder_work;
reg        quotient_negative;
reg        remainder_negative;

reg [15:0] quotient_delay [0:POST_DELAY-1];
reg [15:0] remainder_delay [0:POST_DELAY-1];
integer i;

wire [15:0] dividend_magnitude =
    ((IS_SIGNED != 0) && dividend[15]) ? (~dividend + 16'd1) : dividend;
wire [15:0] divisor_magnitude =
    ((IS_SIGNED != 0) && divisor[15]) ? (~divisor + 16'd1) : divisor;

wire [16:0] divisor_ext = {1'b0, divisor_reg};

wire [16:0] remainder_shift_1 =
    {remainder_work[15:0], dividend_shift[15]};
wire quotient_bit_1 = (remainder_shift_1 >= divisor_ext);
wire [16:0] remainder_next_1 = quotient_bit_1
    ? (remainder_shift_1 - divisor_ext) : remainder_shift_1;

wire [16:0] remainder_shift_2 =
    {remainder_next_1[15:0], dividend_shift[14]};
wire quotient_bit_2 = (remainder_shift_2 >= divisor_ext);
wire [16:0] remainder_next_2 = quotient_bit_2
    ? (remainder_shift_2 - divisor_ext) : remainder_shift_2;
wire [15:0] quotient_next_2 =
    {quotient_work[13:0], quotient_bit_1, quotient_bit_2};

wire [15:0] completed_quotient = quotient_negative
    ? (~quotient_next_2 + 16'd1) : quotient_next_2;
wire [15:0] completed_remainder = remainder_negative
    ? (~remainder_next_2[15:0] + 16'd1) : remainder_next_2[15:0];

initial begin
  rfd = 1'b0;
  quotient = 16'd0;
  fractional = 16'd0;
  cadence = 3'd0;
  busy = 1'b0;
  iteration = 3'd0;
  dividend_shift = 16'd0;
  divisor_reg = 16'd0;
  quotient_work = 16'd0;
  remainder_work = 17'd0;
  quotient_negative = 1'b0;
  remainder_negative = 1'b0;
  for (i = 0; i < POST_DELAY; i = i + 1) begin
    quotient_delay[i] = 16'd0;
    remainder_delay[i] = 16'd0;
  end
end

always @(posedge clk) begin
  cadence <= cadence + 3'd1;
  rfd <= (cadence == RFD_PHASE);

  quotient <= quotient_delay[POST_DELAY-1];
  fractional <= remainder_delay[POST_DELAY-1];
  for (i = POST_DELAY-1; i > 0; i = i - 1) begin
    quotient_delay[i] <= quotient_delay[i-1];
    remainder_delay[i] <= remainder_delay[i-1];
  end

  if (busy) begin
    dividend_shift <= {dividend_shift[13:0], 2'b00};
    quotient_work <= quotient_next_2;
    remainder_work <= remainder_next_2;

    if (iteration == 3'd7) begin
      quotient_delay[0] <= completed_quotient;
      remainder_delay[0] <= completed_remainder;
      busy <= 1'b0;
    end else begin
      iteration <= iteration + 3'd1;
    end
  end

  // rfd is the Xilinx "ready for data" pulse. The input is captured on
  // the rising clock edge for which the previously registered rfd is high.
  if (rfd) begin
    dividend_shift <= dividend_magnitude;
    divisor_reg <= divisor_magnitude;
    quotient_work <= 16'd0;
    remainder_work <= 17'd0;
    quotient_negative <= (IS_SIGNED != 0) &&
                         (dividend[15] ^ divisor[15]);
    remainder_negative <= (IS_SIGNED != 0) && dividend[15];
    iteration <= 3'd0;
    busy <= 1'b1;
  end
end

endmodule

module div_16X16 (
  output        rfd,
  input         clk,
  input  [15:0] dividend,
  output [15:0] quotient,
  input  [15:0] divisor,
  output [15:0] fractional
);
  div_16x16_compatible #(
    .IS_SIGNED (1),
    .POST_DELAY(12),
    .RFD_PHASE (3'd6)
  ) u_div_16x16_signed (
    .rfd(rfd), .clk(clk), .dividend(dividend), .quotient(quotient),
    .divisor(divisor), .fractional(fractional)
  );
endmodule

module unsigned_div_16X16 (
  output        rfd,
  input         clk,
  input  [15:0] dividend,
  output [15:0] quotient,
  input      [15:0] divisor,
  output [15:0] fractional
);
  div_16x16_compatible #(
    .IS_SIGNED (0),
    .POST_DELAY(10),
    .RFD_PHASE (3'd7)
  ) u_div_16x16_unsigned (
    .rfd(rfd), .clk(clk), .dividend(dividend), .quotient(quotient),
    .divisor(divisor), .fractional(fractional)
  );
endmodule
