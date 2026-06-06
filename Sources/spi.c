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

static uint8_t xdata g_oled_buffer[OLED_BUFFER_SIZE];
static uint8_t xdata g_oled_dma_cmd[3];
static uint8_t g_wave_y[OLED_WIDTH];
static uint8_t g_wave_head = 0;
static volatile uint8_t g_oled_dma_busy = 0;
static uint8_t g_oled_dma_page = 0;
static uint8_t g_oled_dma_phase = 0;
static uint16_t g_oled_dma_addr = 0;
static uint8_t g_oled_test_active = 0;
static uint8_t code g_oled_sine32[32] =
{
    32, 38, 44, 49, 54, 58, 61, 63,
    63, 61, 58, 54, 49, 44, 38, 32,
    31, 25, 19, 14, 9, 5, 2, 0,
    0, 2, 5, 9, 14, 19, 25, 31
};

static void OLED_Write(uint8_t mode, uint8_t dat);
static void OLED_WriteCommand(uint8_t cmd);
static void OLED_DrawPoint(uint8_t x, uint8_t y);
static void OLED_RenderWave(void);
static void OLED_DrawVerticalLine(uint8_t x, uint8_t y0, uint8_t y1);
static void OLED_BuildTestPattern(uint8_t pattern);
static void OLED_FlushPIO(void);
static void OLED_DMAStartCommand(uint8_t page);
static void OLED_DMAStartData(void);
static void OLED_WaitDMA(void);
//<<AICUBE_USER_GLOBAL_DEFINE_END>>


uint8_t xdata pu8SPIDMATxBuffer[SPI_DMASIZE]; //SPI DMA发送缓冲区数组

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

    DMA_SPI_ManualSS();                 //关闭SPI DMA自动控制SS功能
    DMA_SPI_SetAmount(SPI_DMASIZE - 1); //设置SPI DMA发送/接收字节数
    DMA_SPI_SetTxAddress(pu8SPIDMATxBuffer); //设置SPI DMA发送缓冲区地址
    DMA_SPI_SetInterval(SPI_DMAITV);    //设置SPI DMA发送/接收字节间隔时间（系统时钟）
    DMA_SPI_ClearFIFO();                //清空SPI DMA FIFO缓冲区
    DMA_SPI_ClearFlag();                //清除SPI DMA中断标志
    DMA_SPI_EnableTx();                 //使能发送数据
    DMA_SPI_DisableRx();                //禁止接收数据
    DMA_SPI_SetBusPriority(0);          //设置总线访问为最低优先级
    DMA_SPI_Enable();                   //使能SPI DMA功能
//  DMA_SPI_MasterTrigger();            //触发SPI主机DMA

    //<<AICUBE_USER_SPI_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
#if SPI_OLED_ENABLE
    OLED_Init();
#endif
    DAC8311_Init();
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
    OLED_WaitDMA();
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
    OLED_WaitDMA();

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
    uint8_t last_y;
    uint8_t y;
    uint16_t src;

    OLED_ClearBuffer();

    src = g_wave_head;
    last_y = g_wave_y[(uint8_t)src];
    for (x = 0; x < OLED_WIDTH; x++)
    {
        src = (uint16_t)g_wave_head + x;
        if (src >= OLED_WIDTH)
        {
            src -= OLED_WIDTH;
        }

        y = g_wave_y[(uint8_t)src];
        OLED_DrawVerticalLine(x, last_y, y);
        last_y = y;
    }
}

static void OLED_DrawVerticalLine(uint8_t x, uint8_t y0, uint8_t y1)
{
    uint8_t y;
    uint8_t top;
    uint8_t bottom;

    if (y0 < y1)
    {
        top = y0;
        bottom = y1;
    }
    else
    {
        top = y1;
        bottom = y0;
    }

    for (y = top; y <= bottom; y++)
    {
        OLED_DrawPoint(x, y);
        if (y == 255)
        {
            break;
        }
    }
}

void OLED_Flush(void)
{
    uint16_t i;

    if (g_oled_dma_busy)
    {
        return;
    }

    for (i = 0; i < OLED_BUFFER_SIZE; i++)
    {
        pu8SPIDMATxBuffer[i] = g_oled_buffer[i];
    }

    g_oled_dma_page = 0;
    g_oled_dma_phase = 0;
    g_oled_dma_addr = (uint16_t)pu8SPIDMATxBuffer;
    g_oled_dma_busy = 1;
    OLED_DMAStartCommand(0);
}

