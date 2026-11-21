"""Run production Host entry points against the legacy block-20 ready/consume contract."""
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def main():
    compiler = shutil.which('gcc')
    assert compiler, 'Host GCC required'
    source = (ROOT / 'Source/04_App/10_Host/SstDpramHost.c').read_text(encoding='utf-8-sig')
    functions = []
    for name in ['SstDpramHost_Initialize', 'SstDpramHost_ProcessIncomingBlocks']:
        match = re.search(r'^void ' + name + r'\(void\)\s*\{.*?^\}', source, re.M | re.S)
        assert match, name
        functions.append(match.group(0))
    fixture = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "DpramConfig.h"
#define SST_DPRAM_MAINTENANCE_BLOCK 21U
#define MainControl_PluginUnit_4 4U

/* 外围替身只验证Host与旧写接口的ready/consume约定，参数算法由screen回归另测。 */
struct { struct { struct { uint16_t led3; } Bbits; } LED; } FPGA_Interface[3];
struct { uint16_t DPRAM_Block1Buf[16]; } PORT_Control_DATA_Buf;
struct { uint16_t DPRAM_Block2Buf[43]; } PORT_Regulate_DATA_Buf;
struct { uint16_t control_result, control_count, regulate_result, regulate_count; } g_sst_dpram_screen_result;
uint16_t Flag_RemoteControlRegulate, FLAG_NEW_L, FLAG_RemRegulate_SetOver, rtt, WriteBlock_Index;
static uint16_t s_regulate_reply_sent; /* 与生产Host同名的后台通知让出标志。 */
static uint16_t actual[43], incoming[43], shared[43]; /* 实际参数、ARM请求、共享区回读。 */
static uint16_t reply_result, replies, mailbox; /* 受理结果、实际回写次数、通知值。 */

/* 参数层替身：成功才应用电压，失败保持实际值，用于验证Host回传的是实际值。 */
uint16_t SstDpramParameter_DecodeAndApplyLegacyBlock(const uint16_t *words)
{
    if (!reply_result) actual[8] = words[8];
    return reply_result;
}
void SstDpramParameter_EncodeLegacyBlock(uint16_t *words) { memcpy(words, actual, sizeof(actual)); }
/* 读接口替身：模拟邮箱2及原43字复制。 */
void DPRAM_Read(void)
{
    FLAG_NEW_L = ReadBlock_remRegulate;
    memcpy(PORT_Regulate_DATA_Buf.DPRAM_Block2Buf, incoming, sizeof(incoming));
}
/* 旧DPRAM_Write(20)在ready=0时不会复制；本测试要求Host每笔明确建立回写许可。 */
void DPRAM_Write(uint16_t block)
{
    assert(block == WriteBlock_remRegulate && FLAG_RemRegulate_SetOver == 1U);
    memcpy(shared, PORT_Regulate_DATA_Buf.DPRAM_Block2Buf, sizeof(shared));
    FLAG_RemRegulate_SetOver = 0U;
    mailbox = block;
    ++replies;
}
void SstDpramMaintenance_MarkRequestPending(void) {}
uint16_t SstDpramCommand_ProcessControlBlock(const uint16_t *words) { (void)words; return 0U; }
/* PRODUCTION_FUNCTIONS */
int main(void)
{
    unsigned int index; /* 成功、范围错误、忙、不支持、拒绝的连续提交序号。 */
    static const uint16_t results[] = {0U, 3U, 4U, 7U, 5U, 0U};
    actual[8] = 60U;
    SstDpramHost_Initialize();
#if SST_ENABLE_DPRAM
    assert(replies == 1U && shared[8] == 60U && mailbox == 20U);
    assert(!FLAG_RemRegulate_SetOver && WriteBlock_Index == WriteBlock_PluginUnit_2);
    for (index = 0U; index < sizeof(results) / sizeof(results[0]); ++index)
    {
        uint16_t expected; /* 本笔应回传的实际电压，不是无条件回显候选。 */
        reply_result = results[index];
        incoming[8] = (uint16_t)(61U + index);
        expected = reply_result ? actual[8] : incoming[8];
        Flag_RemoteControlRegulate = 1U;
        SstDpramHost_ProcessIncomingBlocks();
        assert(replies == index + 2U && shared[8] == expected && mailbox == 20U);
        assert(!FLAG_RemRegulate_SetOver && !Flag_RemoteControlRegulate);
        assert(g_sst_dpram_screen_result.regulate_count == index + 1U);
        assert(g_sst_dpram_screen_result.regulate_result == reply_result);
        assert(s_regulate_reply_sent);
        SstDpramHost_ProcessIncomingBlocks();
        assert(replies == index + 2U); /* 没有新通知不得重复发布。 */
    }
    puts("PASS: production Host init and six consecutive success/failure replies rearm legacy block20 and return actual values");
#else
    (void)index; (void)results;
    Flag_RemoteControlRegulate = 1U;
    SstDpramHost_ProcessIncomingBlocks();
    assert(!replies && !mailbox && !FLAG_RemRegulate_SetOver);
    puts("PASS: disabled DPRAM Host performs no read/write or reply");
#endif
    return 0;
}
'''.replace('/* PRODUCTION_FUNCTIONS */', '\n'.join(functions))
    with tempfile.TemporaryDirectory(prefix='sst_host_reply_') as directory:
        unit = Path(directory) / 'host_reply.c'
        unit.write_text(fixture, encoding='utf-8')
        for enabled in [1, 0]:
            binary = Path(directory) / f'host_reply_{enabled}.exe'
            subprocess.run([compiler, '-std=c99', '-O2', '-Wall', '-Wextra', '-Werror',
                            '-Wno-unused-variable', '-I', str(ROOT / 'Source/02_Peripheral/01_DPRAM'),
                            f'-DSST_ENABLE_DPRAM={enabled}', str(unit), '-o', str(binary)], check=True)
            result = subprocess.run([str(binary)], capture_output=True, text=True, timeout=10)
            assert result.returncode == 0, result.stdout + result.stderr
            print(result.stdout.strip())


if __name__ == '__main__':
    main()
