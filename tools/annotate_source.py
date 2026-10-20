"""One-time, comment-only application annotation, using reviewed semantic tables.

Always reads the saved pre-annotation baseline. It never changes code tokens.
Tree-sitter is an optional documentation tool, not a firmware build dependency.
"""
from pathlib import Path
from collections import defaultdict
import hashlib
import json
import re
from comment_inventory import ROOT, source_files, collect, walk, identifier
from comment_semantics import FUNCTIONS, VARIABLES, LOCALS

PARAMETERS = dict(row.split('|',1) for row in '''
CJ_index|插件编号，按原工程从 2 开始，寄存器数组下标为编号减 2
MK_index|插件内模块编号，从 1 开始，模块数组下标为编号减 1
CS_index|ModuleIndex_* 采样类型编号
latch|0 读取实时寄存器，非零读取锁存寄存器
VIa|A 相瞬时输入，与调用者电压/电流量同单位
VIb|B 相瞬时输入，与调用者电压/电流量同单位
VIc|C 相瞬时输入，与调用者电压/电流量同单位
VIalpha|α 轴输入，与调用者电压/电流量同单位
VIbeta|β 轴输入，与调用者电压/电流量同单位
VId|d 轴输入，与调用者电压/电流量同单位
VIq|q 轴输入，与调用者电压/电流量同单位
Cos_ph|变换相角余弦值
Sin_ph|变换相角正弦值
ErrorIn|本拍控制误差，与被控制物理量同单位
FilterIN|本拍滤波输入，与输出同单位
SourceAddr|源地址，连续数据至少覆盖要求的复制长度
DestAddr|输出目标地址，调用者须提供足够容量
RW_fpga|原 FPGA 插件通信读/写选择值
IndexM|模块桥类型使能位图，沿用原函数按位解释
EN50Hz|工频桥使能，0 禁用、非零使能
ENdab|DAB 桥使能，0 禁用、非零使能
DoublingValue50Hz|工频桥驱动分频字段值
DoublingValueDAB|DAB 驱动分频字段值
Rst50Hz|工频桥驱动复位控制值
RstDab|DAB 驱动复位控制值
runindex|模块硬件复位控制值
FANdisable|模块风扇关闭控制值
valid50Hz|工频桥关断有效控制值
validDab|DAB 桥关断有效控制值
TestMode|模块单链测试模式字段值
OpticalNum|光纤插件/组编号
Shutdown_1|第 1 组模块关断位图
Shutdown_2|第 2 组模块关断位图
Shutdown_3|第 3 组模块关断位图
Shutdown_4|第 4 组模块关断位图
Shutdown_5|第 5 组模块关断位图
INV_A_FOEN|A 相逆变故障光纤使能位图
INV_B_FOEN|B 相逆变故障光纤使能位图
INV_C_FOEN|C 相逆变故障光纤使能位图
INV_N_FOEN|中性桥故障光纤使能位图
Deadzone_inv|逆变桥死区 FPGA 计数
Period_inv|逆变桥周期 FPGA 计数
LatchValue|BKU 故障锁存控制值
RST_FB|端口硬件故障复位控制值
AllShutdown|全模块关断控制值
FT3_index|PTCT_FT3/FaultSwitch_FT3/EER2_FT3/Backup_FT3 插件选择
dataPackNum|FT3 本次发送的数据包数/控制字字段
Flag_R|WriteBlock_* DPRAM 回写块号
Inital_Y|旧遥调参数来源/初始化选择值，沿用调用点约定
u8PageNum|EEPROM 页号，每页 64 字节
pu8Offset|EEPROM 页内字节偏移
u8Data|待写入字节，或调用者提供的段写数据缓存
pu8RecByte|输出页读取缓存，容量至少 64 字节
u8Byte|软件 I²C 待发送字节，高位先发
time|输出 RTC 时间寄存器字节缓存
rec_time|输入 RTC 时间寄存器字节缓存
val|待转换的两位 BCD/十进制数值
instanceNum|UART 实例编号，沿用板级可用实例
modemCtrlChoice|UART RTS/CTS 流控引脚选择
PCcomm|旧 UART 通信请求缓存
StatcomIndex|旧 STATCOM 无功、功率因数或电压模式选择
ApfIndex|旧 APF 谐波补偿模式选择
HbridgeIndexEn|旧 PWM 桥类型/使能选择值
IndexDis|旧 PWM 禁用类型/范围选择值
RstFlagFault|旧故障复位阶段选择
ReadBlock_remRegulate|旧遥调块编号/兼容请求字段
group|固定参数组号，范围 0..29
index|所选表/对象的索引；范围按函数对应表容量检查
newest_index|历史记录从新到旧的索引，0 为最新
signedValue|非零按有符号 16 位编码，0 按无符号编码
load|0 请求保存，非零请求加载
factor|物理值到线上整数的缩放乘数
dataList|输入参数/帧数据缓存，按该接口的固定字格式解释
DataList|参考 PcDebug 的接收字节数组，每字只使用低 8 位
ValidLenth|输入完整帧的有效字节数指针，沿用参考字段拼写
validLength|参考接口的有效数据长度
instruction|参考 AnyEyes 响应命令码，只使用低 8 位
byte|后台收到的单个 UART 字节
start_word|原遥控启动字，按控制核心的端口位解释
stop_word|原遥控停止字，停止优先于启动
start|DPRAM/维护启动位图，停止/关脉冲优先
stop|DPRAM/维护停止位图，停止优先于启动
pulse_on|脉冲使能位图，bit0 为 DAB、bit1 为 CHB
pulse_off|脉冲禁用位图，bit0 为 DAB、bit1 为 CHB，优先于使能
reset|故障复位请求位图，仅停机时接受
closed|交流输入或预充旁路闭合选择，0 断开、非零闭合
angle|待归一化角度，单位 rad
kp|PI 比例系数
ki|PI 积分系数，计算时乘 sample_period_s
period|控制采样周期，单位 s
id_reference|用于 PI 接管对齐的 d 轴电流给定，单位 A
state|CHB_STATE_* 目标状态
port_status|旧端口 PortStatusNum_* 兼容状态值
fault_code|CHB_FAULT_* 控制故障编号
minimum|限幅下界，与 value 同单位
maximum|限幅上界，与 value 同单位
fault_code|控制核心 CHB_FAULT_* 故障编号
flags|维护帧响应/错误标志位图
sequence|维护请求序号，响应原样回带
command|维护协议命令编号
object|维护协议对象类型编号
error|维护协议 MAINT_ERR_* 错误码
offset|对象中的字偏移或镜像字节偏移，按接口说明解释
record|输出详细故障记录的目标指针，不可为空
input|已经后台采集并检查关键状态一致性的故障输入
level|参考故障等级，0..4
code|参考五位 SST_FAULT_* 上报码
payload|输入帧载荷缓存，不含帧头与 CRC
values|输入浮点波形通道数组，容量至少 count
bytes|输入完整帧字节缓存，容量至少 length
enable|启停命令，0 停止、1 启动
word|参数浮点值的半字序号/16 位数据，按对应函数约定
size|输入缓存有效长度
'''.strip().splitlines())