static void OLED_FlushPIO(void)
{
    uint8_t page;
    uint8_t col;
    uint16_t index;

    OLED_WaitDMA();

    for (page = 0; page < OLED_PAGE_COUNT; page++)
    {
        OLED_WriteCommand((uint8_t)(0xB0 | page));
        OLED_WriteCommand(0x10);
        OLED_WriteCommand(0x00);

        D_C = OLED_DATA;
        index = (uint16_t)page * OLED_WIDTH;
        for (col = 0; col < OLED_WIDTH; col++)
        {
            SPI_WriteByte(g_oled_buffer[index + col]);
        }
    }
}

static void OLED_DMAStartCommand(uint8_t page)
{
    g_oled_dma_cmd[0] = (uint8_t)(0xB0 | (page & 0x07));
    g_oled_dma_cmd[1] = 0x10;
    g_oled_dma_cmd[2] = 0x00;

    D_C = OLED_CMD;

    DMA_SPI_Disable();
    DMA_SPI_DisableRx();
    DMA_SPI_EnableTx();
    DMA_SPI_SetAmount(3 - 1);
    DMA_SPI_TXAH = HIBYTE((uint16_t)g_oled_dma_cmd);
    DMA_SPI_TXAL = LOBYTE((uint16_t)g_oled_dma_cmd);
    DMA_SPI_SetInterval(0);
    DMA_SPI_ClearFIFO();
    DMA_SPI_ClearFlag();
    DMA_SPI_ClearOverWriteFlag();
    DMA_SPI_ClearRxLossFlag();
    SPI_ClearFlag();
    DMA_SPI_Enable();

    NOP(8);
    DMA_SPI_MasterTrigger();
}

static void OLED_DMAStartData(void)
{
    D_C = OLED_DATA;

    DMA_SPI_Disable();
    DMA_SPI_DisableRx();
    DMA_SPI_EnableTx();
    DMA_SPI_SetAmount(OLED_WIDTH - 1);
    DMA_SPI_TXAH = HIBYTE(g_oled_dma_addr);
    DMA_SPI_TXAL = LOBYTE(g_oled_dma_addr);
    DMA_SPI_SetInterval(0);
    DMA_SPI_ClearFIFO();
    DMA_SPI_ClearFlag();
    DMA_SPI_ClearOverWriteFlag();
    DMA_SPI_ClearRxLossFlag();
    SPI_ClearFlag();
    DMA_SPI_Enable();

    NOP(8);
    DMA_SPI_MasterTrigger();
    g_oled_dma_addr += OLED_WIDTH;
}

void OLED_DMATask(void)
{
    if (!g_oled_dma_busy)
    {
        return;
    }

    if (DMA_SPI_CheckFlag())
    {
        delay_us(2);
        DMA_SPI_ClearFlag();
        DMA_SPI_Disable();
        DMA_SPI_ClearFIFO();

        if (g_oled_dma_phase == 0)
        {
            g_oled_dma_phase = 1;
            OLED_DMAStartData();
        }
        else
        {
            g_oled_dma_page++;
            if (g_oled_dma_page >= OLED_PAGE_COUNT)
            {
                g_oled_dma_phase = 0;
                g_oled_dma_busy = 0;
            }
            else
            {
                g_oled_dma_phase = 0;
                OLED_DMAStartCommand(g_oled_dma_page);
            }
        }
    }
}

static void OLED_WaitDMA(void)
{
    while (g_oled_dma_busy)
    {
        OLED_DMATask();
    }
}

