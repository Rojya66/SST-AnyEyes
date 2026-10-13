/****************************************************************************/
/*                                                                          */
/*              OMAPL138 及 DSP C6748 内存空间分配定义                      */
/*                                                                          */
/*              2014年05月16日                                               */
/*                                                                          */
/****************************************************************************/

MEMORY
{
#ifdef DSP_CORE
/****************************************************************************/
/*                                                                          */
/*              DSP 专有内存区域                                            */
/*                                                                          */
/****************************************************************************/
    DSPL2ROM     o = 0x00700000  l = 0x00100000  /* 1MB   L2 DSP 本地 ROM (DSP ROM Bootloader) */
    DSPL2RAM     o = 0x00800000  l = 0x00040000  /* 256kB L2 DSP 本地 RAM */
    DSPL1PRAM    o = 0x00E00000  l = 0x00008000  /* 32kB  L1 DSP 本地程序 RAM */
    DSPL1DRAM    o = 0x00F00000  l = 0x00008000  /* 32kB  L1 DSP 本地数据 RAM */
#endif

/****************************************************************************/
/*                                                                          */
/*              公共内存区域                                                */
/*                                                                          */
/****************************************************************************/
    SHDSPL2ROM   o = 0x11700000  l = 0x00100000  /* 1MB   L2 共享内置 ROM */
//  SHDSPL2RAM   o = 0x11800000  l = 0x00040000  /* 256KB L2 共享内置 RAM */
    SHDSPL2RAMs1 o = 0x11800000  l = 0x00001000  /* 256KB L2 共享内置 RAM */
    SHDSPL2RAM   o = 0x11801000  l = 0x0003F000  /* 256KB L2 共享内置 RAM */

    SHDSPL1PRAM  o = 0x11E00000  l = 0x00008000  /* 32KB  L1 共享内置程序 RAM */
    SHDSPL1DRAM  o = 0x11F00000  l = 0x00008000  /* 32KB  L1 共享内置数据 RAM */


    EMIFACS0     o = 0x40000000  l = 0x20000000  /* 512MB SDRAM (CS0) */
    
//  EMIFACS2     o = 0x60000000  l = 0x02000000  /* 32MB  异步          (CS2) */

//  FPGA_SPACE     o = 0x60000000  l = 0x2000
//  EMIFACS2BACK   o = 0x60002000  l = 0x01ffe000

    FPGA_SPACE     o = 0x60000000  l = 0x1E00

	FT3_PTCT_Register_U17          o = 0x60001E00  l = 0x0040
	FT3_FaultSwitch_Register_U17   o = 0x60001E40  l = 0x0040
	FT3_EER2_Register_U17          o = 0x60001E80  l = 0x0040
	FT3_Backup_Register_U17        o = 0x60001EC0  l = 0x0040
    
//    EMIFACS2BACK   o = 0x60001F00  l = 0x01ffe100

//    FPGA_SPACE     o = 0x60000000  l = 0x00002000
/*
    FPGA_Plug_1    o = 0x60000100  l = 0x00000100
    FPGA_SPACE_2   o = 0x60000200  l = 0x00000100
    FPGA_Plug_2    o = 0x60000300  l = 0x00000100    
    FPGA_SPACE_3   o = 0x60000400  l = 0x00000100
    FPGA_Plug_3    o = 0x60000500  l = 0x00000100  
    FPGA_SPACE_4   o = 0x60000600  l = 0x00000100
    FPGA_Plug_4    o = 0x60000700  l = 0x00000100          
    FPGA_SPACE_5   o = 0x60000800  l = 0x00000100
    FPGA_Plug_5    o = 0x60000900  l = 0x00000100  
    FPGA_SPACE_6   o = 0x60000A00  l = 0x00000100
    FPGA_Plug_6    o = 0x60000B00  l = 0x00000100    
    FPGA_SPACE_7   o = 0x60000C00  l = 0x00000100
    FPGA_Plug_7    o = 0x60000D00  l = 0x00000100  
    FPGA_SPACE_8   o = 0x60000E00  l = 0x00000100
    FPGA_Plug_8    o = 0x60000F00  l = 0x00000100          
    FPGA_SPACE_9   o = 0x60001000  l = 0x00000100
    FPGA_Plug_9    o = 0x60001100  l = 0x00000100   
    FPGA_SPACE_10  o = 0x60001200  l = 0x00000100
    FPGA_Plug_10   o = 0x60001300  l = 0x00000100    
    FPGA_SPACE_11  o = 0x60001400  l = 0x00000100
    FPGA_Plug_11   o = 0x60001500  l = 0x00000100  
    FPGA_SPACE_12  o = 0x60001600  l = 0x00000100
    FPGA_Plug_12   o = 0x60001700  l = 0x00000100          
    FPGA_SPACE_13  o = 0x60001800  l = 0x00000100
    FPGA_Plug_13   o = 0x60001900  l = 0x00000100   
    FPGA_SPACE_14  o = 0x60001A00  l = 0x00000100
    FPGA_Plug_14   o = 0x60001B00  l = 0x00000100  
    FPGA_SPACE_15  o = 0x60001C00  l = 0x00000100
    FPGA_Plug_15   o = 0x60001D00  l = 0x00000100          
    FPGA_SPACE_16  o = 0x60001E00  l = 0x00000100
    FPGA_Plug_16   o = 0x60001F00  l = 0x00000100   
 */
 	EMIFACS2BACK   o = 0x60002000  l = 0x01ffe000
    
    EMIFACS3     o = 0x62000000  l = 0x02000000  /* 32MB  异步          (CS3) */
    EMIFACS4     o = 0x64000000  l = 0x02000000  /* 32MB  异步          (CS4) */
    
//  EMIFACS5     o = 0x66000000  l = 0x02000000  /* 32MB  异步          (CS5) */

    EMIFACS5_BACK_START			  o = 0x66000000  l = 0x000001E0
    CS5_DPRAMBLOCK_1          o = 0x660001E0  l = 0x00000020 
  	CS5_DPRAMBLOCK_1BACK      o = 0x66000200  l = 0x000001fe 
  	
    CS5_DPRAMBLOCK_2          o = 0x660003fe  l = 0x00000056 
  	CS5_DPRAMBLOCK_2BACK      o = 0x66000454  l = 0x000005ac 
  	
    CS5_DPRAMBLOCK_3          o = 0x66000a00  l = 0x00000042 
  	CS5_DPRAMBLOCK_3BACK      o = 0x66000a42  l = 0x000002be 
  	  	
  	CS5_DPRAMBLOCK_4_U2       o = 0x66000d00  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U2BACK   o = 0x66000e00  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U3       o = 0x66000f00  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U3BACK   o = 0x66001000  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U4       o = 0x66001100  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U4BACK   o = 0x66001200  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U5       o = 0x66001300  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U5BACK   o = 0x66001400  l = 0x00000100   	
  	CS5_DPRAMBLOCK_4_U6       o = 0x66001500  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U6BACK   o = 0x66001600  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U7       o = 0x66001700  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U7BACK   o = 0x66001800  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U8       o = 0x66001900  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U8BACK   o = 0x66001A00  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U9       o = 0x66001B00  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U9BACK   o = 0x66001C00  l = 0x00000100  
  	CS5_DPRAMBLOCK_4_U10      o = 0x66001D00  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U10BACK  o = 0x66001E00  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U11      o = 0x66001F00  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U11BACK  o = 0x66002000  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U12      o = 0x66002100  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U12BACK  o = 0x66002200  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U13      o = 0x66002300  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U13BACK  o = 0x66002400  l = 0x00000100   	
  	CS5_DPRAMBLOCK_4_U14      o = 0x66002500  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U14BACK  o = 0x66002600  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U15      o = 0x66002700  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U15BACK  o = 0x66002800  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U16      o = 0x66002900  l = 0x00000100 	
  	CS5_DPRAMBLOCK_4_U16BACK  o = 0x66002A00  l = 0x00000100 
  	CS5_DPRAMBLOCK_4_U17      o = 0x66002B00  l = 0x00000100 
//  CS5_DPRAMBLOCK_4_U17BACK  o = 0x66002C00  l = 0x00000100 
    CS5_SST_REQUEST          o = 0x66002C00  l = 0x00000100
    CS5_SST_RESPONSE         o = 0x66002D00  l = 0x00000100
    CS5_SST_TELEMETRY        o = 0x66002E00  l = 0x00000100
    CS5_SST_HISTORY          o = 0x66002F00  l = 0x00000400
    CS5_DPRAMBLOCK_BACK      o = 0x66003300  l = 0x00000D00
		
		EMIFACS5_BACK_END         o = 0x66004000  l = 0x1ffc000


    SHRAM        o = 0x80000000  l = 0x00020000  /* 128KB 共享 RAM */
    DDR2         o = 0xC0001000  l = 0x08000000  /* 128MB  DDR2 分配给 DSP */

	EntryPoint   o = 0xC0000000  l = 0x00000800  /* 2 KB  C 语言入口点 */
	Vector       o = 0xC0000800  l = 0x00000800  /* 2 KB  中断向量表 */

/****************************************************************************/
/*                                                                          */
/*              外设内存区域                                                */
/*                                                                          */
/****************************************************************************/
    SYSCFG0      o = 0x01C14000  l = 0x00001000  /* 4K    SYSCFG0 */
    uPP          o = 0x01E16000  l = 0x00001000  /* 4K    uPP */
    GPIO         o = 0x01E26000  l = 0x00001000  /* 4K    GPIO */
    McBSP1       o = 0x01D11000  l = 0x00000800  /* 2K    McBSP1 */

#ifndef DSP_CORE
/****************************************************************************/
/*                                                                          */
/*              ARM 专有内存区域                                            */
/*                                                                          */
/****************************************************************************/
    ARMROM       o = 0xFFFD0000  l = 0x00010000  /* 64kB  ARM 本地 ROM (ARM ROM Bootloader) */
    ARMRAM       o = 0xFFFF0000  l = 0x00002000  /* 8kB   ARM 本地 RAM */
#endif
}