def function_info(name):
    key = name.split('::')[-1]
    if key in FUNCTIONS:
        return FUNCTIONS[key]
    match = re.fullmatch(r'GPIOBank(\d+)Pin(\d+)PinMuxSetup', key)
    if match:
        return dict(purpose=f'把 GPIO bank {match[1]} 的 pin {match[2]} 配置为板级指定复用功能，保留其他引脚位',
                    result='无', context='上电板级引脚配置；寄存器偏移和复用码沿用原工程。', output='对应 SYSCFG PINMUX 寄存器。')
    raise KeyError('Missing function: '+name)

def old_comment(data, symbol):
    tail = data[symbol['end']:data.find(b'\n',symbol['end']) if b'\n' in data[symbol['end']:] else len(data)].decode()
    match = re.search(r'^\s*(?://(.*)|/\*(.*?)\*/)',tail)
    if match:
        comment = (match[1] or match[2]).strip()
        if len(re.findall(r'[\u4e00-\u9fff]',comment))>=4:
            return re.sub(r'/\*|\*/','',comment).strip()
    return None

REGISTERS = dict(row.split('|',1) for row in '''
ID|插件身份、类型、端口及标识寄存器
LED|插件八路 LED 控制寄存器
NUM|原调制算法在线模块数量寄存器
PM|本插件在线模块位图
Reference|工频桥原始调制参考计数
Orient|模块调制/电流方向字段
DC_Log|模块直流电压排序及电流方向控制字
TAD|FPGA 控制周期计数
DAB_LEFTDuty|DAB 左桥占空比计数
DAB_RIGHTDuty|DAB 右桥占空比计数
DAB_Start|DAB 脉冲起始计数
PM_CTL|工频桥/DAB 使能、分频、复位及测试控制字
TX_EN|插件发送模块使能位图
TX_CTL|插件发送控制寄存器
TX|插件原始发送数据区
RX|插件原始接收数据区
DOUT|数字输出控制字
AD_PROT_EN|端口 AD 保护使能位图
SHUTDOWN|模块硬件关断位图
FO_EN|光纤故障输入使能位图
INV_SHUTDOWN|独立逆变桥关断位图
INV_FO_EN|独立逆变桥光纤故障使能位图
INV_REF|独立逆变桥原始调制参考计数
INV_CTL|独立逆变桥周期/死区控制字
BUCK_DUTY|BUCK 占空比计数
BUCK_CTL|BUCK 使能及周期控制字
BKU|BKU 故障锁存控制寄存器
FB_EN|FB 硬件故障输入使能位图
TEST3A|原 FPGA 测试寄存器 A
TEST3B|原 FPGA 测试寄存器 B
Debug_L|左桥 FPGA 调制调试寄存器
Debug_R|右桥 FPGA 调制调试寄存器
DSP_CTL|DSP 全关断及硬件复位控制字
AD|端口原始 AD 实时采样数组
DC|模块直流电压原始实时采样数组
IR|模块直流电流原始实时采样数组
IG|模块交流/端口电流原始实时采样数组
TEMP|模块温度原始实时采样数组
STATUS|各模块原始实时状态字数组
HD_S_PH|原 HVDC 同步相位字段
DRIVER|各模块驱动原始状态字数组
DIN|端口数字输入实时位图
AD_PROT|端口 AD 保护实时故障位图
FO|光纤故障输入实时位图
FB|FB 硬件故障输入实时位图
OR_STATUS|模块状态按位或的汇总值
Left_Ref|左桥实际参考回读
Right_Ref|右桥实际参考回读
INV_FO|独立逆变桥光纤故障实时位图
Sum_DC|本插件模块直流电压采样总和
Avg_DC|本插件模块直流电压采样平均值
Sum_IG|本插件模块交流电流采样总和
Avg_IG|本插件模块交流电流采样平均值
Sum_IR|本插件模块直流电流采样总和
Avg_IR|本插件模块直流电流采样平均值
'''.strip().splitlines())

