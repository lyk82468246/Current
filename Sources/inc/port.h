//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: port.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __PORT_H__
#define __PORT_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
#define SEG_GROUP_SET_CURRENT       0
#define SEG_GROUP_ACTUAL_CURRENT    1
#define SEG_DISPLAY_DIGITS          8
//<<AICUBE_USER_DEFINE_END>>



void PORT0_Init(void);
void PORT1_Init(void);
void PORT2_Init(void);
void PORT3_Init(void);



//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
extern volatile uint16_t g_set_current_mA;
extern volatile uint16_t g_actual_current_mA;
extern volatile uint8_t g_seg_display_buf[SEG_DISPLAY_DIGITS];
extern volatile uint8_t g_seg_scan_pos;

void SEG_UpdateMemory(uint8_t idx, uint16_t current_mA);
void SEG_ScanNext(void);
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
