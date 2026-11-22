`timescale 1ns/1ps

// Exercise the real UART receiver and EMIFA registers. The simulation PLL
// passes through the test's 100 MHz clock, and the PWM multiplier is modeled.
// Run from the project root in PowerShell:
// $sim = Join-Path $env:TEMP 'pango501-udc-watchdog.vvp'
// $rtl = @(Get-ChildItem source -Filter '*.v' | ForEach-Object FullName)
// iverilog -g2012 -s tb_unit_udc_watchdog -o $sim @rtl tests/tb_unit_udc_watchdog.sv
// vvp $sim
module tb_unit_udc_watchdog;
    localparam integer TIMEOUT_CYCLES = 2200;
    localparam integer UART_BIT_CYCLES = 32;
    localparam integer BK_FILTER_CYCLES = 4;

    reg clk = 0;
    always #5 clk = ~clk;

    reg [7:0] xa = 0;
    wire [15:0] xd;
    reg [15:0] write_data = 0;
    reg drive_xd = 0;
    reg we_n = 1;
    reg rd_n = 1;
    reg cs_n = 1;
    reg bk = 0;
    reg [7:0] rx = 8'hff;
    assign xd = drive_xd ? write_data : 16'hzzzz;

    main501 #(
        .BK_FILTER(BK_FILTER_CYCLES),
        .UNIT_UDC_TIMEOUT_CYCLES(TIMEOUT_CYCLES)
    ) dut (
        .CLK20M_IN(clk),
        .XA(xa), .XD(xd), .WE(we_n), .RD(rd_n), .CS(cs_n),
        .DIR(), .BK(bk), .FB(), .LED(),
        .IO_A(1'b0), .IO_B(1'b0), .IO_C(1'b0), .GPIO(3'b000),
        .RX(rx), .TX(), .SYNC_IN(1'b0), .FPGA_LED()
    );

    task automatic emifa_write(input [7:0] address, input [15:0] value);
        @(negedge clk);
        xa = address;
        write_data = value;
        drive_xd = 1;
        cs_n = 0;
        we_n = 0;
        repeat (5) @(negedge clk);
        cs_n = 1;
        we_n = 1;
        drive_xd = 0;
        repeat (5) @(negedge clk);
    endtask

    task automatic expect_register(input [7:0] address, input [15:0] expected);
        @(negedge clk);
        xa = address;
        cs_n = 0;
        rd_n = 0;
        repeat (5) @(negedge clk);
        if (xd !== expected)
            $fatal(1, "EMIFA %h read %h, expected %h at %0t", address, xd, expected, $time);
        cs_n = 1;
        rd_n = 1;
        repeat (5) @(negedge clk);
    endtask

    task automatic uart_word(input integer lane, input [15:0] word, input bit stop_bit);
        integer bit_index;
        @(negedge clk);
        rx[lane] = 0;
        repeat (UART_BIT_CYCLES) @(negedge clk);
        for (bit_index = 0; bit_index < 16; bit_index = bit_index + 1) begin
            rx[lane] = word[bit_index];
            repeat (UART_BIT_CYCLES) @(negedge clk);
        end
        rx[lane] = stop_bit;
        repeat (UART_BIT_CYCLES) @(negedge clk);
        rx[lane] = 1;
    endtask

    initial begin
        expect_register(8'hf1, 16'h0000);
        expect_register(8'hf2, 16'h0000);
        uart_word(0, 16'h0123, 1);
        expect_register(8'hf1, 16'h0000);

        emifa_write(8'h03, 16'h0003);
        repeat (TIMEOUT_CYCLES + 10) @(negedge clk);
        expect_register(8'hf2, 16'h0000);
        uart_word(0, 16'h0123, 0);
        repeat (600) @(negedge clk);
        uart_word(0, 16'h2345, 1);
        expect_register(8'hf1, 16'h0000);

        uart_word(0, 16'h0123, 1);
        expect_register(8'hf1, 16'h0101);
        expect_register(8'h80, 16'h0123);
        uart_word(1, 16'h0456, 1);
        expect_register(8'hf1, 16'h0303);
        repeat (900) @(negedge clk);
        uart_word(0, 16'h0123, 1);
        uart_word(1, 16'h2345, 1);
        repeat (230) @(negedge clk);
        expect_register(8'hf1, 16'h0101);
        expect_register(8'hf2, 16'h0002);
        expect_register(8'hf2, 16'h0002);
        emifa_write(8'hf2, 16'h0001);
        expect_register(8'hf2, 16'h0002);
        uart_word(1, 16'h0789, 1);
        expect_register(8'hf1, 16'h0303);
        expect_register(8'hf2, 16'h0002);
        emifa_write(8'hf2, 16'h0002);
        expect_register(8'hf2, 16'h0000);

        emifa_write(8'h03, 16'h0001);
        bk = 1;
        repeat (BK_FILTER_CYCLES + 5) @(negedge clk);
        repeat (5) uart_word(0, 16'h0678, 1);
        expect_register(8'hf1, 16'h0001);
        expect_register(8'h80, 16'h0123);
        expect_register(8'hf2, 16'h0000);
        bk = 0;
        repeat (BK_FILTER_CYCLES + 5) @(negedge clk);
        expect_register(8'hf1, 16'h0001);
        expect_register(8'h80, 16'h0678);
        uart_word(0, 16'h0789, 1);
        expect_register(8'hf1, 16'h0101);
        expect_register(8'h80, 16'h0789);

        fork
            begin
                wait (dut.unit_rx[0].udc_timer == 3);
                emifa_write(8'hf2, 16'h0001);
            end
            begin
                wait (dut.clear_unit_timeout);
                @(posedge clk);
                if (!dut.unit_rx[0].udc_expired)
                    $fatal(1, "Expected timeout and F2 clear on the same clock");
            end
        join
        expect_register(8'hf1, 16'h0000);
        expect_register(8'hf2, 16'h0001);
        uart_word(0, 16'h0123, 1);
        expect_register(8'hf1, 16'h0101);
        expect_register(8'hf2, 16'h0001);
        emifa_write(8'h03, 16'h0000);
        expect_register(8'hf1, 16'h0000);
        expect_register(8'hf2, 16'h0000);

        emifa_write(8'h03, 16'h0001);
        uart_word(0, 16'h0789, 1);
        repeat (TIMEOUT_CYCLES + 10) @(negedge clk);
        expect_register(8'hf2, 16'h0001);
        uart_word(0, 16'h0123, 1);
        expect_register(8'hf1, 16'h0101);
        emifa_write(8'h01, 16'h0538);
        expect_register(8'hf1, 16'h0000);
        expect_register(8'hf2, 16'h0000);
        expect_register(8'h80, 16'h0000);
        emifa_write(8'h01, 16'h0000);
        uart_word(0, 16'h0123, 1);
        expect_register(8'hf1, 16'h0101);
        $display("PASS: UNIT Udc online, sample validity, and sticky timeout over UART and EMIFA");
        $finish;
    end

    initial #1000000 $fatal(1, "Test timed out");
endmodule

module clock(output wire clkout0, input wire clkin1, output wire lock);
    assign clkout0 = clkin1;
    assign lock = 1'b1;
endmodule

module mul_16X16(
    input wire ce, rst, clk,
    input wire [15:0] a, b,
    output reg [31:0] p
);
    always @(posedge clk or posedge rst)
        if (rst)
            p <= 0;
        else if (ce)
            p <= a * b;
endmodule