def patterned(name, owner='', declaration=''):
    if name in VARIABLES: return VARIABLES[name]
    if name in REGISTERS: return REGISTERS[name]+'，保持原 FPGA/DPRAM 字布局'
    if re.fullmatch(r'(HVDC|LVDC|LVAC)_(Start|STOP)',name): return name[:4]+' 端口的原遥控'+('启动' if name.endswith('Start') else '停止')+'位，仅保留协议布局'
    if re.fullmatch(r'(Port_)?FO_[123]_EN',name): return '第 '+name[-4]+' 组光纤故障输入使能位图'
    if re.fullmatch(r'PORT_[VI]_[ABCN]',name): return '端口 '+name[-1]+' 相/中线'+('电压' if '_V_' in name else '电流')+'遥测；float 为物理量，16 位版比例见 RemoteSig_Dispose'
    if re.fullmatch(r'[VI]_[ABC]_2',name): return '端口第二组 '+name[2]+' 相'+('电压' if name[0]=='V' else '电流')+'遥测；16 位版比例见 RemoteSig_Dispose'
    if re.fullmatch(r'(RMS|PHA)_CURRENT_(5|7|11)TH(?:_SET)?',name):
        harmonic=re.search(r'_(\d+)TH',name)[1]
        return harmonic+' 次谐波电流'+('有效值' if name.startswith('RMS') else '相位')+('给定' if name.endswith('SET') else '遥测')+'；定点/浮点比例见旧遥调/遥测转换'
    if re.fullmatch(r'PORT_[VI]_[PI]_[123]',name): return '旧第 '+name[-1]+' 组'+('电压' if name[5]=='V' else '电流')+'环'+('比例' if name[7]=='P' else '积分')+'遥调系数，保持原缩放约定'
    if name.endswith('_LATCH') and name[:-6] in REGISTERS:
        return REGISTERS[name[:-6]].replace('实时','锁存')+'，供故障/同步采样使用'
    if re.match(r'Reserved|Invalid',name,re.I): return '原硬件/协议保留字段，用于保持字偏移及位域布局，不作为新增控制量'
    if name in ('H_Filter','Har_Controller','PID_Controller','FT3_Register','DPRAM_PluginUnit','DPRAM_RemControl','DPRAM_RemMeaSig','DPRAM_RemRegulate'):
        return '对应结构化字段视图，与本联合体的整字数组重叠，布局保持原工程'
    if name.endswith('16') or '_Word16' in name: return '本联合体的完整 16 位字视图，与位域成员共用存储'
    if name.endswith(('_BUF','Buf')): return '本联合体的连续原始字/状态数组视图，供复制或存储打包使用'
    if name in ('IN_n_1','IN_n_2','OUT_n_1','OUT_n_2'):
        return ('前一拍' if name.endswith('1') else '前两拍')+('输入' if name.startswith('IN') else '输出')+'历史值，单位与该控制器输入/输出一致'
    if name in ('Num_1','Num_0'): return '增量 PI 的'+('本拍' if name.endswith('1') else '前一拍')+'误差系数'
    if re.fullmatch(r'led[0-7]',name): return f'第 {name[-1]} 路插件 LED 控制位，1 点亮、0 熄灭'
    if re.fullmatch(r'DA_DATA[0-3]',name): return f'第 {name[-1]} 路 DA 调试输出原始计数寄存器'
    if re.fullmatch(r'Rec_Data[1-8]',name): return f'FT3 接收第 {name[-1]} 个原始 16 位字'
    if re.fullmatch(r'FR_tri_count[1-3]',name): return f'原 FT3 录波第 {name[-1]} 组触发计数'
    if name.startswith('DPRAM_Block4_PluginUnit'): return '原地址映射的插件 '+name[len('DPRAM_Block4_PluginUnit'):]+' 遥测块，仅启用 DPRAM 时访问'
    if name.startswith('MainStatus_'): return name[11:]+' 端口主状态及模式缓存；仅对应条件编译分支使用'
    if name.startswith(('WrongTimes_','LastWrong_','Wrong_')):
        prefix,channel = name.split('_',1)
        return f'原 FPGA {channel} 参考/通信核对的'+dict(WrongTimes='累计不一致次数',LastWrong='上一轮不一致值',Wrong='本轮不一致值')[prefix]
    if name.startswith(('Flag_','cout','countWait')):
        times = dict(WaitPowerStart10s='主电源等待 10 s', **{'1s':'周期 1 s','50ms':'周期 50 ms','100ms':'周期 100 ms','RemoteMeasure':'遥测轮询','Switch500ms':'开关操作等待 500 ms','Switch1s':'开关操作等待 1 s','Precharge500ms':'预充等待 500 ms','PreStart3s':'启动准备等待 3 s','SVG1s':'SVG 投入等待 1 s','20s':'上电等待 20 s','StopWait':'停机等待','_Wait':'原控制等待','200us':'原名为 200 us 的兼容定时'})
        base = re.sub(r'^(?:Flag_|count|cout)','',name)
        if base in times:
            return ('Timer2 更新、后台消费的'+times[base]+'到时标志' if name.startswith('Flag') else 'Timer2 原 '+times[base]+'计数；实际节拍沿用原判断式')
    if re.fullmatch(r'counter_(OC|OV)_(flag|time)1?',name):
        return '原'+('二级' if name.endswith('1') else '一级')+('过流' if '_OC_' in name else '过压')+('连续超限样本计数' if '_flag' in name else '保护确认计时计数')
    if name.startswith('FF_Module_'): return '本轮故障分类的模块'+name[10:].replace('OverVoltLevel2','二级过压').replace('OverVolt','过压').replace('OverCurrent','过流').replace('OverTemp','过温').replace('DAB_H','DAB 桥').replace('50Hz_H','工频桥')+'位图'
    if re.fullmatch(r'(HA|LA|HD|LD)_PM_[ABC][12]',name): return name[:2]+' 端口 '+name[-2:]+' 组在线模块位图，每位对应一个模块'
    if re.fullmatch(r'N(H|L)[AD]_module(?:_[ABC])?',name): return name[1:3]+' 端口'+(' '+name[-1]+' 相' if name[-1] in 'ABC' else '')+'在线模块数量，用于采样平均及调制尺度'
    if name.startswith(('HA_','LA_','HD_','LD_')):
        port = dict(HA='高压交流',LA='低压交流',HD='高压直流',LD='低压直流')[name[:2]]
        tail=name[3:]
        phase=re.search(r'_(A|B|C|AB|BC|CA)(?:_|$)',tail)
        ph=phase[1]+' 相/相间' if phase else ''
        if 'Rms_LF_' in tail: return port+'端口 '+tail.split('_LF_')[0]+' 有效值低通滤波第 '+tail[-1]+' 个历史状态，电压 V、电流 A'
        if re.fullmatch(r'Port[PQ]_LF_[12]',tail): return port+'端口'+('有功' if tail[4]=='P' else '无功')+'低通滤波第 '+tail[-1]+' 个历史状态'
        if 'Volt' in tail or 'Voltage' in tail:
            return port+'端口 '+ph+('模块电压调制给定' if 'RefVolt' in tail else '原采样电压')+'，单位 V'+('；上一拍噪声比较缓存' if tail.endswith('_last') else '；锁存采样值' if tail.endswith('_LATCH') else '')
        if 'Current' in tail: return port+'端口 '+ph+dict(Grid='电网',Port='端口',Lg='滤波电感',Cap='电容').get(tail.split('_')[0],'')+'电流，单位 A'+('；锁存采样值' if tail.endswith('_LATCH') else '')
        if tail.startswith(('DC_Log','IG_Log')): return port+'端口 '+ph+('直流电压排序方向字' if tail.startswith('DC') else '电流方向控制字')
        if tail.startswith('TotalVDC'): return port+'端口 '+ph+'在线模块直流电压总和，单位 V'
        if tail.startswith('ReferenceDEF'): return port+'端口 '+ph+'按在线模块数得到的 FPGA 调制参考基值'
        if tail.startswith('DABDuty'): return port+'端口 '+ph+'DAB 占空比 FPGA 计数'
        if tail.startswith('DAB_PluseStart'): return port+'端口 '+ph+'DAB 脉冲起始 FPGA 计数'
        if tail=='DAB_T': return port+'端口 DAB 周期 FPGA 计数'
        if tail.endswith('LV_Precharge'): return port+'端口 '+ph+'预充欠压判断标志'
        if tail.endswith('HV_Protect'): return port+'端口 '+ph+'原直流高压判断标志'
    if name.startswith('HVAC_Real'):
        return 'HVAC 实时量窗口的'+dict(Voltage='电压',Current='端口电流',GridCurrent='电网电流').get(name[9:].split('_')[0],'采样')+('累加缓存，供原有效值计算' if name.endswith('Sum') else '采样窗口计数')
    if re.fullmatch(r'SVG_Udc_[ABC]_Ave',name): return name[8]+' 相在线模块平均直流电压，单位 V'
    if name.startswith('LPF_'): return '原 '+name.split('_')[1]+' 低通滤波 '+name.split('_')[2]+' 的上一拍'+('输入' if '_X_' in name else '输出')+'状态'
    if name.startswith(('Sin_ph_Trans','Cos_ph_Trans')): return ('正弦' if name.startswith('Sin') else '余弦')+'坐标变换系数'+('，含原相位补偿' if '_Com' in name else '')+('，下一拍预测值' if '_next' in name else '，输出变换使用值' if '_NEW' in name else '')
    if name.endswith('_PID_Controller'):
        return name[:-15].replace('PLL','锁相环').replace('Volt','直流电压').replace('Current_d','d 轴电流').replace('Current_q','q 轴电流').replace('Neg_','负序').replace('Cluster','相间平衡')+'增量 PI 的系数与上一拍输入/输出历史状态'
    if '_PID_Num_' in name: return name.split('_PID_')[0].replace('PLL','锁相环').replace('Volt','电压').replace('PH_','相间').replace('Current','电流').replace('Neg_','负序')+'增量 PI '+('本拍' if '_Num_1' in name else '上一拍')+'误差系数'+('的预设值' if name.endswith('Pres') else '')
    if re.fullmatch(r'Har_(2|6|12)_Factor',name): return name.split('_')[1]+' 次谐振控制系数调整因子'
    if re.fullmatch(r'Har_(2|6|12)_Controller_(Num|Den)_[012]',name): return name.split('_')[1]+' 次谐振控制器差分式'+('分子' if '_Num_' in name else '分母')+'第 '+name[-1]+' 项系数'
    if name.endswith('_Filter'):
        return name[:-7].replace('U_d_Grid','电网 d 轴电压').replace('U_q_Grid','电网 q 轴电压').replace('I_d_SVG','端口 d 轴电流').replace('I_q_SVG','端口 q 轴电流').replace('I_d_Load','负载 d 轴电流').replace('I_q_Load','负载 q 轴电流').replace('I_d_Ref','d 轴电流给定').replace('Q_Load','负载无功').replace('Va_Ave','A 相直流平均电压').replace('Vb_Ave','B 相直流平均电压').replace('Vc_Ave','C 相直流平均电压').replace('Ave','平均量')+'滤波器的四个历史输入/输出状态'
    if name.endswith('_Har_Controller'): return name[:-15]+' 次谐振控制器的历史输入/输出状态'
    if re.fullmatch(r'Har_(2|6|12)_Controller',name): return name.split('_')[1]+' 次谐振通用历史输入/输出缓存'
    if re.fullmatch(r'Har_(5|7|11)_(RMS|PH)',name): return name.split('_')[1]+' 次谐波'+('电流有效值，单位 A' if name.endswith('RMS') else '相位给定，单位 rad')
    if name in ('Har_5OR7_PH0','Har_11_PH0'): return '原 '+name[4:].split('_')[0]+' 次谐波初始相位，单位 rad'
    m=re.fullmatch(r'(Ugrid|Iin|Iload)_(d|q|hp|har)_(Positive|Negative|d|q)(?:_last)?',name)
    if m:
        return dict(Ugrid='电网电压',Iin='端口电流',Iload='负载电流')[m[1]]+' '+m[2]+' 轴/分量的'+dict(Positive='正序基波',Negative='负序基波',d='d 轴谐波',q='q 轴谐波')[m[3]]+'值，单位 '+('V' if m[1]=='Ugrid' else 'A')+('；上一拍噪声比较缓存' if name.endswith('last') else '')
    m=re.fullmatch(r'(Id|Iq)_(Positive|Negative)_(Ref|Max|Min)',name)
    if m: return m[1]+' '+('正序' if m[2]=='Positive' else '负序')+dict(Ref='电流给定',Max='电流给定上限',Min='电流给定下限')[m[3]]+'，单位 A'
    if name.startswith('Uout_'): return '旧电流环 '+name[5:].replace('Positive','正序').replace('Negative','负序').replace('Max','上限').replace('Min','下限')+' 输出电压分量/限幅值，单位 V'
    if re.fullmatch(r'(Ud_st|Uq_st|Idst|Iqst)_(Max|Min)',name): return name.split('_')[0]+' 原控制给定'+('上限' if name.endswith('Max') else '下限')+'，电压 V、电流 A'
    if name in ('VA_Control','VB_Control','VC_Control'): return name[1]+' 相原输出电压控制给定，单位 V'
    if re.fullmatch(r'(HA|LA)_.*_Temp',name): return patterned(name[:-5])+'；本拍采样噪声比较缓存'
    if name.startswith(('Check_temp_','temp_')) and name[-1:] in 'ABC': return name[-1]+' 相模块电压巡检的位图/判定缓存'
    if name.startswith('FT3_SendData_RT'): return 'FT3 第 '+name[-1]+' 路遥测发送数据字'
    if name.startswith('FT3_RecData_'): return 'FT3 第 '+name[-1]+' 个接收数据字'
    if name.startswith('RT') and name.endswith('_EN'): return 'FT3 第 '+name[2]+' 路遥测发送使能位'
    if name.startswith('Driver') and name.endswith('_Status'): return '模块第 '+name[6]+' 路驱动状态位'
    if re.fullmatch(r'(FO_EN|FB_EN)_bit\d+',name): return name.split('_bit')[0]+' 第 '+name.split('_bit')[1]+' 路硬件故障输入使能位'
    return None

