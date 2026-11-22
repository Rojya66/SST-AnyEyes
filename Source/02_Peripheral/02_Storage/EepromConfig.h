//###########################################################################
// FILE: EepromConfig.h
// FUNCTION: BL24C512A硬件连接及容量；参数镜像地址不随物理页大小改变。
//###########################################################################
#ifndef SST_EEPROM_CONFIG_H
#define SST_EEPROM_CONFIG_H

/* A2:A0的接线值，范围0..7。全接地时7位地址50h，线上写/读字节A0h/A1h。 */
/* 现场已确认：接DSP SDA/SCL，A2/A1/A0及WP均接地；WP不由软件切换。 */
#ifndef SST_EEPROM_ADDRESS_PINS
#define SST_EEPROM_ADDRESS_PINS 0U
#endif
#if SST_EEPROM_ADDRESS_PINS > 7U
#error "BL24C512A address pins must be 0..7"
#endif

/* 沿用DSP软件I2C：库编号9/10对应GPIO0[8]/GPIO0[9]，须接EEPROM SDA/SCL。 */
#define SST_EEPROM_SDA_PIN 9U
/* SST_EEPROM_SCL_PIN：GPIO0[9]的库编号10，软件I2C时钟。 */
#define SST_EEPROM_SCL_PIN 10U
/* SST_EEPROM_DEVICE_WRITE：含R/W=0的线上器件寻址字节。 */
#define SST_EEPROM_DEVICE_WRITE (0xA0U | (SST_EEPROM_ADDRESS_PINS << 1U))
/* SST_EEPROM_CAPACITY_BYTES：512Kbit换算为65536字节。 */
#define SST_EEPROM_CAPACITY_BYTES 65536UL
/* SST_EEPROM_PHYSICAL_PAGE_BYTES：器件一次页写不能跨越的128字节边界。 */
#define SST_EEPROM_PHYSICAL_PAGE_BYTES 128U
/* 旧页号接口保持64字节逻辑页：页6仍是0180h，不改成0300h。 */
#define SST_EEPROM_LOGICAL_PAGE_BYTES 64U
/* 每个SCL高/低阶段至少4us；写忙最多轮询64次，期间不关闭任何中断。 */
#define SST_EEPROM_HALF_CLOCK_US 4U
/* SST_EEPROM_READY_POLLS：同步段写的最大ACK轮询次数，覆盖3ms写周期。 */
#define SST_EEPROM_READY_POLLS 64U

#endif
