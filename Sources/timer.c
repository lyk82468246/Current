//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: timer.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-06-05) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明  
volatile uint8_t g_oled_update_pending = 0;
volatile uint8_t g_actual_current_update_pending = 0;
volatile uint8_t g_uart_telemetry_pending = 0;

extern void ADC_StartWaveSample(void);

static uint8_t g_oled_sample_divider = 0;
static uint8_t g_oled_refresh_divider = 0;
static uint8_t g_uart_telemetry_divider = 0;
//<<AICUBE_USER_GLOBAL_DEFINE_END>>



////////////////////////////////////////
// 定时器0初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER0_Init(void)
{
#define T0_PSCR                 (0)
#define T0_RELOAD               (65536 - (float)SYSCLK / 12 / (T0_PSCR + 1) * 1 / 1000) //定时周期1毫秒

    TIMER0_TimerMode();                 //设置定时器0为定时模式
    TIMER0_12TMode();                   //设置定时器0为12T模式
    TIMER0_Mode0();                     //设置定时器0为模式0 (16位自动重载模式)
    TIMER0_DisableGateINT0();           //禁止定时器0门控
    TIMER0_SetIntPriority(0);           //设置中断为最低优先级
    TIMER0_EnableInt();                 //使能定时器0中断
    TIMER0_SetPrescale(T0_PSCR);        //设置定时器0的8位预分频
    TIMER0_SetReload16(T0_RELOAD);      //设置定时器0的16位重载值
    TIMER0_Run();                       //定时器0开始运行

    //<<AICUBE_USER_TIMER0_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_TIMER0_INITIAL_END>>
}

////////////////////////////////////////
// 定时器1初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER1_Init(void)
{
#define T1_PSCR                 (1)
#define T1_RELOAD               (65536 - (float)SYSCLK / 12 / (T1_PSCR + 1) * 20 / 1000) //定时周期20毫秒

    TIMER1_TimerMode();                 //设置定时器1为定时模式
    TIMER1_12TMode();                   //设置定时器1为12T模式
    TIMER1_Mode0();                     //设置定时器1为模式0 (16位自动重载模式)
    TIMER1_DisableGateINT1();           //禁止定时器1门控
    TIMER1_SetIntPriority(1);           //设置中断为较低优先级
    TIMER1_EnableInt();                 //使能定时器1中断
    TIMER1_SetPrescale(T1_PSCR);        //设置定时器1的8位预分频
    TIMER1_SetReload16(T1_RELOAD);      //设置定时器1的16位重载值
    TIMER1_Run();                       //定时器1开始运行

    //<<AICUBE_USER_TIMER1_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    TIMER1_Stop();
    TIMER1_DisableInt();
    TIMER1_ClearFlag();
    //<<AICUBE_USER_TIMER1_INITIAL_END>>
}

////////////////////////////////////////
// 定时器2初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER2_Init(void)
{
#define T2_PSCR                 (0)
#ifdef BAUDRATE
#undef BAUDRATE
#endif
#define BAUDRATE                (115200UL)
#define T2_RELOAD               (65536 - (SYSCLK / BAUDRATE + 2) / 4)

    TIMER2_TimerMode();                 //设置定时器2为定时模式
    TIMER2_1TMode();                    //设置定时器2为1T模式
    TIMER2_SetPrescale(T2_PSCR);        //设置定时器2的8位预分频
    TIMER2_SetReload16(T2_RELOAD);      //设置定时器2的16位重载值
    TIMER2_Run();                       //定时器2开始运行

    //<<AICUBE_USER_TIMER2_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_TIMER2_INITIAL_END>>
}

////////////////////////////////////////
// 定时器3初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER3_Init(void)
{
#define T3_PSCR                 (5)
#define T3_RELOAD               (65536 - (float)SYSCLK / 12 / (T3_PSCR + 1) * 100 / 1000) //定时周期100毫秒

    TIMER3_TimerMode();                 //设置定时器3为定时模式
    TIMER3_12TMode();                   //设置定时器3为12T模式
    TIMER3_EnableInt();                 //使能定时器3中断
    TIMER3_SetPrescale(T3_PSCR);        //设置定时器3的8位预分频
    TIMER3_SetReload16(T3_RELOAD);      //设置定时器3的16位重载值
    TIMER3_Run();                       //定时器3开始运行

    //<<AICUBE_USER_TIMER3_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_TIMER3_INITIAL_END>>
}


////////////////////////////////////////
// 定时器0中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER0_ISR(void) interrupt TMR0_VECTOR
{
    //<<AICUBE_USER_TIMER0_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码
    SEG_ScanNext();

    g_oled_sample_divider++;
    if (g_oled_sample_divider >= SSD1315_WAVE_SAMPLE_INTERVAL_MS)
    {
        g_oled_sample_divider = 0;
        ADC_StartWaveSample();
    }

    g_oled_refresh_divider++;
    if (g_oled_refresh_divider >= 50)
    {
        g_oled_refresh_divider = 0;
        g_oled_update_pending = 1;
    }
    //<<AICUBE_USER_TIMER0_ISR_CODE1_END>>
}

////////////////////////////////////////
// 定时器1中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER1_ISR(void) interrupt TMR1_VECTOR
{
    //<<AICUBE_USER_TIMER1_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码  
    EXTI_KeyDebounceProcess();
    //<<AICUBE_USER_TIMER1_ISR_CODE1_END>>
}

////////////////////////////////////////
// 定时器3中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void TIMER3_ISR(void) interrupt TMR3_VECTOR
{
    //<<AICUBE_USER_TIMER3_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码  
    TIMER3_ClearFlag();
    g_actual_current_update_pending = 1;

    g_uart_telemetry_divider++;
    if (g_uart_telemetry_divider >= 10)
    {
        g_uart_telemetry_divider = 0;
        g_uart_telemetry_pending = 1;
    }
    //<<AICUBE_USER_TIMER3_ISR_CODE1_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
void TIMER1_StartDebounce(void)
{
    TIMER1_Stop();
    TIMER1_ClearFlag();
    TIMER1_SetReload16(T1_RELOAD);
    TIMER1_EnableInt();
    TIMER1_Run();
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


