"""Generate the modern parallel point list and expand all DPRAM word addresses.

The old 2142 rows are retained. New points reuse the checked AnyEyes wire
encoding, while requests use a dedicated parallel block, never UART frames.
"""
from pathlib import Path
import csv
import json

ROOT = Path(__file__).resolve().parents[1]
DOCS = ROOT / 'docs'

# Original protection timers are 10ms counter limits using a strict > comparison.
LEGACY = [
    ('OC_level_SET', '1', 'A', '1', '65535'),
    ('OC_time_SET', '1', '10ms counter threshold', '0', '65534'),
    ('OC_level_SET_L2', '1', 'A', '1', '65535'),
    ('OC_time_SET_L2', '1', '10ms counter threshold', '0', '65534'),
    ('OV_level_SET', '1', 'V', '1', '65535'),
    ('OV_time_SET', '1', '10ms counter threshold', '0', '65534'),
    ('HA_L_Voltage_Precharge', '10', 'V', '0', '65534'),
    ('HA_H_Voltage', '10', 'V', '1', '65535'),
]

REQUEST = [
    ('magic', '0x5353'), ('version', '1'), ('sequence', '1..65535，最近请求序号'),
    ('operation', '0读取/1写单点/2五字遥控/3保存/4加载'),
    ('point', '0..53，仅op0/1使用'), ('value', '单点编码；op3/4须0xA55A'),
    ('start', 'bit0 CHB+DAB启动'), ('stop', 'bit0 CHB+DAB停止'),
    ('pulse_on', 'bit0 DAB/bit1 CHB'), ('pulse_off', 'bit0 DAB/bit1 CHB'),
    ('fault_reset', 'bit0故障复位'), ('reserved', '必须0'),
    ('crc16', '前12字按低字节先CRC16/IBM initFFFF polyA001 xor0'),
    ('sequence_copy', '必须等于word2'), ('commit', '0xA55A，最后写入'),
    ('reserved', '必须0'),
]
# Fixed protocol layout, kept in step with SstDpramExtension.h.
REQUEST_BASE = 0x66002C00
RESPONSE_BASE = 0x66002D00
TELEMETRY_BASE = 0x66002E00
HISTORY_BASE = 0x66002F00
BLOCK_WORDS = 128
HISTORY_WORDS = 512
REQUEST_WORDS = 16
RESPONSE_PARAMETER_START_WORD = 32
TELEMETRY_ACTIVE_FAULT_START_WORD = 32
HISTORY_RECORD_START_WORD = 16
HISTORY_RECORD_WORDS = 15
HISTORY_RECORD_COUNT = 32
RESPONSE = {
    0: ('publication_version', '奇数未完成；非零偶数完整，前后读取须相等'),
    1: ('magic', '0x5353'), 2: ('version', '1'), 3: ('ack_sequence', '最近请求序号，0表示无请求'),
    4: ('ack_result', '0OK/1地址/2只读/3范围/4忙/5禁止/6存储/7格式CRC序号冲突'),
    5: ('ack_operation', '对应请求操作码'), 6: ('ack_point', '对应请求点号'),
    7: ('actual_value', '对应点当前编码，非法点号为0'),
    8: ('storage_result', 'SST_PARAM_*，保存期间4，完成0，失败6'),
    9: ('storage_pending', '0空闲/1保存中'), 10: ('remote_allowed', '0仿真器/1并口远控'),
    11: ('point_count', '54'),
}
TELEMETRY = {
    0: ('publication_version', '奇数未完成；非零偶数完整，前后读取须相等'),
    1: ('magic', '0x5353'), 2: ('version', '1'),
    3: ('state', '0停机/1预充/2旁路延时/3等PLL/4运行/5故障'),
    4: ('control_fault', '0无/1预充超时/2PLL超时/3快速AC过流/4快速DC过压/5硬件/6非法配置'),
    5: ('startup_phase', '0空闲/1固定Id斜坡/2电压环平滑移交/3正常电压环运行'),
    6: ('flags', 'bit0输入合/1旁路/2CHB PWM/3DAB PWM/4PLL锁定/5电压环/6快保使能/7CHB请求/8DAB请求；1有效'),
    7: ('ac_mode', '0开环/1闭环'), 8: ('first_current_code', '五位故障码，当前表首项，0无故障'),
    9: ('fault_level', '0一级/1二级/2三级/3四级/4无故障'), 10: ('active_count', '0..32'),
    12: ('tick_10ms_low', '32位10ms节拍低16位'), 13: ('tick_10ms_high', '高16位'),
    14: ('startup_tad_low', '32位启动TAD计数低16位'), 15: ('startup_tad_high', '高16位'),
    16: ('dc_average', '0.1V/count'), 17: ('dc_min', '0.1V/count'),
    18: ('pll_angle', '0.0001rad/count uint16'), 19: ('pll_frequency', '0.01Hz/count'),
    20: ('id_feedback', '0.01A/count int16'), 21: ('iq_feedback', '0.01A/count int16'),
    22: ('id_reference', '0.01A/count int16'), 23: ('startup_id_reference', '0.01A/count int16'),
    24: ('startup_voltage_reference', '0.1V/count'), 25: ('modulation_actual', '0.0001pu/count'),
    26: ('dab_duty_count', '实际DAB占空比计数0..2500'),
    27: ('storage_result', 'SST_PARAM_*'), 28: ('storage_pending', '0空闲/1保存中'),
    29: ('history_count', '0..32'),
}
HISTORY_FIELDS = ['code', 'level', 'control_fault', 'port_fault', 'fault_type',
                  'port_latch', 'fb_latch', 'fo_a', 'fo_b', 'fo_c',
                  'module_dispose', 'tick_10ms_low', 'tick_10ms_high',
                  'sequence_low', 'sequence_high']