def meaning(symbol, data):
    name, owner=symbol['name'],symbol.get('owner','')
    if (owner,name) in LOCALS: return LOCALS[(owner,name)]
    if owner=='Chb_SourceEquivalentPiStep' and name=='state': return '输入/输出 PI 状态指针，保存积分项 integral'
    if owner=='Chb_SourceEquivalentPiStep' and name=='error': return '本拍给定减反馈的控制误差，单位取决于电压环或电流环'
    if owner=='SstFault_ReadInput' and name=='input' and symbol['kind']=='parameter': return '输出目标指针，本函数填写后台故障输入副本'
    if owner=='SstSerial_RouteByte' and name=='value': return '本轮 UART 收到的单字节，按当前协议分流状态解释'
    if owner.startswith('VOFA_Send') and symbol['kind']=='parameter':
        if name in 'abcdefghijkl' or re.fullmatch(r'f\d+',name): return '按函数参数顺序排列的浮点波形通道值，与原信号同单位'
    if owner.startswith('Chb_') and name in ('a','b','c'): return '三量比较的第 '+dict(a='一',b='二',c='三')[name]+' 个输入，与其余输入同单位'
    if name=='raw' and 'SST_FAULT_RECORD' in symbol['declaration']: return '后台采集的原控制故障、硬件锁存及观察时刻的完整副本'
    if owner=='_MAINT_FLOAT_WORDS': return '同一浮点位表示的'+('两个 16 位半字视图' if name=='word' else 'float 数值视图')
    if symbol['kind']=='parameter':
        if name in ('length','count'):
            if owner.startswith('MemDataCopy'): return '待复制的 16 位字数，源/目标容量均须满足'
            if owner=='H_Filter_Memcopy': return '固定复制四个浮点历史状态'
            if owner.startswith('VOFA_'): return '浮点通道数，最多 12'
            if owner.endswith(('Pack','ParseInstruct','ExecuteInstruct')): return '参考线上载荷字节数，每个 16 位元素只承载低 8 位'
            if owner.startswith(('Sst','Store','Maint','r_eeprom','CRC')): return '有效字节数；维护对象按其接口约定使用 16 位字数'
        if name=='p': return '已经绑定地址、缩放及范围的 SST_PARAMETER 映射条目'
        if name=='runindex' and owner=='FaultLatch_Clear': return '原故障锁存清除时序的执行/初始化选择值，沿用现有调用点'
        if name=='value' and owner in ('SstParams_Read','GetOneValueFromRealMem'): return '输出指针，用于返回固定地址对应的 16 位线上参数编码'
        if name=='value' and owner in ('SstParams_Write','SetOneValue2BothMem','WriteEntry'): return '待写入的 16 位线上参数编码，有符号量按二补码解释'
        if name in ('data','value') and owner.startswith('Chb_'): return '当前限幅/控制计算输入，与对应反馈或目标同单位'
        if name in PARAMETERS: return PARAMETERS[name]
        if name in VARIABLES: return VARIABLES[name]
    description=patterned(name,owner,symbol['declaration'])
    if description: return description
    old=old_comment(data,symbol)
    if old: return old
    if name=='i':
        return '本次字节/位循环序号' if owner.startswith(('r_iic','CRC','StoreCrc')) else '本次数组、参数或模块遍历的索引'
    if name in ('count','length'):
        return ('发送帧有效字节数' if 'SstDebug' in owner else '有效数据数量；字节缓存按字节、16 位对象复制按字计数')
    if name in ('result',): return '本次子操作返回值，成功/失败约定见被调用函数'
    if name in ('value','word'):
        return '本次协议/存储处理的值，类型和编码保持原接口约定'
    if name in ('data',): return '本次协议载荷或 EEPROM 镜像数据缓存，长度由当前接口限定'
    if name in ('index','offset','error','p','start','stop','on','off'):
        return dict(index='当前维护对象/参数实例索引',offset='当前对象字偏移或保存镜像字节偏移',error='本次维护操作的 MAINT_ERR_* 结果码',p='本次访问的 SST_PARAMETER 映射条目指针',start='旧遥控启动位图，停止/关脉冲优先',stop='旧遥控停止位图，停止优先于启动',on='旧脉冲使能位图，bit0 DAB、bit1 CHB',off='旧脉冲禁用位图，优先于同位使能')[name]
    if name in ('n',): return '本次有效数据数量或循环次数，按调用接口的长度单位解释'
    if name in ('a','b','c') and owner.startswith('Chb_'): return f'三量比较的输入 {name}，与其余输入同单位'
    if owner.startswith('VOFA_Send') and (name in 'abcdefghijkl' or re.fullmatch(r'f\d+',name)): return '按函数参数顺序排列的浮点波形通道值，与原信号同单位'
    if name in ('tempTrans','tempTrans1','tempTrans2','tempTrans3','temp_0','temp_1'):
        topics={'U_Decomposition_PLL':'电压坐标变换/PLL 相角','Iload_Decomposition':'负载电流正负序及谐波分离','P_Q_Calculator':'dq 电压电流乘积与功率滤波','SVG_Q_Reference':'无功目标及功率因数换算','SVG_I_Reference':'正负序电流参考合成','Port_AD_Sample':'采样累加与原滤波'}
        if owner in topics: return topics[owner]+'计算的浮点中间缓存 '+name
    if owner.startswith('GPIOBank') and name=='value': return '目标 GPIO 引脚的 PINMUX 读改写值，保留其他引脚字段'
    return None

