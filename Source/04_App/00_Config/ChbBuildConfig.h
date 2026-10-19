#ifndef CHB_BUILD_CONFIG_H_
#define CHB_BUILD_CONFIG_H_

/* 编译器 --define 可覆盖未预先定义的配置项；也可在本文件集中配置。 */
/* SST_DEBUG_PROTOCOL_ANYEYES：参考 PcDebug/AnyEyes 协议选择值 1。 */
#define SST_DEBUG_PROTOCOL_ANYEYES 1U
/* SST_DEBUG_PROTOCOL_VOFA：VOFA JustFloat 协议选择值 2。 */
#define SST_DEBUG_PROTOCOL_VOFA 2U
#ifndef SST_DEBUG_PROTOCOL
/* SST_DEBUG_PROTOCOL：串口协议二选一配置，只允许 ANYEYES 或 VOFA。 */
#define SST_DEBUG_PROTOCOL SST_DEBUG_PROTOCOL_VOFA
#endif
#ifndef SST_ENABLE_DPRAM
/* SST_ENABLE_DPRAM：DPRAM硬件及并口维护使能；当前单UNIT调试默认0。 */
#define SST_ENABLE_DPRAM 0U
#endif
#ifndef SST_SIMULATOR_MODE
/* SST_SIMULATOR_MODE：当前默认1由仿真器控制；0允许所选通信源远控，急停仍有效。 */
#define SST_SIMULATOR_MODE 1U
#endif
#ifndef SST_BENCH_INPUT_SIM
/* SST_BENCH_INPUT_SIM：临时无主回路输入注入；0 使用真实采样，1 生成测试采样。
 * 修改后需重新编译；本功能不替代功率回路模型。 */
#define SST_BENCH_INPUT_SIM 0U
#endif
#ifndef SST_PHASE_A_ONLY_TEST
/* SST_PHASE_A_ONLY_TEST：仅 A 相 AN 采样与虚拟 dq 双闭环测试；1 启用双 SOGI 并封锁 B/C，0 保留三相算法。 */
#define SST_PHASE_A_ONLY_TEST 1U
#endif
#ifndef SST_LOW_VOLTAGE_100V_TEST
/* SST_LOW_VOLTAGE_100V_TEST：默认随单相宏选择 100 Vac 参数；单相宏为 0 时默认恢复高压三相参数。
 * 可显式指定 0 选择高压单相；显式指定 1 只允许用于单相试验。 */
#define SST_LOW_VOLTAGE_100V_TEST SST_PHASE_A_ONLY_TEST
#endif

/* 100 Vac/6 模块低压台架参数。数值单位为物理量单位，供初始化和 DPRAM 边界校验共用。 */
/* SST_LV100_MODULE_REF_UDC_V：低压试验每个 CHB 模块的直流电压目标，单位 V。 */
#define SST_LV100_MODULE_REF_UDC_V 35.0
/* SST_LV100_PRECHARGE_V：低压试验每个模块允许进入旁路的最低预充电压，单位 V。 */
#define SST_LV100_PRECHARGE_V 20.0
/* SST_LV100_MODULE_OV_V：低压试验相模块均压的快速过压阈值，单位 V/模块；
 * 不是逐模块最大电压保护，不能据此判断每个子模块均未过压。 */
#define SST_LV100_MODULE_OV_V 42.0
/* SST_LV100_PORT_V_MIN：低压试验上位机可写单模块电压给定下限，单位 V。 */
#define SST_LV100_PORT_V_MIN 20.0
/* SST_LV100_PORT_V_MAX：低压试验上位机可写模块均压给定上限，单位 V；距 42 V 保护留有裕量。 */
#define SST_LV100_PORT_V_MAX 38.0
/* SST_LV100_PORT_V_DEFAULT：低压试验通信/仿真命令的默认单模块电压给定，单位 V。 */
#define SST_LV100_PORT_V_DEFAULT 35.0
/* SST_LV100_INPUT_PRESENT_V：低压试验判定 AN 交流输入已建立的有效值门槛，单位 V RMS。 */
#define SST_LV100_INPUT_PRESENT_V 50.0
/* SST_LV100_INPUT_LEVEL1_V：低压试验交流输入一级状态判断门槛，单位 V。 */
#define SST_LV100_INPUT_LEVEL1_V 30.0
/* SST_LV100_INPUT_LEVEL2_V：低压试验交流输入二级状态判断门槛，单位 V。 */
#define SST_LV100_INPUT_LEVEL2_V 90.0
/* SST_LV100_CURRENT_MIN_A：低压试验通信参数允许的电流给定下限，单位 A。 */
#define SST_LV100_CURRENT_MIN_A 1.0
/* SST_LV100_CURRENT_MAX_A：低压试验快速过流及电流给定上限，单位 A。 */
#define SST_LV100_CURRENT_MAX_A 3.0
/* SST_LV100_CURRENT_DIRECTION_HYST_A：低压 A 相的 501 排序电流方向滞环，单位 A；不是过流保护阈值。
 * Avg_IG/10 的分辨率为 0.1 A；严格越过 ±0.1 A（即达到 ±0.2 A）才切换，边界及区间内保持原方向。
 * 此阈值用于小电流试验，需在现场核验采样噪声后再确定。高压及 B/C 相仍沿用 ±1 A 判据。 */
