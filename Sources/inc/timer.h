//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: timer.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __TIMER_H__
#define __TIMER_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
//<<AICUBE_USER_DEFINE_END>>



void TIMER0_Init(void);
void TIMER1_Init(void);
void TIMER2_Init(void);
void TIMER3_Init(void);



//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
extern volatile uint8_t g_oled_update_pending;
extern volatile uint8_t g_actual_current_update_pending;
extern volatile uint8_t g_uart_telemetry_pending;

void TIMER1_StartDebounce(void);
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
