//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: spi.c
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
#define OLED_CMD                0
#define OLED_DATA               1

static uint8_t g_oled_buffer[OLED_BUFFER_SIZE];
static uint8_t g_wave_y[OLED_WIDTH];
static uint8_t g_wave_head = 0;

static void OLED_Write(uint8_t mode, uint8_t dat);
static void OLED_WriteCommand(uint8_t cmd);
static void OLED_DrawPoint(uint8_t x, uint8_t y);
static void OLED_RenderWave(void);
//<<AICUBE_USER_GLOBAL_DEFINE_END>>



////////////////////////////////////////
// SPI初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void SPI_Init(void)
{
    SPI_SwitchP2n();                    //选择SPI数据口: SS(P2.4), MOSI(P2.5), MISO(P2.6), SCLK(P2.7)

    SPI_MasterMode();                   //设置SPI为主机模式
    SPI_IgnoreSS();                     //忽略SS脚
    SPI_DataMSB();                      //设置SPI数据顺序为MSB (高位在前)
    SPI_SetMode0();                     //设置SPI工作模式0 (CPOL=0, CPHA=0)
    SPI_SetClockDivider4();             //设置SPI时钟分频

    HSSPI_Disable();                    //关闭SPI高速模式

    SPI_Enable();                       //使能SPI功能

    //<<AICUBE_USER_SPI_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    OLED_Init();
    //<<AICUBE_USER_SPI_INITIAL_END>>
}

////////////////////////////////////////
// SPI主机模式发送字节函数
// 入口参数: dat (待发送的字节数据)
// 函数返回: 无
////////////////////////////////////////
void SPI_WriteByte(uint8_t dat)
{
    SPI_SendData(dat);                  //触发主机发送数据
    while (!SPI_CheckFlag());           //等待发送完成
    SPI_ClearFlag();                    //清除中断标志
}

////////////////////////////////////////
// SPI主机模式读取字节函数
// 入口参数: 无
// 函数返回: 读取的字节数据
////////////////////////////////////////
uint8_t SPI_ReadByte(void)
{
    SPI_SendData(0xff);                 //触发主机读取数据（主机发送时钟信号）
    while (!SPI_CheckFlag());           //等待读取完成
    SPI_ClearFlag();                    //清除中断标志
    return SPI_ReadData();
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
static void OLED_Write(uint8_t mode, uint8_t dat)
{
    D_C = mode;
    SPI_WriteByte(dat);
}

static void OLED_WriteCommand(uint8_t cmd)
{
    OLED_Write(OLED_CMD, cmd);
}

void OLED_Init(void)
{
    uint8_t i;

    RST = 0;
    delay_ms(20);
    RST = 1;
    delay_ms(100);

    OLED_WriteCommand(0xAE);            // display off
    OLED_WriteCommand(0xD5);            // set display clock
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xA8);            // set multiplex ratio
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3);            // set display offset
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0x40);            // set display start line
    OLED_WriteCommand(0x8D);            // charge pump
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0x20);            // horizontal addressing mode
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0xA1);            // segment remap
    OLED_WriteCommand(0xC8);            // COM scan direction remap
    OLED_WriteCommand(0xDA);            // COM pins hardware config
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0x81);            // contrast
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xD9);            // pre-charge period
    OLED_WriteCommand(0xF1);
    OLED_WriteCommand(0xDB);            // VCOMH deselect level
    OLED_WriteCommand(0x40);
    OLED_WriteCommand(0xA4);            // resume RAM content display
    OLED_WriteCommand(0xA6);            // normal display

    OLED_ClearBuffer();
    for (i = 0; i < OLED_WIDTH; i++)
    {
        g_wave_y[i] = OLED_HEIGHT - 1;
    }
    g_wave_head = 0;
    OLED_Flush();

    OLED_WriteCommand(0xAF);            // display on
}

void OLED_ClearBuffer(void)
{
    uint16_t i;

    for (i = 0; i < OLED_BUFFER_SIZE; i++)
    {
        g_oled_buffer[i] = 0x00;
    }
}

static void OLED_DrawPoint(uint8_t x, uint8_t y)
{
    uint16_t index;

    if ((x >= OLED_WIDTH) || (y >= OLED_HEIGHT))
    {
        return;
    }

    index = ((uint16_t)(y >> 3) * OLED_WIDTH) + x;
    g_oled_buffer[index] |= (uint8_t)(1u << (y & 0x07));
}

static void OLED_RenderWave(void)
{
    uint8_t x;
    uint16_t src;

    OLED_ClearBuffer();

    for (x = 0; x < OLED_WIDTH; x++)
    {
        src = (uint16_t)g_wave_head + x;
        if (src >= OLED_WIDTH)
        {
            src -= OLED_WIDTH;
        }

        OLED_DrawPoint(x, g_wave_y[(uint8_t)src]);
    }
}

void OLED_Flush(void)
{
    uint16_t i;

    OLED_WriteCommand(0x21);            // set column address
    OLED_WriteCommand(0x00);
    OLED_WriteCommand((uint8_t)(OLED_WIDTH - 1));
    OLED_WriteCommand(0x22);            // set page address
    OLED_WriteCommand(0x00);
    OLED_WriteCommand((uint8_t)(OLED_PAGE_COUNT - 1));

    D_C = OLED_DATA;
    for (i = 0; i < OLED_BUFFER_SIZE; i++)
    {
        SPI_WriteByte(g_oled_buffer[i]);
    }
}

void OLED_WaveTask(void)
{
    uint16_t adc_value;
    uint8_t y;

    adc_value = ADC_Convert(ADC_WAVE_CHANNEL);
    if (adc_value > 4095)
    {
        adc_value = 4095;
    }

    y = (uint8_t)(OLED_HEIGHT - 1 - (adc_value >> 6));
    g_wave_y[g_wave_head] = y;

    g_wave_head++;
    if (g_wave_head >= OLED_WIDTH)
    {
        g_wave_head = 0;
    }

    OLED_RenderWave();
    OLED_Flush();
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


