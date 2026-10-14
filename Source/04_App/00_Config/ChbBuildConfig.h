#ifndef CHB_BUILD_CONFIG_H_
#define CHB_BUILD_CONFIG_H_

/* Override these choices with compiler --define, or edit this file. */
/* SST_DEBUG_PROTOCOL_ANYEYES：参考 PcDebug/AnyEyes 协议选择值 1。 */
#define SST_DEBUG_PROTOCOL_ANYEYES 1U
/* SST_DEBUG_PROTOCOL_VOFA：VOFA JustFloat 协议选择值 2。 */
#define SST_DEBUG_PROTOCOL_VOFA    2U
#ifndef SST_DEBUG_PROTOCOL
/* SST_DEBUG_PROTOCOL：串口协议二选一配置，只允许 ANYEYES 或 VOFA。 */
#define SST_DEBUG_PROTOCOL SST_DEBUG_PROTOCOL_ANYEYES
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
/* SST_BENCH_INPUT_SIM：临时无主回路输入注入，默认0；1生成测试采样，不替代功率回路模型。 */
#define SST_BENCH_INPUT_SIM 1U
#endif
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

/* Legacy constants kept for shared source compatibility. */
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
#if SST_BENCH_INPUT_SIM && (!SST_SIMULATOR_MODE || SST_ENABLE_DPRAM || CHB_LOAD_EEPROM_PARAMETERS)
#error "Bench input simulation requires simulator=1, DPRAM=0 and EEPROM boot load=0"
#endif
#if SST_TELEMETRY_PERIOD_10MS < 1U
#error "Telemetry period must be at least 10ms"
#endif
#endif
