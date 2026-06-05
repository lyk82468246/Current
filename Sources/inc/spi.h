//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: spi.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __SPI_H__
#define __SPI_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
#define OLED_WIDTH              128
#define OLED_HEIGHT             64
#define OLED_PAGE_COUNT         8
#define OLED_BUFFER_SIZE        (OLED_WIDTH * OLED_PAGE_COUNT)
//<<AICUBE_USER_DEFINE_END>>



void SPI_Init(void);
uint8_t SPI_ReadByte(void);
void SPI_WriteByte(uint8_t dat);



//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
void OLED_Init(void);
void OLED_ClearBuffer(void);
void OLED_Flush(void);
void OLED_WaveTask(void);
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