def token_stream(text):
    # Preserve literals, operators, preprocessing directives and newlines there.
    pattern=r'("(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\')|/\*[\s\S]*?\*/|//[^\n]*'
    clean=re.sub(pattern,lambda m:m[1] or ' ',text)
    lexical = r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|\w+|[^\s\w]'
    return re.findall(lexical,clean), [re.sub(r'\s+',' ',line).strip() for line in clean.splitlines() if line.lstrip().startswith('#')]

def apply():
    unresolved=[]; results=[]; prepared=[]
    for path in source_files():
        rel=path.relative_to(ROOT).as_posix()
        baseline=ROOT/'build/comment_baseline'/rel
        raw, enc, data, root, symbols=collect(baseline)
        edits=defaultdict(list)
        nodes={(n.start_byte,n.end_byte):n for n in walk(root)}
        def text(n): return data[n.start_byte:n.end_byte].decode() if n else ''
        for s in symbols:
            if s['kind'] in ('function','prototype'):
                info=function_info(s['name'])
                node=nodes[(s['start'],s['end'])]
                params=[x for x in walk(node.child_by_field_name('declarator')) if x.type=='parameter_declaration']
                if s['kind']=='prototype':
                    params=[x for x in walk(node) if x.type=='parameter_declaration']
                inputs=[]
                for p in params:
                    pn=identifier(p.child_by_field_name('declarator'))
                    if pn:
                        ps=dict(kind='parameter',name=text(pn),owner=s['name'],declaration=text(p),start=p.start_byte,end=p.end_byte)
                        desc=meaning(ps,data)
                        if not desc: unresolved.append((rel,s['name'],text(pn),'parameter'))
                        inputs.append(text(pn)+'：'+(desc or '?'))
                indent=re.match(r'[ \t]*',data[data.rfind(b'\n',0,s['start'])+1:s['start']].decode())[0]
                header=['/*==================================================================',
                        ' * 函数功能: '+info['purpose'],
                        ' * 输入参数: '+(inputs[0] if inputs else '无')]
                header+=[' *           '+v for v in inputs[1:]]
                header+=[' * 输出参数: '+info.get('output','无指针输出；结果写入本模块状态或硬件寄存器。'),
                         ' * 返回值: '+info['result'],
                         ' * 执行说明: '+info.get('context',''),
                         ' *================================================================*/']
                prefix=('\r\n'+indent).join(header)+'\r\n'+indent
                insertion=s['start']
                # Keep the function banner ahead of its C-linkage qualifier.
                left=data[data.rfind(b'\n',0,insertion)+1:insertion]
                linkage=re.search(rb'extern\s+"C"\s*$',left)
                if linkage: insertion=data.rfind(b'\n',0,insertion)+1+linkage.start()
                edits[insertion].append(prefix.encode())
            elif s['kind'] not in ('parameter','enum'):
                desc=meaning(s,data)
                if not desc: unresolved.append((rel,s.get('owner',''),s['name'],s['kind']))
                else:
                    line_start=data.rfind(b'\n',0,s['start'])+1
                    lead=data[line_start:s['start']].decode()
                    indent=re.match(r'[ \t]*',lead)[0]
                    if lead.strip(): indent+='    '
                    # One named description per line, even for original multi-declarator statements.
                    join=' ' if not edits[s['end']] else '\r\n'+indent
                    edits[s['end']].append((join+'/* '+s['name']+'：'+desc+'。 */').encode())
            elif s['kind']=='enum':
                # Enumerated constants carry meanings shared with compile macros.
                desc=constant_meaning(s['name'],s['declaration'])
                if not desc: unresolved.append((rel,'',s['name'],'enum'))
                else: edits[s['start']].append(('/* '+desc+'。 */ ').encode())
        # Collect enum constants and #define symbols independently of declarations.
        for node in walk(root):
            if node.type not in ('preproc_def','preproc_function_def'): continue
            nn=node.child_by_field_name('name')
            if not nn: continue
            name=text(nn)
            if name.endswith(('_H_','_H','_HPP_','_HPP')) and not text(node.child_by_field_name('value')).strip(): continue
            if name.startswith('_') and 'H' in name and not text(node.child_by_field_name('value')).strip(): continue
            desc=constant_meaning(name,text(node))
            if not desc:
                sy=dict(start=node.start_byte,end=node.end_byte-1)
                desc=old_comment(data,sy)
            if not desc: unresolved.append((rel,'',name,'macro'))
            else: edits[node.start_byte].append(('/* '+name+'：'+desc+'。 */\r\n').encode())
        after=data
        for pos,chunks in sorted(edits.items(),reverse=True):
            after=after[:pos]+b''.join(chunks)+after[pos:]
        new=after.decode()
        assert token_stream(raw)==token_stream(new),rel+' executable tokens changed'
        # Improve originally compressed structs/locals by splitting only whitespace.
        new=re.sub(r'(\*/)[ \t]+(?=(?:(?:unsigned|short|long|const|volatile)\s+)*(?:float|bool|u16|s16|uint\d+_t|int\d+_t|void)\b)',r'\1\r\n    ',new)
        new=re.sub(r'(typedef struct)[ \t]+\{[ \t]+',r'\1\r\n{\r\n    ',new)
        new=re.sub(r'(\*/)[ \t]+\}[ \t]+(ST_[A-Z_]+|SST_PARAMETER|SST_TX_FRAME);',r'\1\r\n} \2;',new)
        assert token_stream(raw)==token_stream(new),rel+' formatting changed executable tokens'
        prepared.append((path,new,enc))
        results.append(dict(file=rel,symbols=len(symbols),functions=sum(s['kind']=='function' for s in symbols),
                            baseline_sha256=hashlib.sha256(baseline.read_bytes()).hexdigest(),tokens_unchanged=True))
    report=ROOT/'build/comment_inventory/unresolved.json'
    report.write_text(json.dumps(unresolved,ensure_ascii=False,indent=2),encoding='utf-8')
    if unresolved:
        print('Unresolved symbols:',len(unresolved))
        for item in unresolved[:250]: print(' | '.join(item))
        return 1
    for path,new,enc in prepared:
        # Write bytes directly, preserving original newline sequences and BOM.
        bom=path.read_bytes().startswith(b'\xef\xbb\xbf')
        encoded=new.encode('utf-8-sig' if bom else 'utf-8')
        if path.read_bytes()!=encoded:
            path.write_bytes(encoded)
    (ROOT/'build/comment_inventory/coverage.json').write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
    print('Annotated',len(prepared),'files;',sum(r['symbols']for r in results),'symbols; all executable tokens unchanged.')
    return 0

