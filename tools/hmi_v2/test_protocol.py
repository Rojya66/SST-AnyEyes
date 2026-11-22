"""Loopback checks with an ARM-style Modbus TCP DPRAM server."""

from pathlib import Path
import socketserver
import struct
import tempfile
import threading
import unittest

from panel import HistoryJournal
from protocol import ModbusTcp, Panel, ProtocolError, SIGNATURE, load_points


class FakeArm(socketserver.StreamRequestHandler):
    def handle(self):
        while header := self.rfile.read(7):
            sequence, _, length, unit = struct.unpack(">HHHB", header)
            request = self.rfile.read(length - 1)
            reply = self.handle_request(request)
            self.wfile.write(struct.pack(">HHHB", sequence, 0, len(reply) + 1, unit) + reply)

    def handle_request(self, pdu):
        function, address, value = struct.unpack(">BHH", pdu[:5])
        writes = self.server.writes
        memory = self.server.memory
        if function == 3:
            if address == 0x050C and self.server.advance_live:
                memory[0x050C] += 3
            if address == 0x0507 and self.server.storage_wait:
                self.server.storage_wait -= 1
                if self.server.storage_wait == 0:
                    self.complete_parameter()
            words = memory[address:address + value]
            return struct.pack(f">BB{value}H", function, value * 2, *words)
        if function == 0x10:
            assert pdu[5] == value * 2
            words = struct.unpack(f">{value}H", pdu[6:])
            memory[address:address + value] = words
            writes.append((function, address, words))
            return pdu[:5]
        if function == 6:
            memory[address] = value
            writes.append((function, address, value))
            if address == 0x1FFE:
                if value == 1 and not self.server.drop_control_receipt:
                    memory[0x0504] = (memory[0x0504] + 1) & 0xFFFF
                    memory[0x0505] = memory[0x0100]
                    memory[0x0506] = self.server.control_result
                elif value == 2:
                    if memory[0x01FF] == 2:
                        memory[0x050B] = 1
                        self.server.storage_wait = 4
                    else:
                        self.complete_parameter()
            return pdu
        raise AssertionError("unexpected test function")

    def complete_parameter(self):
        memory = self.server.memory
        operation = memory[0x01FF]
        result = self.server.parameter_result
        if operation == 1 and result == 0:
            self.server.active = memory[0x0200:0x023D]
        memory[0x0200:0x023D] = self.server.active
        memory[0x0508:0x050C] = (operation, result, 0x0201 if result else 0xFFFF, 0)
        memory[0x0507] = (memory[0x0507] + 1) & 0xFFFF