#define SST_LV100_CURRENT_DIRECTION_HYST_A 0.1F
/* SST_LV100_UST_ADD_V：低压启动阶段电压给定的附加步进，单位 V。 */
#define SST_LV100_UST_ADD_V 3.0
/* SST_ALLOW_REMOTE_COMMANDS：由仿真器模式与 DPRAM 配置派生的遥控权限。 */
#define SST_ALLOW_REMOTE_COMMANDS (!SST_SIMULATOR_MODE)
/* SST_ALLOW_SERIAL_COMMANDS：由仿真器模式派生的普通串口命令权限。 */
#define SST_ALLOW_SERIAL_COMMANDS (SST_ALLOW_REMOTE_COMMANDS && !SST_ENABLE_DPRAM)
#ifndef SST_UART_BAUD
/* SST_UART_BAUD：UART2 调试通信波特率，单位 bit/s。 */
#define SST_UART_BAUD 115200U
#endif
#ifndef SST_UART_INPUT_CLOCK_HZ
/* SST_UART_INPUT_CLOCK_HZ：UART2 输入时钟，单位 Hz，需与板级 SYSCLK 一致。 */
#define SST_UART_INPUT_CLOCK_HZ 228000000U
#endif
#ifndef SST_TELEMETRY_PERIOD_10MS
/* SST_TELEMETRY_PERIOD_10MS：VOFA 周期上报间隔，单位 10 ms。 */
#define SST_TELEMETRY_PERIOD_10MS 2U
#endif

/* 保留旧控制模式常量，供公共源码兼容使用。 */
/* CHB_CONTROL_MODE_SIMULATOR：旧接口的仿真器模式编号。 */
#define CHB_CONTROL_MODE_SIMULATOR 0U
/* CHB_CONTROL_MODE_DPRAM：旧接口的 DPRAM 模式编号。 */
#define CHB_CONTROL_MODE_DPRAM 1U
/* CHB_CONTROL_MODE_485：旧接口的串口 485 模式编号。 */
#define CHB_CONTROL_MODE_485 2U
#ifndef CHB_CONTROL_MODE
#if SST_SIMULATOR_MODE
/* CHB_CONTROL_MODE：由新配置派生的旧控制模式兼容值。 */
#define CHB_CONTROL_MODE CHB_CONTROL_MODE_SIMULATOR
#elif SST_ENABLE_DPRAM
/* CHB_CONTROL_MODE：由新配置派生的旧控制模式兼容值。 */
#define CHB_CONTROL_MODE CHB_CONTROL_MODE_DPRAM
#else
/* CHB_CONTROL_MODE：由新配置派生的旧控制模式兼容值。 */
#define CHB_CONTROL_MODE CHB_CONTROL_MODE_485
#endif
#endif
#ifndef CHB_LOAD_EEPROM_PARAMETERS
/* CHB_LOAD_EEPROM_PARAMETERS：上电加载持久化参数使能，0 使用源码默认、1 尝试 EEPROM 恢复。 */
#define CHB_LOAD_EEPROM_PARAMETERS 0U
#endif
#ifndef CHB_OPEN_LOOP_PULSE_TEST
/* CHB_OPEN_LOOP_PULSE_TEST：原开环脉冲测试兼容开关。 */
#define CHB_OPEN_LOOP_PULSE_TEST 0U
#endif
#if CHB_OPEN_LOOP_PULSE_TEST
#error "Use CHB_AC_MODE_OPEN_LOOP; legacy EMIF bridge is not supported"
#endif
#if SST_DEBUG_PROTOCOL != SST_DEBUG_PROTOCOL_ANYEYES && SST_DEBUG_PROTOCOL != SST_DEBUG_PROTOCOL_VOFA
#error "Select ANYEYES(1) or VOFA(2)"
#endif
#if (SST_ENABLE_DPRAM > 1U) || (SST_SIMULATOR_MODE > 1U)
#error "SST boolean switches must be 0 or 1"
#endif
#if SST_BENCH_INPUT_SIM > 1U
#error "SST_BENCH_INPUT_SIM must be 0 or 1"
#endif
#if SST_PHASE_A_ONLY_TEST > 1U
#error "SST_PHASE_A_ONLY_TEST must be 0 or 1"
#endif
#if SST_LOW_VOLTAGE_100V_TEST > 1U
#error "SST_LOW_VOLTAGE_100V_TEST must be 0 or 1"
#endif
#if SST_LOW_VOLTAGE_100V_TEST && !SST_PHASE_A_ONLY_TEST
#error "100Vac low-voltage profile requires SST_PHASE_A_ONLY_TEST=1"
#endif
#if SST_BENCH_INPUT_SIM && (!SST_SIMULATOR_MODE || SST_ENABLE_DPRAM || CHB_LOAD_EEPROM_PARAMETERS)
#error "Bench input simulation requires simulator=1, DPRAM=0 and EEPROM boot load=0"
#endif
#if SST_TELEMETRY_PERIOD_10MS < 1U
#error "Telemetry period must be at least 10ms"
#endif
#endif