void OLED_WaveTask(void)
{
    uint16_t adc_value;
    uint8_t y;

    OLED_DMATask();
    if (g_oled_test_active)
    {
        return;
    }

    if (g_oled_dma_busy)
    {
        return;
    }

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

void OLED_ShowTestPattern(uint8_t pattern)
{
    OLED_BuildTestPattern(pattern);
    OLED_Flush();
}

void OLED_ShowTestPatternPIO(uint8_t pattern)
{
    OLED_BuildTestPattern(pattern);
    OLED_FlushPIO();
}

static void OLED_BuildTestPattern(uint8_t pattern)
{
    uint16_t i;
    uint8_t x;
    uint8_t y;

    OLED_WaitDMA();
    OLED_ClearBuffer();
    g_oled_test_active = 1;

    switch (pattern)
    {
    case OLED_TEST_FULL:
        for (i = 0; i < OLED_BUFFER_SIZE; i++)
        {
            g_oled_buffer[i] = 0xFF;
        }
        break;

    case OLED_TEST_ODD_ROWS:
        for (i = 0; i < OLED_BUFFER_SIZE; i++)
        {
            g_oled_buffer[i] = 0xAA;
        }
        break;

    case OLED_TEST_EVEN_ROWS:
        for (i = 0; i < OLED_BUFFER_SIZE; i++)
        {
            g_oled_buffer[i] = 0x55;
        }
        break;

    case OLED_TEST_ODD_COLS:
        for (x = 0; x < OLED_WIDTH; x++)
        {
            if (x & 0x01)
            {
                OLED_DrawVerticalLine(x, 0, OLED_HEIGHT - 1);
            }
        }
        break;

    case OLED_TEST_EVEN_COLS:
        for (x = 0; x < OLED_WIDTH; x++)
        {
            if (!(x & 0x01))
            {
                OLED_DrawVerticalLine(x, 0, OLED_HEIGHT - 1);
            }
        }
        break;

    case OLED_TEST_SINE:
        for (x = 0; x < OLED_WIDTH; x++)
        {
            y = g_oled_sine32[x & 0x1F];
            OLED_DrawPoint(x, y);
        }
        break;

    case OLED_TEST_SQUARE:
        for (x = 0; x < OLED_WIDTH; x++)
        {
            y = (x & 0x20) ? 48 : 16;
            OLED_DrawPoint(x, y);
        }
        break;

    default:
        break;
    }
}

void OLED_ResumeWave(void)
{
    g_oled_test_active = 0;
}

void OLED_SetComPins(uint8_t com_pins)
{
    OLED_WaitDMA();
    OLED_WriteCommand(0xAE);
    OLED_WriteCommand(0xDA);
    OLED_WriteCommand(com_pins);
    OLED_WriteCommand(0xAF);
}

void OLED_SetMuxRatio(uint8_t mux_ratio)
{
    if (mux_ratio < 16)
    {
        mux_ratio = 16;
    }
    else if (mux_ratio > 64)
    {
        mux_ratio = 64;
    }

    OLED_WaitDMA();
    OLED_WriteCommand(0xAE);
    OLED_WriteCommand(0xA8);
    OLED_WriteCommand((uint8_t)(mux_ratio - 1));
    OLED_WriteCommand(0xAF);
}

void OLED_SetComScanRemap(uint8_t remap)
{
    OLED_WaitDMA();
    OLED_WriteCommand(remap ? 0xC8 : 0xC0);
}

void OLED_SetSegmentRemap(uint8_t remap)
{
    OLED_WaitDMA();
    OLED_WriteCommand(remap ? 0xA1 : 0xA0);
}

void DAC8311_Init(void)
{
    DAC8311_SYNC = 1;
    DAC8311_SCLK = 0;
    DAC8311_DIN = 0;
}

void DAC8311_WriteRaw(uint16_t frame)
{
    uint8_t i;

    DAC8311_SYNC = 0;
    for (i = 0; i < 16; i++)
    {
        DAC8311_DIN = (frame & 0x8000) ? 1 : 0;
        DAC8311_SCLK = 1;
        NOP(2);
        DAC8311_SCLK = 0;
        NOP(2);
        frame <<= 1;
    }
    DAC8311_SYNC = 1;
}

void DAC8311_WriteCode(uint16_t dac_code)
{
    if (dac_code > DAC8311_MAX_CODE)
    {
        dac_code = DAC8311_MAX_CODE;
    }

    DAC8311_WriteRaw(DAC8311_PD_NORMAL | dac_code);
}

void DAC8311_PowerDown(uint16_t power_mode)
{
    DAC8311_WriteRaw(power_mode & 0xC000);
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


