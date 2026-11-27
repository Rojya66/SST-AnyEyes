`timescale 1ns/1ps

// Run with python tests/run_fan_threshold_test.py and Icarus on PATH.
module tb_fan_thresholds;
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
    assign xd = drive_xd ? write_data : 16'hzzzz;

    wire [7:0] tx_501;
    wire unit_tx;
    reg down_connected = 1;
    reg up_connected = 1;
    reg omit_off_lo = 0;
    wire unit_rx = down_connected && !(omit_off_lo && dut.TX0_buf[15:8] == 8'hC3)
                   ? tx_501[0] : 1'b1;
    wire [7:0] rx_501 = {7'h7F, up_connected ? unit_tx : 1'b1};

    reg [15:0] adc = 16'hFFFF;
    reg ov = 0;
    string scenario = "initial measurements";
    wire fan_0, fan_1;
    main501 #(.BK_FILTER(4), .UNIT_UDC_TIMEOUT_CYCLES(20000)) dut (
        .CLK20M_IN(clk), .XA(xa), .XD(xd), .WE(we_n), .RD(rd_n), .CS(cs_n),
        .BK(bk), .IO_A(1'b0), .IO_B(1'b0), .IO_C(1'b0), .GPIO(3'b000),
        .RX(rx_501), .TX(tx_501), .SYNC_IN(1'b0)
    );
    main #(.DI_FILTER(4), .PROT_FILTER(4), .FO_FILTER(4), .OP_FILTER(4)) unit (
        .CLK(clk), .AD_DATA(adc), .OP_RX(unit_rx), .OP_TX(unit_tx),
        .FP_OUT0(fan_0), .FP_OUT1(fan_1), .FPI_TEMP(1'b0),
        .PROT_HI(ov), .PROT_LO(1'b0), .PROT_Ig(1'b0), .PROT_Ir(1'b0),
        .L_SO(4'd0), .H_SO(4'd0), .SYNC_IN(1'b0), .SHUTDOWN(1'b0)
    );

    initial begin
        // Model the FPGA power-up value of the legacy UNIT receiver counter.
        force unit.recv.cnt = 16'd0;
        #1 release unit.recv.cnt;
    end

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

    task automatic configure(input [7:0] generation, input [15:0] off_value, on_value);
        emifa_write(8'h2A, off_value);
        emifa_write(8'h2B, on_value);
        emifa_write(8'h2C, {8'h80, generation});
    endtask

    task automatic expect_fan(input bit expected);
        repeat (12) @(negedge clk);
        if ({fan_0, fan_1} !== {2{expected}})
            $fatal(1, "Fans %b%b, expected both %b at %0t", fan_0, fan_1, expected, $time);
    endtask

    task automatic expect_thresholds(input [15:0] off_value, on_value);
        if (unit.fan_off_udc !== off_value || unit.fan_on_udc !== on_value)
            $fatal(1, "Thresholds %d/%d, expected %d/%d at %0t",
                   unit.fan_off_udc, unit.fan_on_udc, off_value, on_value, $time);
    endtask

    initial begin
        expect_register(8'h2D, 16'h4641);
        emifa_write(8'h03, 16'h0001);
        repeat (12000) @(negedge clk);
        expect_thresholds(170, 235);
        if (unit.SCI_UDC !== 16'hFFFF)
            $fatal(1, "Expected raw negative SCI_UDC to remain unchanged, got %h", unit.SCI_UDC);
        expect_register(8'h80, 16'hFFFF);
        expect_fan(0);
        ov = 1;
        expect_fan(1);
        expect_fan(1);
        ov = 0;
        expect_fan(0);

        scenario = "incomplete request and retry with BK active";
        bk = 1;
        omit_off_lo = 1;
        configure(8'h41, 180, 250);
        repeat (70000) @(negedge clk);
        expect_register(8'h2E, 16'h4100);
        expect_register(8'h2F, 16'h4100);
        expect_thresholds(170, 235);
        expect_register(8'hF1, 16'h0001);
        omit_off_lo = 0;
        wait (dut.fan_ack_mask == 1);
        expect_register(8'h2E, 16'h4101);
        expect_register(8'h2F, 16'h4100);
        expect_thresholds(180, 250);
        wait (dut.fan_tx_slot);
        @(negedge clk);
        if (dut.TX0_buf !== 16'hC000)
            $fatal(1, "Acknowledged lane did not return to C000");

        force unit.SCI_UDC = 16'd250;
        expect_fan(0);
        force unit.SCI_UDC = 16'd251;
        expect_fan(1);
        force unit.SCI_UDC = 16'd180;
        expect_fan(1);
        force unit.SCI_UDC = 16'd179;
        expect_fan(0);
        release unit.SCI_UDC;

        // A response already in flight cannot confirm a recommit of the same generation/values.
        scenario = "stale reply and same-generation recommit";
        configure(8'h42, 185, 260);
        wait (unit.fan_reply_gen == 8'h42 && unit.fan_reply_part == 4'd2);
        emifa_write(8'h2C, 16'h8042);
        down_connected = 0;
        repeat (70000) @(negedge clk);
        expect_register(8'h2E, 16'h4200);
        down_connected = 1;
        wait (dut.fan_ack_mask == 1);
        expect_register(8'h2E, 16'h4201);
        expect_thresholds(185, 260);

        scenario = "UNIT Udc watchdog and PM reset";
        up_connected = 0;
        repeat (21000) @(negedge clk);
        expect_register(8'h2E, 16'h4200);
        // Reconnect during the idle gap after the last frame to avoid joining mid-byte.
        wait (unit.sci_cnt == 16'd4900);
        @(negedge clk);
        up_connected = 1;
        wait (dut.fan_ack_mask == 1);
        expect_register(8'h2E, 16'h4201);
        emifa_write(8'h03, 16'h0000);
        expect_register(8'h2E, 16'h4200);
        emifa_write(8'h03, 16'h0001);

        scenario = "out-of-range reply";
        configure(8'h43, 300, 200);
        wait (dut.fan_reject_mask == 1);
        expect_register(8'h2E, 16'h4300);
        expect_register(8'h2F, 16'h4301);
        expect_thresholds(185, 260);
        scenario = "running rejects configuration, then accepts when stopped";
        force unit.svg_running = 1'b1;
        force unit.FAN_OFF = 1'b1;
        expect_fan(0);
        force unit.FAN_OFF = 1'b0;
        expect_fan(1);
        release unit.FAN_OFF;
        configure(8'h44, 190, 270);
        wait (dut.fan_reject_mask == 1);
        expect_register(8'h2F, 16'h4401);
        expect_thresholds(185, 260);
        release unit.svg_running;
        wait (dut.fan_ack_mask == 1);
        expect_register(8'h2E, 16'h4401);
        expect_register(8'h2F, 16'h4400);
        expect_thresholds(190, 270);
        emifa_write(8'h2C, 16'h0044);
        expect_register(8'h2C, 16'h0044);
        expect_register(8'h30, 16'h0000);
        expect_register(8'h2E, 16'h4400);
        expect_thresholds(190, 270);

        $display("PASS: fan hysteresis, OV, serial configuration, reply validation and freshness");
        $finish;
    end

    initial #10000000 $fatal(1, "Fan threshold test timed out during %s", scenario);

    always @(posedge clk) begin
        if ($test$plusargs("trace") && unit.fan_rx_frame)
            $display("%0t %s UNIT rx %h expects %d crc %h bridges %b%b completed %b",
                     $time, scenario, unit.RxD_data, unit.fan_rx_part, unit.fan_rx_crc,
                     unit.svg_running, unit.dab_running, unit.fan_command_complete);
        if ($test$plusargs("trace") && dut.unit_rx[0].fan_reply_frame)
            $display("%0t %s 501 rx %h expects %d window %b expected crc %h",
                     $time, scenario, dut.unit_rx[0].rx_word, dut.unit_rx[0].fan_ack_part,
                     dut.unit_rx[0].fan_ack_open, dut.fan_active_crc);
    end
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
            p <= $signed(a) * $signed(b);
endmodule
