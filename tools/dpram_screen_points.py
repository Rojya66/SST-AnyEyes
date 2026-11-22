"""Export concrete legacy/ordinary Modbus addresses from the DSP descriptors and V1.1 table names."""
from pathlib import Path
import csv
import re
import json
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/DPRAM_Screen_V11'
REFERENCE = Path('D:/Code/Gemini/SST/输出文档/ARM_DSP_DPRAM与LAN通讯协议及上位机界面设计_V1.1.docx')


def main():
    OUT.mkdir(exist_ok=True)
    ns = {'w': 'http://schemas.openxmlformats.org/wordprocessingml/2006/main'}
    document = ET.fromstring(zipfile.ZipFile(REFERENCE).read('word/document.xml'))
    tables = []
    for table in document.findall('.//w:tbl', ns):
        tables.append([[''.join(t.text or '' for t in cell.findall('.//w:t', ns))
                        for cell in row.findall('w:tc', ns)] for row in table.findall('w:tr', ns)])
    (OUT / 'reference_tables.json').write_text(json.dumps(tables, ensure_ascii=False, indent=2), encoding='utf-8')
    rows = []

    def add(address, field, access, encoding, scale, meaning, source, condition='', minimum='', maximum=''):
        rows.append(dict(modbus_hex=f'0x{address:04X}', modbus_decimal=address,
                         display_40001=40001 + address, arm_byte=f'0x{0x60000000+2*address:08X}',
                         dsp_byte=f'0x{0x66000000+2*address:08X}', field=field, host_access=access,
                         encoding=encoding, multiply_before_encoding=scale,
                         raw_min=minimum, raw_max=maximum, meaning=meaning, source=source, condition=condition))

    host = 'Source/04_App/10_Host/'
    command_meaning = {2:'bit0 HVAC启动，仅run_enable置1，dab_enable保持原值；DAB用00F8/00F9 bit0独立控制',
        3:'bit0整机停机，run_enable/dab_enable均清0，优先执行',
        4:'bit0恒压能力确认；不改变ac_mode；其他模式位忽略并记录',
        6:'bit0 KT1吸合保持、bit1 QF1合闸脉冲；bit2/3忽略并记录',
        7:'bit0释放KT1、bit1 QF1分闸并释放KT1',8:'bit0 DAB ON、bit1 CHB ON',9:'bit0 DAB OFF、bit1 CHB OFF',
        10:'bit0停机复位',11:'bit0允许/bit1禁止外部合闸；两位同时1拒绝',12:'bit0 HVAC描述；其他端口位忽略并记录',
        13:'PT/CT变比及接线描述；兼容接收，不动态修改采样比例',
        14:'电网/负载及强弱、故障属性描述；兼容接收，不改变PLL或保护'}
    command_masks = [0,0,0xF,0xF,0x3FFF,0,0xF,0xF,3,3,1,3,0xF,0xFF,0x3F,0]
    local_masks = [0,0,1,1,1,0,3,3,3,3,1,3,1,0xFF,0x3F,0]
    for row in tables[5][1:]:
        i = int(row[0])
        meaning = command_meaning.get(i,'保留字接收忽略，发送端建议写0')
        meaning += f'；协议掩码0x{command_masks[i]:04X}，本机提取0x{local_masks[i]:04X}；未接入位不拦截正常命令，诊断见19A5'
        add(0xF0+i,row[1],'W request','uint16 bitmap',1,meaning,host+'SstDpramCommand.c', '完整16字写入后，最后0x1FFE=1')
    legacy_map = {0:('bits',1,'有效00FF；bit0运行1/测试0、bit1自动1/手动0；bit2..7其他端口不支持；bit8..15忽略'),
        1:('bits',1,'有效F000：bit12 IA、13 IB、14 IC、15 IN；低12位忽略'),
        2:('bits',1,'有效7FFF：FO1保护使能bit0..14；bit15忽略'),
        3:('bits',1,'有效7FFF：FO2保护使能bit0..14；bit15忽略'),
        4:('bits',1,'有效7FFF：FO3保护使能bit0..14；bit15忽略'),
        5:('bits',1,'有效FFFF：FB保护使能bit0..15对应U2..U17'),
        8:('int16',1,'单模块Udc目标V；回读取整；修改时范围1..6553V'),
        9:('int16',10,'Hz：开环输出频率或闭环PLL额定频率'),
        27:('int16 two-complement',100000,'voltage_kp，真实外环Kp'),
        28:('int16 two-complement',100,'voltage_ki，连续积分增益，执行乘Ts'),
        29:('int16 two-complement',1000,'current_kp，共用dq电流Kp，保留负号'),
        30:('int16 two-complement',1000,'current_ki，共用dq电流Ki，执行乘Ts，保留负号')}
    for index in range(23,27):
        legacy_map[index] = ('uint16',1,'本版保留字：接收忽略、回读0；不复用为调制或限流参数')
    for row in tables[8][1:]:
        i=int(row[0]); typ,scale,meaning=legacy_map.get(i,('int16',1,'未实现，实际回读0；非零写入整块拒绝，返回7'))
        add(0x1FF+i,row[1],'RW candidate / applied',typ,scale,meaning,host+'SstDpramParameter.c','停机且存储空闲；完整43字后最后0x1FFE=2，邮箱20表示已回读而非一定成功')
    for row in tables[10][1:]:
        i=int(row[0]); scale=10 if i in [3,4,8,9,10] else 100 if i==25 else 1
        meanings={0:'低6位：07停止、02预充、03旁路、05待PLL、20运行、08故障；bit7外部带电',
          1:'bit0 DI6 KT1合位、bit1 DI7 QF1合位、bit2 DI8 QF1分位；bit3..6保留；bit13..14 S_Mode，bit15保留；PWM位1闭锁',2:'Word故障位图，13通信、14同步；不复制内部复用位',
          3:'kW',4:'kvar',5:'单相AN RMS V；三相为端口线电压RMS V（保持测量生产者）',
          6:'B通道线电压RMS V；单相无效0',7:'C通道线电压RMS V；单相无效0',
          8:'A相RMS A',9:'B相RMS A；单相无效0',10:'C相RMS A；单相无效0',
          18:'A相串联总母线V=SVG_Udc_A_Ave*NHA_module_A；非单模块目标',25:'功率因数',
          30:'A相U8/701故障位图=FO_LATCH & Port_FO_1_EN & 本相PM位图；bitN对应OP_IN[N]，不重排模块序号',
          31:'B相U12/701故障位图=FO_LATCH & Port_FO_2_EN & 本相PM位图；bitN对应OP_IN[N]，不重排模块序号',
          32:'C相U16/701故障位图=FO_LATCH & Port_FO_3_EN & 本相PM位图；bitN对应OP_IN[N]，不重排模块序号'}
        add(0x500+i,row[1],'R','uint16 bitmap' if i<3 or 30<=i<=32 else 'int16 saturated',scale,
            meanings.get(i,'本版无实际控制/采样生产者，固定0且有效性位清零'),host+'SstDpramTelemetry.c','刷新约60ms；补充区有效性位图给出有效字段')
    text=(ROOT/'Source/04_App/01_FuncCode/SstParameterTable.c').read_text(encoding='utf-8-sig')
    pattern=r'\{\(volatile void \*\)&(g_chb_dab_(?:params|command)\.\w+),\s*([\d.]+)F,\s*(-?\d+)L,\s*(-?\d+)L,\s*(\d)U,\s*(\d)U\}'
    entries=re.findall(pattern,text)
    command_entries=entries[:8]; parameter_entries=entries[8:]
    assert len(parameter_entries)==37 and len(command_entries)==8
    snapshot=parameter_entries+[command_entries[i] for i in [2,4,5,6,7]]
    legacy_names=['OC_level_SET','OC_time_SET','OC_level_SET_L2','OC_time_SET_L2','OV_level_SET','OV_time_SET','HA_L_Voltage_Precharge','HA_H_Voltage']
    extra_names=['g_chb_dab_protection_enable','system_mode_HVAC','Port_AD_PORT_EN','Port_FO_1_EN','Port_FO_2_EN','Port_FO_3_EN','Port_FB_EN']
    for i in range(57):
        if i<42:
            name,scale,low,high,signed,word=snapshot[i];typ='int16 two-complement' if signed=='1' else 'uint16'
            meaning='唯一参数表直接成员；物理值=有符号/无符号解码后除比例'
            src='Source/04_App/01_FuncCode/SstParameterTable.c'
        elif i<50:
            name=legacy_names[i-42];scale=10 if i>=48 else 1;typ='uint16';low=0;high=65534 if i in [43,45,47] else 65535
            meaning='原软件保护/逐模块电压巡检；HA_H_Voltage是巡检高压门槛，不等同快速OV'
            src='Source/04_App/01_FuncCode/SstLegacyParameters.c'
        else:
            name=extra_names[i-50];scale=1;typ='uint16 bitmap';low=0;high=65535
            meaning='运行期配置，不包含在v2 EEPROM镜像；快速保护开关不影响硬件保护'
            src=host+'SstDpramParameter.c'
        for base,access in [(0x1A00,'RW staged'),(0x1A80,'R applied')]:
            add(base+i,name, 'R hardware timing' if i==0 else access,typ,scale,meaning,src,
                'STOP且存储空闲；先REFRESH再编辑，最后APPLY；索引0不可改变',low,high)
    statuses={0:('epoch','奇数发布中；前后读取相同偶数才接受跨区快照'),1:('magic','0x5353'),2:('map_version','0x0102；1B01直接提交，1B02为DSP完成计数'),3:('parameter_count','57'),
       4:('control_count','块1消费计数'),5:('control_result','块1处理结果'),6:('regulate_count','块2消费计数'),7:('regulate_result','块2处理结果'),8:('error_word','最近错误字段/参数号，FFFF非单字段'),9:('core_state','原核心0..5'),10:('core_fault','CHB_FAULT_*'),11:('startup_phase','启动阶段0..3'),
       12:('tick_10ms_L','10ms计数低字'),13:('tick_10ms_H','10ms计数高字'),14:('build_flags','bit0单相、1实际开关输出、2反馈、3允许远控'),15:('run_flags','bit0 CHB PWM、1 DAB PWM、2 PLL锁定、3保留固定0、4外环生效'),16:('switch_commands','bit0 QF1合闸目标、bit1 KT1保持命令；非实际触点'),17:('switch_din','S301原始DIN：bit0急停、1复归、2远方、3开机、4停机、5 KT1合位、6 QF1合位、7 QF1分位；1触点闭合'),18:('switch_feedback','bit0 DI7 QF1合位、bit1 DI6 KT1合位、bit2 DI8 QF1分位；需build_flags.bit2有效'),19:('storage_result','参数存储最近结果'),32:('capabilities','bit0旧遥控、1组1PI、2直接参数、3EEPROM'),33:('telemetry_valid_L','块3 word0..15有效位'),34:('telemetry_valid_H','块3 word16..31有效位'),35:('switch_permission','外部合闸许可'),36:('plugin_valid','bit0..15=U2..U17有配置生产者；不证明链路实时健康'),37:('ignored_control_words','最近块1被忽略的未接入字段位图，bitN=wordN；仅诊断，不拒绝正常命令；纯保留内容不计入')}
    statuses[38]=('telemetry_valid_32_47','块3 word32..47有效位；bit0表示0520的C相701故障位图已接入')
    floats=['Udc_module_average_v','Idref_a','Id_feedback_a','Iq_feedback_a','startup_Udc_ref_v','PLL_frequency_hz']
    for i in range(64):
        name,meaning=statuses.get(i,('reserved','保留0'))
        if 20<=i<32:
            name=floats[(i-20)//2]+('_L' if i%2==0 else '_H');meaning='IEEE754 float32，低16位在前，必须用epoch检查两字一致'
        add(0x1980+i,name,'R','uint16 / float32 halves',1,meaning,host+'SstDpramScreen.c')
    faults={0:'Word_PORT_FAULT',1:'raw_Port_RealFault',2:'raw_FaultTypeFLAG',3:'core_fault',4:'first_current_code',5:'level',6:'active_count',7:'history_count',40:'HVAC_OC_Fault',41:'HVAC_OV_Fault'}
    for i in range(64):
        name=faults.get(i,'active_code_'+str(i-8) if 8<=i<40 else 'reserved')
        add(0x19C0+i,name,'R','uint16',1,'当前详细五位码表；不是首触发锁存，历史沿用0x1780区域',host+'SstDpramScreen.c')
    txn=[('reserved_1B00','RW ignored','保留，默认0；写入不参与解析，屏幕无需设置'),
         ('operation','W last','单写1 APPLY、2 SAVE、3 LOAD、4 REFRESH即可提交；DSP受理后清0；忙时写入清0丢弃'),
         ('completed_count','R','DSP每笔处理完成后自动加1，含失败；16位回绕；屏幕无需维护或匹配序号'),
         ('result','R','0成功1地址2只读3范围4忙5拒绝6存储7不支持；等operation=0且busy=0后读取'),
         ('busy','R','1操作处理中；0空闲；结果和完成信息发布后最后清0'),
         ('error_parameter','R','参数号/旧块字号；FFFF未细分'),('completed_operation','R','最近完成操作')]
    for i in range(16):
        name,access,meaning=txn[i] if i<len(txn) else ('reserved','R','保留0')
        add(0x1B00+i,name,access,'uint16',1,meaning,host+'SstDpramScreen.c','op=0且busy=0时单写操作字；等待op=0且busy=0后读result；APPLY完成前冻结候选区')
    for i in range(16):
        unit=i+2
        fields=[]
        for name in ['DC','DC_LATCH','IR','IR_LATCH','IG','IG_LATCH','TEMP','TEMP_LATCH','STATUS','HD_S_PH','DRIVER','Reserved_6']:
            fields.extend(f'{name}[{j}]' for j in range(8))
        fields+=['DIN','DIN_LATCH','AD_PROT','AD_PROT_LATCH','FO','FO_LATCH','FB','FB_LATCH']
        fields += [f'Reserved_7[{j}]' for j in range(8)]
        fields += ['OR_STATUS','Reserved_8[0]','Reserved_8[1]','Reserved_8[2]','Left_Ref','Right_Ref',
                   'Reserved_9[0]','Reserved_9[1]','INV_FO','INV_FO_LATCH','Sum_DC','Avg_DC','Sum_IG','Avg_IG','Sum_IR','Avg_IR']
        assert len(fields)==128
        for offset,field in enumerate(fields):
            add(0x680+i*0x100+offset,f'U{unit}.{field}','R','raw FPGA words',1,
            '128字原始镜像；各槽位间隔0x100字；只消费plugin_valid置位的槽位；量纲沿用插件FPGA',
            'Source/02_Peripheral/01_DPRAM/DPRAM.C','B/C未配置、U7/U11/U15及U17不发布有效镜像')
    add(0x1FFE,'left_mailbox','W last','uint16',1,'1控制、2遥调；读此地址清除DSP侧中断通知','Source/02_Peripheral/01_DPRAM/DPRAM.C')
    add(0x1FFF,'right_mailbox','R','uint16',1,'3遥测、4..19插件、20遥调实际回读；非队列，可能被后续事件覆盖','Source/02_Peripheral/01_DPRAM/DPRAM.C')
    rows.sort(key=lambda row:row['modbus_decimal'])
    functions={'SstDpramCommand.c':'SstDpramCommand_ProcessControlBlock', 'SstDpramParameter.c':'SstDpramParameter_ApplyParameterSnapshot',
        'SstDpramTelemetry.c':'SstDpramTelemetry_BuildLegacyBlock','SstDpramScreen.c':'SstDpramScreen_PublishStatus',
        'DPRAM.C':'DPRAM_Write','SstParameterTable.c':'SstParameterTable_SnapshotRead',
        'SstLegacyParameters.c':'SstLegacyParameters_Read'}
    for row in rows:
        path=ROOT/row['source'];text=path.read_text(encoding='utf-8-sig')
        function=functions[path.name]
        if path.name=='SstDpramParameter.c' and 0x01FF<=row['modbus_decimal']<=0x0229:
            function='SstDpramParameter_DecodeAndApplyLegacyBlock'
        if path.name=='SstDpramScreen.c' and 0x1B00<=row['modbus_decimal']<=0x1B0F:
            function='SstDpramScreen_ProcessTransactions'
        match=re.search(r'\b'+function+r'\s*\([^;{}]*\)\s*\{',text)
        row['source_line']=text[:match.start()].count('\n')+1
    with (OUT/'DPRAM_MODBUS_POINTS.csv').open('w',encoding='utf-8-sig',newline='') as stream:
        writer=csv.DictWriter(stream,fieldnames=list(rows[0]));writer.writeheader();writer.writerows(rows)
    lines=['# 参数逐项表','', '物理值=解码后的整数÷比例。int16使用二补码；0..41沿用唯一参数表，42..49原保护，50..56运行期配置。', '',
           '|序号|待应用Modbus|实际Modbus|成员|类型|乘数|原始范围|','|---|---|---|---|---|---|---|']
    for i in range(57):
        row=next(row for row in rows if row['modbus_decimal']==0x1A00+i)
        lines.append(f"|{i}|`0x{0x1A00+i:04X}`|`0x{0x1A80+i:04X}`|`{row['field']}`|{row['encoding']}|{row['multiply_before_encoding']}|{row['raw_min']}..{row['raw_max']}|")
    lines+=['','表内范围之外还检查sample_period不变、DAB起始≤目标、开环调制≤最大值、电流反馈符号仅±1、巡检低压<高压；参数0硬件采样周期只读。',
            'AD只允许F000，FO仅7FFF，系统方式仅0..3，快速保护仅0/1。未分配待应用/实际区剩余71字保留，不得写入。']
    (OUT/'PARAMETERS.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    print(f'Exported {len(rows)} address rows and 57 parameter mappings to {OUT}')


if __name__=='__main__':
    main()
