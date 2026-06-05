//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: config.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __CONFIG_H__
#define __CONFIG_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
//<<AICUBE_USER_DEFINE_END>>


#define __ENCODING              "UTF-8" //DO NOT DELETE or MODIFY


#define HIRC                    40000000UL
#define FOSC                    40000000UL
#define SYSCLK                  FOSC
#define MAIN_Fosc               FOSC

#include <ai8051u.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <intrins.h>

#include "def.h"
#include "ai8051u_def.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


#define ADD                     P32
#define SUB                     P33
#define RST                     P20
#define D_C                     P21
#define DA                      P22
#define DB                      P23
#define DC                      P24
#define SH                      P07
#define SG                      P06
#define SF                      P05
#define SE                      P04
#define SD                      P03
#define SC                      P02
#define SB                      P01
#define SA                      P00


#include "port.h"
#include "timer.h"
#include "exti.h"
#include "adc.h"
#include "spi.h"
#include "i2c.h"

void SYS_Init(void);
void delay_us(uint16_t us);
void delay_ms(uint16_t ms);



//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