def constant_meaning(name,declaration):
    if name in CONSTANTS: return CONSTANTS[name]
    if name in VARIABLES: return VARIABLES[name]
    if name.endswith('_def'):
        return (patterned(name[:-4]) or '原 '+name[:-4]+' 控制系数')+'的上电默认值'
    if re.fullmatch(r'H(2|6|12)_(20|100)_Filter_(Num|Den)_[012]',name): return name.split('_')[0]+'/'+name.split('_')[1]+' 原二阶滤波器'+('分子' if '_Num_' in name else '分母')+'第 '+name[-1]+' 项差分系数'
    if re.fullmatch(r'H(2|6|12)_(20|100)_Filter_Gain',name): return '原 '+name[:-5]+' 滤波输出增益'
    if name.startswith('E_'):
        key=name[2:].replace('_Inst','').replace('_Rply','')
        desc=INSTRUCTIONS.get(key)
        return '参考 '+desc+('请求命令码' if name.endswith('Inst') else '响应命令码') if desc else None
    if name.startswith('SST_FAULT_'):
        return FAULT_CONSTANTS.get(name[10:])
    if name.startswith('SST_PARAM_'):
        return '参数服务'+PARAM_RESULTS[name[10:]] if name[10:] in PARAM_RESULTS else None
    m=re.fullmatch(r'CHB_PM_([ABC][12])_MASK',name)
    if m: return m[1]+' 组在线模块位图，决定模块数及参考尺度'
    if name.startswith('CHB_STATE_'): return 'CHB/DAB 状态机的'+STATE_CONSTANTS[name[10:]]+'状态编号'
    if name.startswith('CHB_FAULT_'): return '控制核心'+CONTROL_FAULTS[name[10:]]+'故障编号'
    if name.startswith('GPIO_'): return '板级 GPIO 引脚复用寄存器的目标字段掩码/偏移，保留原硬件定义'
    if name.startswith('Module_') and name.endswith('_Mask'): return '原模块故障状态字的 '+name[7:-5]+' 位掩码，保持 FPGA 状态布局'
    if name.startswith(('PINMUX','SYSCFG_PINMUX')): return 'EMIFA/UART 引脚复用位掩码及目标值，沿用 C6748 板级连接'
    if 'PluginUnit_' in name: return '原 '+name.split('_PluginUnit')[0]+' 插件硬件编号，寄存器数组使用编号减 2'
    if name.startswith('WriteBlock_'): return 'DPRAM '+name[11:]+' 回写数据块标识，保持原协议编号'
    if name.startswith('ReadBlock_'): return 'DPRAM '+name[10:]+' 读取数据块标识，保持原协议编号'
    if name.startswith('DPRAMBlock') and name.endswith('_Length'): return '对应 DPRAM 块的连续 16 位字数，保持原布局'
    if name.startswith('ModuleIndex_'): return '模块采样类型 '+name[12:].replace('DCbus_Volt','直流电压').replace('DCbus_Current','直流电流').replace('Port_Current','端口电流').replace('Temp','温度')+' 的索引编号'
    if name.startswith('PortStatusNum_'): return '原端口状态机 '+name[14:]+' 状态编号，保持兼容数值'
    if name.startswith('VOFA_'): return VOFA_CONSTANTS.get(name[5:])
    if name.endswith('_Ratio'): return '原 '+name[:-6]+' 采样量的传感器/工程量换算比例'
    if name in ('ushort','Uint16'): return '旧代码无符号 16 位整数类型别名'
    if name in ('uchar','Uint8'): return '旧代码无符号 8 位整数类型别名'
    return None

