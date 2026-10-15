/* 文件职责: 模块接口。数据只在所属 .c 定义，外部通过本头的必要声明访问。 */
#ifndef SSTTYPES_H_
#define SSTTYPES_H_

#ifdef __cplusplus
extern "C"
{
#endif
/* ushort：旧代码无符号 16 位整数类型别名。 */
#define ushort unsigned short
/* uchar：旧代码无符号 8 位整数类型别名。 */
#define uchar unsigned char
/* Uint16：旧代码无符号 16 位整数类型别名。 */
#define Uint16 unsigned short
/* Uint8：旧代码无符号 8 位整数类型别名。 */
#define Uint8 unsigned char

#ifdef __cplusplus
}
#endif
#endif
