//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: port.c
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
//<<AICUBE_USER_GLOBAL_DEFINE_END>>



////////////////////////////////////////
// P0口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT0_Init(void)
{
    SetP0nInitLevelHigh(PIN_ALL);       //设置P0初始化电平
    SetP0nQuasiMode(PIN_3 | PIN_2);     //设置P0.3,P0.2为准双向口模式
    SetP0nPushPullMode(PIN_7 | PIN_6 | PIN_5 | PIN_4 | PIN_1 | PIN_0); //设置P0.7,P0.6,P0.5,P0.4,P0.1,P0.0为推挽输出模式
    SetP0nManualMode(PIN_ALL);          //设置P0手动配置端口模式

    DisableP0nPullUp(PIN_ALL);          //关闭P0内部上拉电阻
    DisableP0nPullDown(PIN_ALL);        //关闭P0内部下拉电阻
    EnableP0nSchmitt(PIN_ALL);          //使能P0施密特触发
    SetP0nSlewRateNormal(PIN_ALL);      //设置P0一般翻转速度
    SetP0nDrivingNormal(PIN_ALL);       //设置P0一般驱动能力
    SetP0nDigitalInput(PIN_ALL);        //使能P0数字信号输入功能

    //<<AICUBE_USER_PORT0_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT0_INITIAL_END>>
}

////////////////////////////////////////
// P1口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT1_Init(void)
{
    SetP1nInitLevelHigh(PIN_ALL);       //设置P1初始化电平
    SetP1nQuasiMode(PIN_7 | PIN_6 | PIN_5 | PIN_4); //设置P1.7,P1.6,P1.5,P1.4为准双向口模式
    SetP1nPushPullMode(PIN_3 | PIN_2 | PIN_1); //设置P1.3,P1.2,P1.1为推挽输出模式
    SetP1nHighZInputMode(PIN_0);        //设置P1.0为高阻输入模式
    SetP1nManualMode(PIN_ALL);          //设置P1手动配置端口模式

    DisableP1nPullUp(PIN_ALL);          //关闭P1内部上拉电阻
    DisableP1nPullDown(PIN_ALL);        //关闭P1内部下拉电阻
    EnableP1nSchmitt(PIN_ALL);          //使能P1施密特触发
    SetP1nSlewRateNormal(PIN_ALL);      //设置P1一般翻转速度
    SetP1nDrivingNormal(PIN_ALL);       //设置P1一般驱动能力
    SetP1nDigitalInput(PIN_ALL);        //使能P1数字信号输入功能

    //<<AICUBE_USER_PORT1_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT1_INITIAL_END>>
}

////////////////////////////////////////
// P2口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT2_Init(void)
{
    SetP2nInitLevelHigh(PIN_ALL);       //设置P2初始化电平
    SetP2nQuasiMode(PIN_7 | PIN_6 | PIN_5 | PIN_1 | PIN_0); //设置P2.7,P2.6,P2.5,P2.1,P2.0为准双向口模式
    SetP2nPushPullMode(PIN_4 | PIN_3 | PIN_2); //设置P2.4,P2.3,P2.2为推挽输出模式
    SetP2nManualMode(PIN_ALL);          //设置P2手动配置端口模式

    DisableP2nPullUp(PIN_ALL);          //关闭P2内部上拉电阻
    DisableP2nPullDown(PIN_ALL);        //关闭P2内部下拉电阻
    EnableP2nSchmitt(PIN_ALL);          //使能P2施密特触发
    SetP2nSlewRateNormal(PIN_ALL);      //设置P2一般翻转速度
    SetP2nDrivingNormal(PIN_ALL);       //设置P2一般驱动能力
    SetP2nDigitalInput(PIN_ALL);        //使能P2数字信号输入功能

    //<<AICUBE_USER_PORT2_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT2_INITIAL_END>>
}

////////////////////////////////////////
// P3口初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void PORT3_Init(void)
{
    SetP3nInitLevelHigh(PIN_ALL);       //设置P3初始化电平
    SetP3nQuasiMode(PIN_ALL);           //设置P3为准双向口模式
    SetP3nManualMode(PIN_ALL);          //设置P3手动配置端口模式

    DisableP3nPullUp(PIN_ALL);          //关闭P3内部上拉电阻
    DisableP3nPullDown(PIN_ALL);        //关闭P3内部下拉电阻
    EnableP3nSchmitt(PIN_ALL);          //使能P3施密特触发
    SetP3nSlewRateNormal(PIN_ALL);      //设置P3一般翻转速度
    SetP3nDrivingNormal(PIN_ALL);       //设置P3一般驱动能力
    SetP3nDigitalInput(PIN_ALL);        //使能P3数字信号输入功能

    //<<AICUBE_USER_PORT3_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_PORT3_INITIAL_END>>
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
volatile uint16_t g_set_current_mA = 0;
volatile uint16_t g_actual_current_mA = 0;
volatile uint8_t g_seg_display_buf[SEG_DISPLAY_DIGITS] = {0};
volatile uint8_t g_seg_scan_pos = 0;

#define SEG_DP_MASK             0x80
#define SEG_BLANK_CODE          0x00

static uint8_t code SEG_DIGIT_CODE[10] =
{
    0x3f,   // 0
    0x06,   // 1
    0x5b,   // 2
    0x4f,   // 3
    0x66,   // 4
    0x6d,   // 5
    0x7d,   // 6
    0x07,   // 7
    0x7f,   // 8
    0x6f    // 9
};

static void SEG_SelectDigit(uint8_t pos)
{
    pos &= 0x07;

    DA = (pos & 0x01) ? 1 : 0;
    DB = (pos & 0x02) ? 1 : 0;
    DC = (pos & 0x04) ? 1 : 0;
}

static void SEG_FormatCurrentToBuf(uint16_t current_mA, volatile uint8_t *buf)
{
    uint16_t display_value;

    display_value = current_mA;
    if (display_value > 9999)
    {
        display_value = 9999;
    }

    buf[0] = SEG_DIGIT_CODE[display_value / 1000] | SEG_DP_MASK;
    buf[1] = SEG_DIGIT_CODE[(display_value / 100) % 10];
    buf[2] = SEG_DIGIT_CODE[(display_value / 10) % 10];
    buf[3] = SEG_DIGIT_CODE[display_value % 10];
}

void SEG_UpdateMemory(uint8_t idx, uint16_t current_mA)
{
    if (idx == SEG_GROUP_SET_CURRENT)
    {
        g_set_current_mA = current_mA;
        SEG_FormatCurrentToBuf(current_mA, &g_seg_display_buf[0]);
    }
    else if (idx == SEG_GROUP_ACTUAL_CURRENT)
    {
        g_actual_current_mA = current_mA;
        SEG_FormatCurrentToBuf(current_mA, &g_seg_display_buf[4]);
    }
}

void SEG_ScanNext(void)
{
    uint8_t physical_pos;

    physical_pos = (uint8_t)(7u - g_seg_scan_pos);

    P0 = SEG_BLANK_CODE;
    SEG_SelectDigit(physical_pos);
    P0 = g_seg_display_buf[g_seg_scan_pos];

    g_seg_scan_pos++;
    if (g_seg_scan_pos >= SEG_DISPLAY_DIGITS)
    {
        g_seg_scan_pos = 0;
    }
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


