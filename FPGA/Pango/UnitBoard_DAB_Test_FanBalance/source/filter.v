`timescale 1ns / 1ps

// Four-channel, 32-tap signed Q18 FIR.
// Symmetric tap pairs are multiplied once, then registered. A balanced
// adder tree produces one result per clock without the old ripple chain.
module filter(
    output             rfd,
    output reg         rdy,
    input              clk,
    output reg [34:0]  dout,
    input      [15:0]  din,
    output     [1:0]   chan_in,
    output reg [1:0]   chan_out
);

localparam integer PRODUCT_BITS = 33;
localparam integer PRODUCT_COUNT = 16;
localparam integer PRODUCT_BUS_BITS = PRODUCT_BITS * PRODUCT_COUNT;

reg [1:0] chan = 2'd0;
reg signed [15:0] delay0 [0:30];
reg signed [15:0] delay1 [0:30];
reg signed [15:0] delay2 [0:30];
reg signed [15:0] delay3 [0:30];
reg [PRODUCT_BUS_BITS-1:0] product_comb;
reg [PRODUCT_BUS_BITS-1:0] product_pipe;
reg [1:0] chan_pipe;
reg product_valid;
integer i;
integer k;

assign rfd = 1'b1;
assign chan_in = chan;

function signed [15:0] coefficient;
    input [4:0] tap;
begin
    case (tap)
        5'd0:  coefficient = -16'sd5064;
        5'd1:  coefficient = -16'sd5456;
        5'd2:  coefficient = -16'sd5325;
        5'd3:  coefficient = -16'sd4595;
        5'd4:  coefficient = -16'sd3226;
        5'd5:  coefficient = -16'sd1224;
        5'd6:  coefficient =  16'sd1360;
        5'd7:  coefficient =  16'sd4433;
        5'd8:  coefficient =  16'sd7856;
        5'd9:  coefficient =  16'sd11463;
        5'd10: coefficient =  16'sd15063;
        5'd11: coefficient =  16'sd18457;
        5'd12: coefficient =  16'sd21451;
        5'd13: coefficient =  16'sd23869;
        5'd14: coefficient =  16'sd25567;
        5'd15: coefficient =  16'sd26443;
        default: coefficient = 16'sd0;
    endcase
end
endfunction

function signed [32:0] pair_product;
    input signed [16:0] pair_value;
    input [4:0] tap;
    reg signed [32:0] product;
begin
    product = pair_value * coefficient(tap);
    pair_product = product;
end
endfunction

// Pack 16 symmetric products. The 16 pairs are exactly equivalent to the
// original 32 products because c[k] == c[31-k].
function [PRODUCT_BUS_BITS-1:0] fir_products;
    input signed [15:0] x0;
    input signed [15:0] x1;
    input signed [15:0] x2;
    input signed [15:0] x3;
    input signed [15:0] x4;
    input signed [15:0] x5;
    input signed [15:0] x6;
    input signed [15:0] x7;
    input signed [15:0] x8;
    input signed [15:0] x9;
    input signed [15:0] x10;
    input signed [15:0] x11;
    input signed [15:0] x12;
    input signed [15:0] x13;
    input signed [15:0] x14;
    input signed [15:0] x15;
    input signed [15:0] x16;
    input signed [15:0] x17;
    input signed [15:0] x18;
    input signed [15:0] x19;
    input signed [15:0] x20;
    input signed [15:0] x21;
    input signed [15:0] x22;
    input signed [15:0] x23;
    input signed [15:0] x24;
    input signed [15:0] x25;
    input signed [15:0] x26;
    input signed [15:0] x27;
    input signed [15:0] x28;
    input signed [15:0] x29;
    input signed [15:0] x30;
    input signed [15:0] x31;
    reg signed [15:0] sample [0:31];
    reg signed [16:0] pair_value;
begin
    sample[0]=x0; sample[1]=x1; sample[2]=x2; sample[3]=x3;
    sample[4]=x4; sample[5]=x5; sample[6]=x6; sample[7]=x7;
    sample[8]=x8; sample[9]=x9; sample[10]=x10; sample[11]=x11;
    sample[12]=x12; sample[13]=x13; sample[14]=x14; sample[15]=x15;
    sample[16]=x16; sample[17]=x17; sample[18]=x18; sample[19]=x19;
    sample[20]=x20; sample[21]=x21; sample[22]=x22; sample[23]=x23;
    sample[24]=x24; sample[25]=x25; sample[26]=x26; sample[27]=x27;
    sample[28]=x28; sample[29]=x29; sample[30]=x30; sample[31]=x31;
    for (k = 0; k < PRODUCT_COUNT; k = k + 1) begin
        if (k == 0)
            pair_value = $signed({sample[0][15],sample[0]}) + $signed({sample[31][15],sample[31]});
        else
            pair_value = $signed({sample[k][15],sample[k]}) + $signed({sample[31-k][15],sample[31-k]});
        fir_products[k*PRODUCT_BITS +: PRODUCT_BITS] = pair_product(pair_value, k);
    end
end
endfunction

function signed [34:0] sum_products;
    input [PRODUCT_BUS_BITS-1:0] product_bus;
    reg signed [32:0] p [0:15];
    reg signed [34:0] level1 [0:7];
    reg signed [34:0] level2 [0:3];
    reg signed [34:0] level3 [0:1];
    integer n;
begin
    for (n = 0; n < PRODUCT_COUNT; n = n + 1)
        p[n] = product_bus[n*PRODUCT_BITS +: PRODUCT_BITS];
    for (n = 0; n < 8; n = n + 1)
        level1[n] = {{2{p[2*n][32]}},p[2*n]} + {{2{p[2*n+1][32]}},p[2*n+1]};
    for (n = 0; n < 4; n = n + 1)
        level2[n] = level1[2*n] + level1[2*n+1];
    for (n = 0; n < 2; n = n + 1)
        level3[n] = level2[2*n] + level2[2*n+1];
    sum_products = level3[0] + level3[1];
end
endfunction

// Select the active channel's history before the product register.
always @* begin
    product_comb = {PRODUCT_BUS_BITS{1'b0}};
    case (chan)
        2'd0: product_comb = fir_products(din,
            delay0[0],delay0[1],delay0[2],delay0[3],delay0[4],delay0[5],
            delay0[6],delay0[7],delay0[8],delay0[9],delay0[10],delay0[11],
            delay0[12],delay0[13],delay0[14],delay0[15],delay0[16],delay0[17],
            delay0[18],delay0[19],delay0[20],delay0[21],delay0[22],delay0[23],
            delay0[24],delay0[25],delay0[26],delay0[27],delay0[28],delay0[29],delay0[30]);
        2'd1: product_comb = fir_products(din,
            delay1[0],delay1[1],delay1[2],delay1[3],delay1[4],delay1[5],
            delay1[6],delay1[7],delay1[8],delay1[9],delay1[10],delay1[11],
            delay1[12],delay1[13],delay1[14],delay1[15],delay1[16],delay1[17],
            delay1[18],delay1[19],delay1[20],delay1[21],delay1[22],delay1[23],
            delay1[24],delay1[25],delay1[26],delay1[27],delay1[28],delay1[29],delay1[30]);
        2'd2: product_comb = fir_products(din,
            delay2[0],delay2[1],delay2[2],delay2[3],delay2[4],delay2[5],
            delay2[6],delay2[7],delay2[8],delay2[9],delay2[10],delay2[11],
            delay2[12],delay2[13],delay2[14],delay2[15],delay2[16],delay2[17],
            delay2[18],delay2[19],delay2[20],delay2[21],delay2[22],delay2[23],
            delay2[24],delay2[25],delay2[26],delay2[27],delay2[28],delay2[29],delay2[30]);
        default: product_comb = fir_products(din,
            delay3[0],delay3[1],delay3[2],delay3[3],delay3[4],delay3[5],
            delay3[6],delay3[7],delay3[8],delay3[9],delay3[10],delay3[11],
            delay3[12],delay3[13],delay3[14],delay3[15],delay3[16],delay3[17],
            delay3[18],delay3[19],delay3[20],delay3[21],delay3[22],delay3[23],
            delay3[24],delay3[25],delay3[26],delay3[27],delay3[28],delay3[29],delay3[30]);
    endcase
end

initial begin
    rdy = 1'b0;
    dout = 35'd0;
    chan_out = 2'd0;
    chan_pipe = 2'd0;
    product_pipe = {PRODUCT_BUS_BITS{1'b0}};
    product_valid = 1'b0;
    for (i = 0; i < 31; i = i + 1) begin
        delay0[i] = 16'sd0;
        delay1[i] = 16'sd0;
        delay2[i] = 16'sd0;
        delay3[i] = 16'sd0;
    end
end

always @(posedge clk) begin
    rdy <= product_valid;
    dout <= sum_products(product_pipe);
    chan_out <= chan_pipe;
    product_pipe <= product_comb;
    chan_pipe <= chan;
    product_valid <= 1'b1;

    case (chan)
        2'd0: begin
            for (i = 30; i > 0; i = i - 1) delay0[i] <= delay0[i-1];
            delay0[0] <= din;
        end
        2'd1: begin
            for (i = 30; i > 0; i = i - 1) delay1[i] <= delay1[i-1];
            delay1[0] <= din;
        end
        2'd2: begin
            for (i = 30; i > 0; i = i - 1) delay2[i] <= delay2[i-1];
            delay2[0] <= din;
        end
        default: begin
            for (i = 30; i > 0; i = i - 1) delay3[i] <= delay3[i-1];
            delay3[0] <= din;
        end
    endcase
    chan <= chan + 2'd1;
end
endmodule