SECTIONS
{

//  .text:_c_int00	>  EntryPoint 				 /* 可执行代码 C 程序入口点*/

//  my_boot_sect   >  SHDSPL2RAMs1
	my_boot_sect   >  SHDSPL2RAM

    .text			>  SHDSPL2RAM 				     /* 可执行代码 */
    .stack			>  SHDSPL2RAM 				     /* 软件系统栈 */
    //.bss			>  DDR2 				     /* 未初始化全局及静态变量 */
    .bss			>  SHDSPL2RAM 				     /* 未初始化全局及静态变量 */
    .cio			>  SHDSPL2RAM                      /* C 输入输出缓存 */
    ".vectors"		>  Vector    				 /* 中断向量表 */
    .const			>  SHDSPL2RAM                      /* 常量 */
    .data			>  SHDSPL2RAM                      /* 已初始化全局及静态变量 */
    .switch			>  SHDSPL2RAM                      /* 跳转表 */
    .sysmem			>  SHDSPL2RAM                      /* 动态内存分配区域 */
//    .far			>  DDR2                      /* 远程全局及静态变量 */          //????????????????
//    .args			>  DDR2                //????????????????
//    .ppinfo			>  DDR2              //????????????????
//   .ppdata			>  DDR2              //????????????????
    
    
    .far			>  SHDSPL2RAM
    .args			>  SHDSPL2RAM
    .ppinfo			>  SHDSPL2RAM
    .ppdata			>  SHDSPL2RAM
    
    //.text:_c_int00	>  EntryPoint 				 /* 可执行代码 C 程序入口点*/
    //.text			>  DDR2 				     /* 可执行代码 */
    //.stack			>  DDR2 				     /* 软件系统栈 */
    //.bss			>  DDR2 				     /* 未初始化全局及静态变量 */
    //.cio			>  DDR2                      /* C 输入输出缓存 */
    //".vectors"		>  Vector    				 /* 中断向量表 */
    //.const			>  DDR2                      /* 常量 */
    //.data			>  DDR2                      /* 已初始化全局及静态变量 */
    //.switch			>  DDR2                      /* 跳转表 */
    //.sysmem			>  DDR2                      /* 动态内存分配区域 */
    //.far			>  DDR2                      /* 远程全局及静态变量 */
    //.args			>  DDR2
    //.ppinfo			>  DDR2
    //.ppdata			>  DDR2

    /* TI-ABI 或 COFF */
//   .pinit          >  DDR2                      /* C++ 结构表 */   //????????????????
//    .cinit          >  DDR2                      /* 初始化表 */    //????????????????

    .pinit          >  SHDSPL2RAM                      /* C++ 结构表 */
    .cinit          >  SHDSPL2RAM                      /* 初始化表 */
    
    /* EABI */
    /*
    .binit			>  DDR2
    .init_array		>  DDR2
    .neardata		>  DDR2
    .fardata		>  DDR2
    .rodata         >  DDR2
    .c6xabi.exidx	>  DDR2
    .c6xabi.extab	>  DDR2
    */

    .binit			>  SHDSPL2RAM
    .init_array		>  SHDSPL2RAM
    .neardata		>  SHDSPL2RAM
    .fardata		>  SHDSPL2RAM
    .rodata         >  SHDSPL2RAM
    .c6xabi.exidx	>  SHDSPL2RAM
    .c6xabi.extab	>  SHDSPL2RAM
	/* DDR2 */
    //.buffer			>  DDR2
    .buffer			>  SHDSPL2RAM

	/* 外设  */
	.Reg_SYSCFG0	>  SYSCFG0
	.Reg_uPP		>  uPP
	.Reg_GPIO		>  GPIO
	.Reg_McBSP1		>  McBSP1
	
	.FPGA_InterfaceFile   > FPGA_SPACE  

	.FT3_PTCT_Register           > FT3_PTCT_Register_U17
	.FT3_FaultRecorder_Register  > FT3_FaultSwitch_Register_U17
	.FT3_EER2_Register           > FT3_EER2_Register_U17
	.FT3_Backup_Register         > FT3_Backup_Register_U17

/*
	.FPGA_PU_1         > FPGA_Plug_1
  .FPGA_PU_2         > FPGA_Plug_2   
  .FPGA_PU_3         > FPGA_Plug_3  
  .FPGA_PU_4         > FPGA_Plug_4          
  .FPGA_PU_5         > FPGA_Plug_5  
  .FPGA_PU_6         > FPGA_Plug_6    
  .FPGA_PU_7         > FPGA_Plug_7  
  .FPGA_PU_8         > FPGA_Plug_8          
  .FPGA_PU_9         > FPGA_Plug_9  
  .FPGA_PU_10        > FPGA_Plug_10    
  .FPGA_PU_11        > FPGA_Plug_11
  .FPGA_PU_12        > FPGA_Plug_12         
  .FPGA_PU_13        > FPGA_Plug_13 
  .FPGA_PU_14        > FPGA_Plug_14 
  .FPGA_PU_15        > FPGA_Plug_15          
  .FPGA_PU_16        > FPGA_Plug_16   
*/
	
	.DPRAM_Block1_remControl   > CS5_DPRAMBLOCK_1, type = NOINIT
	.DPRAM_Block2_remRegulate  > CS5_DPRAMBLOCK_2, type = NOINIT
	.DPRAM_Block3_remMeaSig    > CS5_DPRAMBLOCK_3, type = NOINIT
	.DPRAM_Block4_PluginUnit2   > CS5_DPRAMBLOCK_4_U2, type = NOINIT
	.DPRAM_Block4_PluginUnit3   > CS5_DPRAMBLOCK_4_U3, type = NOINIT
	.DPRAM_Block4_PluginUnit4   > CS5_DPRAMBLOCK_4_U4, type = NOINIT
	.DPRAM_Block4_PluginUnit5   > CS5_DPRAMBLOCK_4_U5, type = NOINIT
	.DPRAM_Block4_PluginUnit6   > CS5_DPRAMBLOCK_4_U6, type = NOINIT
	.DPRAM_Block4_PluginUnit7   > CS5_DPRAMBLOCK_4_U7, type = NOINIT
	.DPRAM_Block4_PluginUnit8   > CS5_DPRAMBLOCK_4_U8, type = NOINIT
	.DPRAM_Block4_PluginUnit9   > CS5_DPRAMBLOCK_4_U9, type = NOINIT
	.DPRAM_Block4_PluginUnit10   > CS5_DPRAMBLOCK_4_U10, type = NOINIT
	.DPRAM_Block4_PluginUnit11   > CS5_DPRAMBLOCK_4_U11, type = NOINIT
	.DPRAM_Block4_PluginUnit12   > CS5_DPRAMBLOCK_4_U12, type = NOINIT
	.DPRAM_Block4_PluginUnit13   > CS5_DPRAMBLOCK_4_U13, type = NOINIT
	.DPRAM_Block4_PluginUnit14   > CS5_DPRAMBLOCK_4_U14, type = NOINIT
	.DPRAM_Block4_PluginUnit15   > CS5_DPRAMBLOCK_4_U15, type = NOINIT
	.DPRAM_Block4_PluginUnit16   > CS5_DPRAMBLOCK_4_U16, type = NOINIT
	.DPRAM_Block4_PluginUnit17   > CS5_DPRAMBLOCK_4_U17, type = NOINIT
    .DPRAM_SstRequest   > CS5_SST_REQUEST, type = NOINIT
    .DPRAM_SstResponse  > CS5_SST_RESPONSE, type = NOINIT
    .DPRAM_SstTelemetry > CS5_SST_TELEMETRY, type = NOINIT
    .DPRAM_SstHistory   > CS5_SST_HISTORY, type = NOINIT
}


  	