def write_csv(path, fields, rows):
    with path.open('w', encoding='utf-8-sig', newline='') as stream:
        writer = csv.DictWriter(stream, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def points():
    with (DOCS / 'ANYEYES_PARAMETERS.csv').open(encoding='utf-8-sig') as stream:
        original = list(csv.DictReader(stream))
    commands = [p for p in original if p['group'] == '0']
    params = sorted([p for p in original if p['group'] in ('1', '2')],
                    key=lambda p: (int(p['group']), int(p['index'])))
    assert len(commands) == 8 and len(params) == 37
    enable = dict(group='0', index='8', field='g_chb_dab_protection_enable', type='uint16',
                  scale='1', unit='bool', raw_min='0', raw_max='1', permission='stopped')
    legacy = [dict(group='', index=str(i), field=name, type='uint16', scale=scale,
                   unit=unit, raw_min=low, raw_max=high, permission='stopped')
              for i, (name, scale, unit, low, high) in enumerate(LEGACY)]
    result = []
    for i, p in enumerate(commands + [enable] + params + legacy):
        row = dict(point=i, field=p['field'],
                   read_address=f'0x{RESPONSE_BASE + 2*(RESPONSE_PARAMETER_START_WORD + i):08X}',
                   write_index_address=f'0x{REQUEST_BASE + 2*4:08X}',
                   write_value_address=f'0x{REQUEST_BASE + 2*5:08X}',
                   write_point_value=i, wire_type=p['type'], physical_scale=p['scale'],
                   physical_unit=p['unit'], raw_min=p['raw_min'], raw_max=p['raw_max'],
                   host_permission='R' if p['permission'] == 'read-only' else 'R / request-W',
                   write_condition='只读硬件TAD周期' if p['permission'] == 'read-only' else
                   ('控制源互锁，停/关始终优先；复位完全停机' if i in (0, 1, 3) else '完全停机且存储空闲'),
                   anyeyes_group=p['group'] if i < 46 else '',
                   anyeyes_index=p['index'] if i < 46 else '',
                   persistent='no' if i in (0, 1, 3, 8, 9) else 'v2')
        result.append(row)
    assert len(result) == 54
    return result


def main():
    table = points()
    write_csv(DOCS / 'DPRAM_PARAMETERS.csv', list(table[0]), table)
    table_by_word = {RESPONSE_PARAMETER_START_WORD + row['point']: row for row in table}
    with (DOCS / 'DPRAM_ADDRESSES.csv').open(encoding='utf-8-sig') as stream:
        reader = csv.DictReader(stream)
        fields = reader.fieldnames
        addresses = [row for row in reader if not row['region'].startswith('Sst')]
    assert len(addresses) == 2142
    for region, base, count, permission in [
            ('SstRequest', REQUEST_BASE, BLOCK_WORDS, 'W'),
            ('SstResponse', RESPONSE_BASE, BLOCK_WORDS, 'R'),
            ('SstTelemetry', TELEMETRY_BASE, BLOCK_WORDS, 'R'),
            ('SstHistory', HISTORY_BASE, HISTORY_WORDS, 'R')]:
        for word in range(count):
            name, meaning = 'reserved', '保留，不使用'
            if region == 'SstRequest' and word < 16: name, meaning = REQUEST[word]
            elif region == 'SstResponse':
                if word in RESPONSE: name, meaning = RESPONSE[word]
                elif word in table_by_word:
                    p = table_by_word[word]
                    name, meaning = p['field'], f"physical=raw/{p['physical_scale']} {p['physical_unit']}; point={p['point']}"
            elif region == 'SstTelemetry':
                if word in TELEMETRY: name, meaning = TELEMETRY[word]
                elif TELEMETRY_ACTIVE_FAULT_START_WORD <= word < TELEMETRY_ACTIVE_FAULT_START_WORD + 32:
                    name, meaning = f'active_fault[{word-TELEMETRY_ACTIVE_FAULT_START_WORD}]', '五位当前故障码，未用项0'
            elif region == 'SstHistory':
                header = {0: ('publication_version', '非零偶数完整版本'), 1: ('magic', '0x5353'),
                          2: ('version', '1'), 3: ('history_count', '0..32'), 4: ('record_words', '15'),
                          5: ('latest_sequence_low', '最新事件序号低16位'), 6: ('latest_sequence_high', '高16位')}
                if word in header: name, meaning = header[word]
                elif HISTORY_RECORD_START_WORD <= word < HISTORY_RECORD_START_WORD + HISTORY_RECORD_COUNT * HISTORY_RECORD_WORDS:
                    index, member = divmod(word - HISTORY_RECORD_START_WORD, HISTORY_RECORD_WORDS)
                    name = f'history[{index}].{HISTORY_FIELDS[member]}'
                    meaning = '0最新，至31最旧；仅history_count范围有效；原位图或小端32位低高字'
            address = base + 2 * word
            wire_type = 'int16' if region == 'SstTelemetry' and 20 <= word <= 23 else 'uint16'
            if region == 'SstResponse' and word in table_by_word: wire_type = table_by_word[word]['wire_type']
            addresses.append(dict(region=region, word_index=word, field=name,
                dsp_byte_address=f'0x{address:08X}', dpram_byte_offset=f'0x{address-0x66000000:04X}',
                dpram_word_address=f'0x{(address-0x66000000)//2:04X}', host_permission=permission,
                wire_type=wire_type, numeric_definition=meaning, meaning=meaning,
                implementation_state='保留，不使用' if name == 'reserved' else '已接入并口扩展，版本1',
                evidence='SstDpramMaintenance.c;SstDpramExtension.h;C6748.cmd'))
    # Mailboxes also carry the new block21 identifier.
    for row in addresses:
        if row['field'] == 'MailboxLeft' and '21现代维护' not in row['meaning']: row['meaning'] += '；21现代维护'
        if row['field'] == 'MailboxRight' and '21现代维护' not in row['meaning']: row['meaning'] += '；21现代维护回复（以序号/版本戳确认）'
        if row['field'] == 'MailboxLeft':
            row['implementation_state'] = '单槽通知；读邮箱应答硬件通知；原块1/2无软件执行ACK；块21序号/受理结果见SstResponse区'
        if row['field'] == 'MailboxRight':
            row['implementation_state'] = '单槽通知；原块3..20无软件执行ACK；块21回复发布完成后通知，序号/受理结果见SstResponse区'
    keys = [row['dsp_byte_address'] for row in addresses]
    assert len(keys) == len(set(keys)) == 3038
    write_csv(DOCS / 'DPRAM_ADDRESSES.csv', fields, addresses)

    with (DOCS / 'DPRAM_BITS.csv').open(encoding='utf-8-sig') as stream:
        reader = csv.DictReader(stream)
        bit_fields = reader.fieldnames
        bits = [row for row in reader if row['target'] != '0x66002E0C modern flags']
    names = ['ac_input_closed', 'precharge_bypassed', 'ac_pwm_enabled', 'dab_pwm_enabled',
             'pll_locked', 'voltage_loop_active', 'protection_enable', 'run_request', 'dab_request']
    for bit, name in enumerate(names):
        bits.append(dict(target='0x66002E0C modern flags', field=name, bit_low=bit, bit_high=bit,
                         mask=f'0x{1<<bit:04X}', values='0否/1是', implementation_state='已接入'))
    bits.append(dict(target='0x66002E0C modern flags', field='reserved', bit_low=9, bit_high=15,
                     mask='0xFE00', values='0', implementation_state='保留'))
    write_csv(DOCS / 'DPRAM_BITS.csv', bit_fields, bits)
    # A readable table keeps every individual read address beside the indexed write instruction.
    lines = ['# DPRAM现代参数点表', '', '生成源：`tools/dpram_point_table.py`；版本1，共54点。', '',
             '所有读地址为DSP侧16位字。回复区参数地址是只读回读镜像，不能直接写入；参数写入通过请求区提交：'
             'word3=1，word4（点号）和word5（raw），补齐CRC和提交标记，最后写左邮箱21。'
             '回读位于回复区word32+点号，须按版本戳读取。详见 [DPRAM_EXTENSION.md](DPRAM_EXTENSION.md)。', '',
             '`physical = raw / scale`；int16按二补码解释。R/request-W表示主机读实际值、通过请求写参数；'
             '只有sample_period_s只读。启停/复位按控制源和存储互锁；其余写入须完全停机。', '',
             '| 点号 | 实际回读地址 | 名称 | 类型 | scale/单位 | raw范围 | 权限 |',
             '|---:|---|---|---|---|---|---|']
    for p in table:
        lines.append(f"| {p['point']} | `{p['read_address']}` | `{p['field']}` | {p['wire_type']} | "
                     f"{p['physical_scale']} / {p['physical_unit']} | {p['raw_min']}..{p['raw_max']} | {p['host_permission']} |")
    lines += ['', '跨参数约束：开环调制≤modulation_max；current_feedback_sign只允许+1/-1（raw=1/FFFF）；'
              'DAB起始占空≤目标占空；HA_L_Voltage_Precharge<HA_H_Voltage。'
              '旧软件保护计时使用计数严格大于阈值的原判据，不能把计数阈值直接解释成精确跳闸延时。', '',
              'v2保存37个现代参数（硬件TAD仅校验）、模式/开环调制/频率/DC给定/Iq给定以及8个原保护/巡检值。'
              '启停、复位、快速保护总使能不保存；v1加载只恢复原42字，不改8个新增原参数。', '']
    (DOCS / 'DPRAM_PARAMETERS.md').write_text('\n'.join(lines), encoding='utf-8')
    report = dict(address_rows=len(addresses), parameter_points=len(table), bit_rows=len(bits),
                  protocol_version=1, unique_addresses=True)
    (ROOT / 'build').mkdir(exist_ok=True)
    (ROOT / 'build/dpram_point_table.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__': main()