CONSTANTS=dict(row.split('|',1) for row in '''
SST_DEBUG_PROTOCOL_ANYEYES|参考 PcDebug/AnyEyes 协议选择值 1
SST_DEBUG_PROTOCOL_VOFA|VOFA JustFloat 协议选择值 2
SST_DEBUG_PROTOCOL|串口协议二选一配置，只允许 ANYEYES 或 VOFA
SST_ENABLE_DPRAM|DPRAM 硬件访问及通知中断使能，0 禁用、1 启用
SST_SIMULATOR_MODE|仿真器控制模式，1 禁止普通串口/DPRAM 远程启动，急停仍有效
SST_ALLOW_REMOTE_COMMANDS|由仿真器模式与 DPRAM 配置派生的遥控权限
SST_ALLOW_SERIAL_COMMANDS|由仿真器模式派生的普通串口命令权限
SST_UART_BAUD|UART2 调试通信波特率，单位 bit/s
SST_UART_INPUT_CLOCK_HZ|UART2 输入时钟，单位 Hz，需与板级 SYSCLK 一致
SST_TELEMETRY_PERIOD_10MS|VOFA 周期上报间隔，单位 10 ms
CHB_CONTROL_MODE_SIMULATOR|旧接口的仿真器模式编号
CHB_CONTROL_MODE_DPRAM|旧接口的 DPRAM 模式编号
CHB_CONTROL_MODE_485|旧接口的串口 485 模式编号
CHB_CONTROL_MODE|由新配置派生的旧控制模式兼容值
CHB_LOAD_EEPROM_PARAMETERS|上电加载持久化参数使能，0 使用源码默认、1 尝试 EEPROM 恢复
CHB_OPEN_LOOP_PULSE_TEST|原开环脉冲测试兼容开关
SST_TX_SLOTS|共享 UART 发送队列容量，8 个整帧槽
SST_FRAME_MAX|共享 UART 单帧最大字节数，260 字节
SST_RX_BUDGET|每次后台调用最多处理的 UART 接收字节数，64 字节
SST_TX_BUDGET|每次 TX 服务最多送入 FIFO 的字节数，16 字节
SST_STORE_LENGTH|页 6/7 参数镜像总长度，92 字节
SST_DISPLAY_COUNT|参考波形/面板显示表容量，200 个 16 位字
SST_READ_U16|从旧非 volatile 变量按 volatile 16 位左值读取，不改变旧 ISR 声明
SST_READ_U8|从旧非 volatile 变量按 volatile 8 位左值读取，不改变旧 ISR 声明
CHB_PI|π 浮点常量
CHB_TWO_PI|2π 浮点常量，用于角度周期归一化
CHB_ONE_BY_SQRT3|1/√3 浮点常量，用于 Clarke 变换
CHB_SQRT3_BY_TWO|√3/2 浮点常量，用于三相逆变换
CHB_REFERENCE_MIN|原 FPGA 工频桥归一化参考下限 0.02
CHB_REFERENCE_MAX|原 FPGA 工频桥归一化参考上限 1.98
CHB_AC_MODE_OPEN_LOOP|工频桥开环模式编号 0
CHB_AC_MODE_CLOSED_LOOP|工频桥闭环模式编号 1
CHB_STARTUP_SOURCE_COMPATIBLE|原闭环启动兼容策略编号 0
CHB_STARTUP_ID_RAMP|先固定 Id 斜坡再移交电压环策略编号 1
CHB_STARTUP_IDLE|闭环启动尚未进行阶段编号
CHB_STARTUP_ID_RAMP_PHASE|闭环固定 Id 斜坡阶段编号
CHB_STARTUP_VOLTAGE_HANDOVER|电压环给定平滑移交阶段编号
CHB_STARTUP_NORMAL|闭环正常电压环运行阶段编号
FT3_PTCT|PT/CT FT3 插件原 EMIFA 地址
FT3_FaultRecorder|故障录波 FT3 插件原 EMIFA 地址
FT3_EER2|EER2 FT3 插件原 EMIFA 地址
FT3_Backup|备用 FT3 插件原 EMIFA 地址
DPRAM_Maibox_Left|DPRAM 左邮箱原地址；读访问会应答对端通知
DPRAM_Maibox_Right|DPRAM 右邮箱原地址；写访问用于通知对端
SDAO|把软件 I²C SDA 配置为输出
SDAI|把软件 I²C SDA 配置为输入
SDAH|软件 I²C SDA 输出高电平
SDAL|软件 I²C SDA 输出低电平
SCLH|软件 I²C SCL 输出高电平
SCLL|软件 I²C SCL 输出低电平
ACK|读取软件 I²C SDA 的应答电平
SDA|读取软件 I²C SDA 当前电平
EEPROM_ADDRESS|EEPROM 软件 I²C 器件寻址字节，保持原器件定义
CONTROL_ADDRESS|EEPROM/RTC 控制区软件 I²C 器件寻址字节
CONTROL_ADDRESS_HIGH|EEPROM/RTC 控制寄存器地址高字节
CONTROL_ADDRESS_LOW|EEPROM/RTC 控制寄存器地址低字节
TIME_ADDRESS|RTC 时间区软件 I²C 器件寻址字节
TIME_ADDRESS_HIGH|RTC 时间寄存器地址高字节
TIME_ADDRESS_LOW|RTC 时间寄存器地址低字节
VALIDFLAG|原 EEPROM/I²C 操作成功标志值
INVALIDFLAG|原 EEPROM/I²C 操作失败标志值
Page_ADC_Zero|旧 AD 零点校准 EEPROM 页号
Page_CAL_Para|旧计算参数 EEPROM 页号
Page_CTL_Set|旧控制配置 EEPROM 页号
Page_LOG_A|旧记录 A 的 EEPROM 页号
Page_LOG_B|旧记录 B 的 EEPROM 页号
Page_LOG_C|旧记录 C 的 EEPROM 页号
Page_LOG_D|旧记录 D 的 EEPROM 页号
Addr_Udc_ref|旧 EEPROM 直流电压给定字节偏移
Addr_Q_ref|旧 EEPROM 无功功率给定字节偏移
Addr_IQ_ref|旧 EEPROM q 轴电流给定字节偏移
Addr_Adj_Phi|旧 EEPROM 相位 Phi 调整参数字节偏移
Addr_Adj_Theta|旧 EEPROM 相位 Theta 调整参数字节偏移
Addr_Modulation|旧 EEPROM 调制比参数字节偏移
Addr_Debug_Rec|旧 EEPROM 调试记录参数字节偏移
TMR_PERIOD_LSB32|Timer2 周期计数低 32 位，按原时钟对应 10 ms
TMR_PERIOD_MSB32|Timer2 周期计数高 32 位
SYSCLK_1_FREQ|板级 SYSCLK1 频率，单位 Hz
SYSCLK_2_FREQ|板级 SYSCLK2 频率，单位 Hz
UART_2_FREQ|板级 UART2 时钟频率，单位 Hz
ReadController|旧维护协议控制器读取功能码
WriteController|旧维护协议控制器写入功能码
ControllerAddr|旧维护协议本控制器地址
ValidAddr_CS2_S|旧维护 FPGA CS2 合法地址下界
ValidAddr_CS2_E|旧维护 FPGA CS2 合法地址上界
ValidAddr_CS5_S|旧维护 DPRAM CS5 合法地址下界
ValidAddr_CS5_E|旧维护 DPRAM CS5 合法地址上界
ALL_LIST_NUM|参考功能码组数量，保持固定 30 组
SINGLE_LIST_NUM|参考每组参数数量，保持固定 34 字
HVAC_online|选择高压交流端口的旧代码条件编译分支
PORT_Index|旧端口编号，HVAC 工程为 1
ControlTAD|原 FPGA PWM/TAD 周期计数，保持 5000
SwitchTotalNum|原开关动作表条目数量
OV_Stop|原过压停机策略条件编译开关
EEPROM_ON|原 EEPROM 兼容访问开关，受上电加载配置控制
FaultOutTest|原故障输出测试编译标识
ReferenceDEF|旧 FPGA 模块参考基数，乘在线模块数得到相基值
TAD_PLL|原 PLL 控制周期，0.0001 s，即 10 kHz
LV_TotalTime|原欠压处理总延时，单位原 10 ms 节拍
LV_Time_1|原欠压处理第一阶段时间，单位原 10 ms 节拍
LV_Time_2|原欠压处理第二阶段时间，单位原 10 ms 节拍
Vslope|原欠压处理电压给定斜坡步长
SVG_1|原 SVG 控制算法条件编译选择
SimHVAC|原 HVAC 仿真/测试兼容编译标识
SVG_10KV|原 10 kV 参数方案编译选择
COM_TEST_PluginUnit|原插件通信测试编译选择
PhaseComValue|原电流/锁相变换相位补偿默认值，单位 rad
SoftwarePhaseComValue|原输出变换软件相位补偿默认值，单位 rad
WL_decouple|原 dq 电流控制交叉耦合补偿系数
PTCT_FT3|PT/CT FT3 插件选择编号
FaultSwitch_FT3|故障录波 FT3 插件选择编号
EER2_FT3|EER2 FT3 插件选择编号
Backup_FT3|备用 FT3 插件选择编号
OV_allport_shutdown|原过压时全端口关断策略开关
Sample_UP_20kHz|原 20 kHz 分支采样上沿判定常量，保留兼容定义
Sample_DOWN_20kHz|原 20 kHz 分支采样下沿判定常量，保留兼容定义
SigUst_LVAC_Port|旧 LVAC 启动电压判据，单位 V
UstOK_LVAC_Port|旧 LVAC 电压达标参考，单位 V
Ust_LVAC_Port|旧 LVAC 运行电压参考，单位 V
flux_k1|原磁通估算/滤波差分系数 k1
flux_k2|原磁通估算/滤波差分系数 k2
'''.strip().splitlines())
INSTRUCTIONS=dict(ReadFuncCode='单参数读取',WriteFuncCode='单参数写入',ScopeRun='波形开始',ScopeStop='波形停止',StateRun='状态面板开始',StateStop='状态面板停止',ReadAllFuncCode='全部参数读取保留',WriteAllFuncCode='全部参数写入保留',RunStopCmd='运行/停止',ReadErr='故障查询保留',ReadVersion='版本查询保留',ReqReBoot='复位保留',ReqEmrgStop='紧急停止',PC_LIFE='PC 心跳',MCU_LIFE='MCU 心跳',PC_LINK='连接握手')
FAULT_CONSTANTS=dict(row.split('|',1) for row in '''
LEVEL_1|参考一级故障等级编码 0
LEVEL_2|参考二级故障等级编码 1
LEVEL_3|参考三级故障等级编码 2
LEVEL_4|参考四级故障等级编码 3
LEVEL_NONE|参考无故障等级编码 4
CURRENT_CAPACITY|当前故障表容量 32
HISTORY_CAPACITY|详细 RAM 历史容量 32
LEGACY_HISTORY_WORDS|参考组 19 的历史字数 34，字 0 为写位置
NONE|无上报故障码 0
HW_UNCLASSIFIED|未能细分的硬件故障五位上报码
HW_PORT_OC|端口硬件过流五位上报码
HW_PORT_OV|端口硬件过压五位上报码
HW_MODULE|模块汇总故障五位上报码
HW_MODULE_OC|模块过流五位上报码
HW_MODULE_OV|模块过压五位上报码
HW_MODULE_OT|模块过温五位上报码
HW_CHB_BRIDGE|模块工频 CHB 桥故障五位上报码
HW_DAB_BRIDGE|模块 DAB 桥故障五位上报码
HW_MODULE_SINGLE|单模块故障五位上报码
HW_MODULE_UV|模块欠压五位上报码
HW_MODULE_OV2|模块二级过压五位上报码
HW_FAN|风扇故障五位上报码
HW_CONTROLLER|控制器故障五位上报码
HW_AUX_SUPPLY|辅助电源故障五位上报码
HW_EPO|紧急关断输入故障五位上报码
PRECHARGE_TIMEOUT|预充超时五位上报码，沿用参考 20103
PLL_TIMEOUT|PLL 锁定超时五位上报码
SW_AC_OC|软件交流过流五位上报码
SW_DC_OV|软件直流过压五位上报码
BAD_CONFIGURATION|控制配置非法五位上报码
CONTROL_RUNTIME|运行时采样或控制计算失效五位上报码
PHASE_SEQUENCE|相序异常五位上报码
FREQUENCY|频率异常五位上报码
CONTACTOR|接触器故障五位上报码
BREAKER|断路器故障五位上报码
COMMUNICATION|通信故障五位上报码
SYNC|同步故障五位上报码
'''.strip().splitlines())
PARAM_RESULTS=dict(OK='成功结果码',BAD_ADDRESS='地址非法结果码',READ_ONLY='只读地址拒绝结果码',BUSY='忙或不满足停机条件结果码',RANGE='数值越界结果码',DENIED='编译权限拒绝结果码',STORAGE='存储校验/读写失败结果码',GROUPS='固定参数组数，参考协议为 30',PER_GROUP='每组固定参数字数，参考协议为 34')
STATE_CONSTANTS=dict(STOPPED='停止',PRECHARGE='预充',BYPASS_DELAY='旁路等待',WAIT_PLL='等待 PLL',RUNNING='运行',FAULT='故障')
CONTROL_FAULTS=dict(NONE='无',PRECHARGE_TIMEOUT='预充超时',PLL_TIMEOUT='PLL 超时',AC_OVERCURRENT='交流过流',DC_OVERVOLTAGE='直流过压',HARDWARE='硬件',BAD_CONFIGURATION='配置非法',SWITCH_FEEDBACK='开关反馈',CONTROL_RUNTIME='运行时控制失效')
VOFA_CONSTANTS=dict(RX_BUFFER_SIZE='旧 VOFA 接收缓存容量兼容定义',TX_BUFFER_SIZE='旧 VOFA 发送缓存容量兼容定义')

if __name__=='__main__': raise SystemExit(apply())
