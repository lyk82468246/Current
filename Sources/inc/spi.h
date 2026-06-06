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

#define DAC8311_RESOLUTION      14
#define DAC8311_MAX_CODE        0x3FFF
#define DAC8311_PD_NORMAL       0x0000
#define DAC8311_PD_1K           0x4000
#define DAC8311_PD_100K         0x8000
#define DAC8311_PD_HIGH_Z       0xC000

#define DAC8311_USE_HARDWARE_SPI 0

#define OLED_TEST_EMPTY         0
#define OLED_TEST_FULL          1
#define OLED_TEST_ODD_ROWS      2
#define OLED_TEST_EVEN_ROWS     3
#define OLED_TEST_ODD_COLS      4
#define OLED_TEST_EVEN_COLS     5
#define OLED_TEST_SINE          6
#define OLED_TEST_SQUARE        7

#define SPI_OLED_ENABLE         0
//<<AICUBE_USER_DEFINE_END>>


#define SPI_DMASIZE         1024        //SPI DMA发送缓冲区大小
#define SPI_DMAITV              0       //SPI DMA发送/接收字节间隔时间（系统时钟）


void SPI_Init(void);
uint8_t SPI_ReadByte(void);
void SPI_WriteByte(uint8_t dat);

extern uint8_t xdata pu8SPIDMATxBuffer[SPI_DMASIZE];


//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
void OLED_Init(void);
void OLED_ClearBuffer(void);
void OLED_Flush(void);
void OLED_DMATask(void);
void OLED_WaveTask(void);
void OLED_ShowTestPattern(uint8_t pattern);
void OLED_ShowTestPatternPIO(uint8_t pattern);
void OLED_ResumeWave(void);
void OLED_SetComPins(uint8_t com_pins);
void OLED_SetMuxRatio(uint8_t mux_ratio);
void OLED_SetComScanRemap(uint8_t remap);
void OLED_SetSegmentRemap(uint8_t remap);

void DAC8311_Init(void);
void DAC8311_WriteRaw(uint16_t frame);
void DAC8311_WriteCode(uint16_t dac_code);
void DAC8311_PowerDown(uint16_t power_mode);
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
