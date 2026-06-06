//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: exti.c
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
#define KEY_DEBOUNCE_ADD_FLAG       0x01
#define KEY_DEBOUNCE_SUB_FLAG       0x02
#define KEY_ADD_LONG_TICKS          50

static volatile uint8_t g_key_debounce_flags = 0;
static uint8_t g_add_tracking = 0;
static uint8_t g_add_press_ticks = 0;
static uint8_t g_add_long_fired = 0;

static void KeyDebounceStart(uint8_t flag);
static void CurrentAdjust(uint8_t increase);
static void CurrentModeToggle(void);
//<<AICUBE_USER_GLOBAL_DEFINE_END>>



////////////////////////////////////////
// 外部中断INT0初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void EXTI0_Init(void)
{
    INT0_FallingInt();                  //设置外部中断为下降沿中断
    INT0_SetIntPriority(2);             //设置中断为较高优先级
    INT0_EnableInt();                   //使能外部中断

    //<<AICUBE_USER_EXTI0_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_EXTI0_INITIAL_END>>
}

////////////////////////////////////////
// 外部中断INT1初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void EXTI1_Init(void)
{
    INT1_FallingInt();                  //设置外部中断为下降沿中断
    INT1_SetIntPriority(2);             //设置中断为较高优先级
    INT1_EnableInt();                   //使能外部中断

    //<<AICUBE_USER_EXTI1_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_EXTI1_INITIAL_END>>
}


////////////////////////////////////////
// 外部中断INT0中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void EXTI0_ISR(void) interrupt INT0_VECTOR
{
    //<<AICUBE_USER_EXTI0_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码  
    INT0_ClearFlag();
    KeyDebounceStart(KEY_DEBOUNCE_ADD_FLAG);
    //<<AICUBE_USER_EXTI0_ISR_CODE1_END>>
}

////////////////////////////////////////
// 外部中断INT1中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void EXTI1_ISR(void) interrupt INT1_VECTOR
{
    //<<AICUBE_USER_EXTI1_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码  
    INT1_ClearFlag();
    KeyDebounceStart(KEY_DEBOUNCE_SUB_FLAG);
    //<<AICUBE_USER_EXTI1_ISR_CODE1_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
static void KeyDebounceStart(uint8_t flag)
{
    g_key_debounce_flags |= flag;

    TIMER1_StartDebounce();
}

void EXTI_KeyDebounceProcess(void)
{
    uint8_t flags;
    uint8_t restart_timer;

    TIMER1_Stop();
    TIMER1_DisableInt();
    TIMER1_ClearFlag();

    restart_timer = 0;
    flags = g_key_debounce_flags;
    g_key_debounce_flags = 0;

    if (g_add_tracking)
    {
        if (ADD == 0)
        {
            if (g_add_press_ticks < 255)
            {
                g_add_press_ticks++;
            }

            if (!g_add_long_fired && (g_add_press_ticks >= KEY_ADD_LONG_TICKS))
            {
                CurrentModeToggle();
                g_add_long_fired = 1;
            }

            restart_timer = 1;
        }
        else
        {
            if (!g_add_long_fired)
            {
                CurrentAdjust(1);
            }

            g_add_tracking = 0;
            g_add_press_ticks = 0;
            g_add_long_fired = 0;
        }
    }
    else if ((flags & KEY_DEBOUNCE_ADD_FLAG) && (ADD == 0))
    {
        g_add_tracking = 1;
        g_add_press_ticks = 1;
        g_add_long_fired = 0;
        restart_timer = 1;
    }

    if ((flags & KEY_DEBOUNCE_SUB_FLAG) && (SUB == 0))
    {
        CurrentAdjust(0);
    }

    if (restart_timer)
    {
        TIMER1_StartDebounce();
    }
}

static void CurrentAdjust(uint8_t increase)
{
    uint16_t step_mA;
    uint16_t max_mA;

    if (g_current_mode == 0)
    {
        step_mA = 50;
        max_mA = 1000;
    }
    else
    {
        step_mA = 100;
        max_mA = 2000;
    }

    if (increase)
    {
        if (g_current_set_mA < 100)
        {
            g_current_set_mA = 100;
        }
        else
        {
            g_current_set_mA += step_mA;
        }

        if (g_current_set_mA > max_mA)
        {
            g_current_set_mA = max_mA;
        }
    }
    else
    {
        if (g_current_set_mA <= 100)
        {
            g_current_set_mA = 0;
        }
        else
        {
            g_current_set_mA -= step_mA;
            if (g_current_set_mA < 100)
            {
                g_current_set_mA = 100;
            }
        }
    }
}

static void CurrentModeToggle(void)
{
    if (g_current_mode == 0)
    {
        g_current_mode = 1;
        g_current_set_mA = (g_current_set_mA / 100) * 100;
    }
    else
    {
        g_current_mode = 0;
        if (g_current_set_mA > 1000)
        {
            g_current_set_mA = 1000;
        }
    }
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