class PanelTest(unittest.TestCase):
    def setUp(self):
        self.server = socketserver.TCPServer(("127.0.0.1", 0), FakeArm)
        self.server.memory = [0] * 0x2000
        self.server.memory[0x0500:0x0502] = SIGNATURE
        self.server.active = list(range(61))
        self.server.writes = []
        self.server.storage_wait = 0
        self.server.advance_live = False
        self.server.drop_control_receipt = False
        self.server.control_result = 0
        self.server.parameter_result = 0
        self.thread = threading.Thread(target=self.server.serve_forever, kwargs={"poll_interval": 0.02})
        self.thread.start()
        self.panel = Panel(ModbusTcp("127.0.0.1", self.server.server_address[1]), 1)

    def tearDown(self):
        self.panel.close()
        self.server.shutdown()
        self.server.server_close()
        self.thread.join()

    def test_startup_requires_advancing_dsp_uptime(self):
        with self.assertRaisesRegex(ProtocolError, "实时节拍未更新"):
            self.panel.wait_running(0.08)
        self.server.advance_live = True
        self.panel.wait_running(0.2)
        self.assertEqual([], self.server.writes)

    def test_control_uses_one_mailbox_commit_and_accepts_count_wrap(self):
        self.server.memory[0x0504] = 0xFFFF
        self.server.control_result = 5
        receipt = self.panel.control(13, (0x100, 0, 0))
        self.assertEqual((receipt.count, receipt.operation, receipt.result), (0, 13, 5))
        self.assertEqual(self.server.writes, [(0x10, 0x0100, (13, 0x100, 0, 0)), (6, 0x1FFE, 1)])

    def test_control_receipt_timeout_does_not_retransmit(self):
        self.server.drop_control_receipt = True
        self.panel.command_timeout = 0.06
        with self.assertRaisesRegex(ProtocolError, "回执超时"):
            self.panel.control(5)
        self.assertEqual(self.server.writes, [(0x10, 0x0100, (5, 0, 0, 0)), (6, 0x1FFE, 1)])

    def test_version_mismatch_writes_nothing(self):
        self.server.memory[0x0501] = 0x0102
        with self.assertRaisesRegex(ProtocolError, "接口签名或版本"):
            self.panel.control(1)
        self.assertEqual([], self.server.writes)

    def test_parameters_apply_reject_and_save_wait_for_complete(self):
        receipt, active = self.panel.parameter(4)
        self.assertEqual(receipt.result, 0)
        self.assertEqual(tuple(active.values()), tuple(range(61)))
        new_values = tuple(range(60, -1, -1))
        receipt, active = self.panel.parameter(1, new_values)
        self.assertEqual(receipt.result, 0)
        self.assertEqual(tuple(active.values()), new_values)
        self.assertEqual(self.server.writes[-2:], [(0x10, 0x01FF, (1, *new_values)), (6, 0x1FFE, 2)])
        self.server.parameter_result = 3
        receipt, actual = self.panel.parameter(1, (0,) * 61)
        self.assertEqual((receipt.result, receipt.error_address), (3, 0x0201))
        self.assertEqual(actual, active)
        self.server.parameter_result = 0
        receipt, actual = self.panel.parameter(2)
        self.assertEqual((receipt.operation, receipt.result, self.server.storage_wait), (2, 0, 0))
        self.assertEqual(actual, active)
        self.assertEqual(self.server.writes[-2:], [(6, 0x01FF, 2), (6, 0x1FFE, 2)])

    def test_live_modules_and_history_use_actual_points(self):
        points = load_points()
        mem = self.server.memory
        mem[0x0510] = 5
        mem[0x0540:0x0542] = (0, 0x3F80)
        mem[0x0598] = mem[0x059B] = 0x0100
        mem[0x0620:0x0624] = (0x0280, 0xFFF3, 0x0019, 0x015D)
        mem[0x0701] = 8
        mem[0x0702] = 39103
        mem[0x0800:0x0802] = (2, 32)
        for n in range(32):
            address = 0x0810 + n * 18
            mem[address] = 39103
            mem[address + 13] = 32 - n
            mem[address + 15] = 0x100
        live = self.panel.live()
        self.assertEqual(points["CHB_STATE"].decode(live), 5)
        self.assertEqual(points["U_RMS_A"].decode(live), 1)
        self.assertEqual(points["fault_first_code"].decode(live), 39103)
        module = self.panel.modules(0)
        self.assertEqual(points["OBS_VALID_A"].decode(module), 0x100)
        self.assertEqual(points["A09_IR"].format(module), "-1.3")
        self.assertEqual(points["A09_TEMP"].format(module), "34.9")
        epoch, history = self.panel.history(None)
        self.assertEqual((epoch, len(history), history[0][15], history[-1][13]), (2, 32, 0x100, 1))
        self.assertEqual(self.panel.history(epoch), (2, None))
        self.assertTrue(self.server.writes == [])
        with tempfile.TemporaryDirectory() as folder:
            archive = Path(folder, "history.csv")
            journal = HistoryJournal(archive)
            journal.update_uptime(500)
            journal.append(history)
            journal.append(history)
            self.assertEqual(len(archive.read_text(encoding="utf-8").splitlines()), 33)
            self.assertTrue(journal.update_uptime(8))
            journal.append(history[:1])
            self.assertEqual(len(archive.read_text(encoding="utf-8").splitlines()), 34)


if __name__ == "__main__":
    unittest.main()
