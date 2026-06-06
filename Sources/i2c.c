//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: i2c.c
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
#define SSD1315_CMD_CONTROL             0x00
#define SSD1315_DATA_CONTROL            0x40
#define pu8TxBuffer                     pu8I2CDMATxBuffer
#define pu8RxBuffer                     pu8I2CDMARxBuffer

static uint8_t xdata pu8I2CDMARxBuffer[I2C_DMARXSIZE];
static uint8_t xdata g_ssd1315_buffer[SSD1315_BUFFER_SIZE];
static uint16_t xdata g_ssd1315_wave_adc[SSD1315_WIDTH];
static uint8_t g_ssd1315_i2c_addr = SSD1315_I2C_ADDR;
static uint8_t g_ssd1315_wave_head = 0;
static uint8_t g_ssd1315_test_active = 0;
static volatile uint8_t g_ssd1315_scope_mode = SSD1315_SCOPE_MODE_GLOBAL;
static uint8_t code g_ssd1315_sine32[32] =
{
    32, 38, 44, 49, 54, 58, 61, 63,
    63, 61, 58, 54, 49, 44, 38, 32,
    31, 25, 19, 14, 9, 5, 2, 0,
    0, 2, 5, 9, 14, 19, 25, 31
};

static BOOL SSD1315_WriteCommand(uint8_t cmd);
static BOOL SSD1315_WriteCommand2(uint8_t cmd, uint8_t dat);
static BOOL SSD1315_WriteDataPage(uint16_t offset);
static uint8_t SSD1315_GetSLAW(void);
static void SSD1315_DrawPoint(uint8_t x, uint8_t y);
static void SSD1315_DrawVerticalLine(uint8_t x, uint8_t y0, uint8_t y1);
static void SSD1315_RenderWave(void);
static void SSD1315_BuildTestPattern(uint8_t pattern);
static uint8_t SSD1315_MapGlobal(uint16_t adc_value);
static uint8_t SSD1315_MapZoom(uint16_t adc_value, uint16_t lower, uint16_t upper);
static void SSD1315_DrawScopeOverlay(uint16_t lower, uint16_t upper);
static void SSD1315_DrawTextXor(uint8_t x, uint8_t y, char *text);
static void SSD1315_InvertPixel(uint8_t x, uint8_t y);
static uint8_t SSD1315_GetFontRow(char c, uint8_t row);
static uint8_t SSD1315_TextWidth(char *text);
static uint32_t SSD1315_AdcToCurrentMicroAmp(uint16_t adc_value);
static void SSD1315_FormatCurrent(char *buf, uint32_t current_uA);
static void SSD1315_FormatTimebase(char *buf);
static char *SSD1315_AppendU16(char *buf, uint16_t value);
//<<AICUBE_USER_GLOBAL_DEFINE_END>>


uint8_t xdata pu8I2CDMATxBuffer[I2C_DMATXSIZE]; //I2C DMA发送缓冲区数组

////////////////////////////////////////
// I2C初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C_Init(void)
{
    I2C_SwitchP1415();                  //选择I2C数据口: SCL(P1.5), SDA(P1.4)

    I2C_MasterMode();                   //设置I2C为主机模式
    I2C_SetClockDivider(32);            //设置I2C为主机模式时钟

    I2C_Enable();                       //使能I2C功能

    DMA_I2C_EnableDMA();                //使能I2C DMA
    DMA_I2C_SetDMAAmount(I2C_DMARXSIZE - 1); //使能I2C DMA发送/接收总字节数
    DMA_I2C_SetTxAmount(I2C_DMATXSIZE - 1); //设置I2C DMA发送总字节数
    DMA_I2C_SetTxAddress(pu8I2CDMATxBuffer); //设置I2C DMA发送缓冲区地址
    DMA_I2C_SetInterval(I2C_DMAITV);    //设置I2C DMA发送/接收字节间隔时间（系统时钟）
    DMA_I2C_ClearTxFlag();              //清除I2C 发送DMA中断标志
    DMA_I2C_SetTxBusPriority(0);        //设置总线访问为最低优先级
    DMA_I2C_EnableTx();                 //使能I2C DMA发送功能
//  DMA_I2C_TriggerTx();                //触发I2C DMA发送

    //<<AICUBE_USER_I2C_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    SSD1315_Init();
    //<<AICUBE_USER_I2C_INITIAL_END>>
}

////////////////////////////////////////
// I2C主机模式等待命令完成
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C_MasterWait(void)
{
    while (!I2C_CheckMasterFlag());     //等待完成标志
    I2C_ClearMasterFlag();              //清除完成标志
    I2C_Idle();                         //恢复IDLE状态
}

////////////////////////////////////////
// I2C主机模式发送起始信号
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C_MasterStart(void)
{
    I2C_Start();                        //触发主机模式起始命令
    I2C_MasterWait();                   //等待命令完成
}

////////////////////////////////////////
// I2C主机模式发送停止信号
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void I2C_MasterStop(void)
{
    I2C_Stop();                         //触发主机模式停止命令
    I2C_MasterWait();                   //等待命令完成
}

////////////////////////////////////////
// I2C主机模式发送起始信号+1字节数据
// 入口参数: dat (待发送的字节数据)
// 函数返回: 0   (接收的应答信号为ACK)
//           1   (接收的应答信号为NAK)
////////////////////////////////////////
BOOL I2C_MasterStartSendByte(uint8_t dat)
{
    I2C_WriteData(dat);                 //将数据写入I2C数据寄存器
    I2C_StartSendDataRecvACK();         //触发主机模式起始信号+写数据+读取ACK组合命令
    I2C_MasterWait();                   //等待命令完成

    return I2C_MasterReadACK();         //读取并返回应答信号
}

////////////////////////////////////////
// I2C主机模式发送1字节数据
// 入口参数: dat (待发送的字节数据)
// 函数返回: 0   (接收的应答信号为ACK)
//           1   (接收的应答信号为NAK)
////////////////////////////////////////
BOOL I2C_MasterSendByte(uint8_t dat)
{
    I2C_WriteData(dat);                 //将数据写入I2C数据寄存器
    I2C_SendDataRecvACK();              //触发主机模式写数据+读取ACK组合命令
    I2C_MasterWait();                   //等待命令完成

    return I2C_MasterReadACK();         //读取并返回应答信号
}

////////////////////////////////////////
// I2C主机模式接收1字节数据
// 入口参数: ack (待发送的应答信号)
// 函数返回:     (接收的字节数据)
////////////////////////////////////////
uint8_t I2C_MasterReadByte(BOOL ack)
{
    uint8_t dat;

    if (!ack)
        I2C_RecvDataSendACK();          //触发主机模式读数据+返回ACK命令
    else
        I2C_RecvDataSendNAK();          //触发主机模式读数据+返回NAK命令
    I2C_MasterWait();                   //等待命令完成
    dat = I2C_ReadData();               //读取接收的数据

    return dat;                         //返回接收的数据
}

////////////////////////////////////////
// I2C主机模式DMA方式发送数据
// 入口参数: cnt (DMA发送数据数量)
// 函数返回: 无
////////////////////////////////////////
void I2C_DMA_MasterTriggerSend(uint16_t cnt)
{
    DMA_I2C_DisableDMA();               //写DMA触发命令前须先关闭比DMA
    I2C_StartSendDataRecvACK();         //I2C DMA发送数据必须使用09H命令
    DMA_I2C_EnableDMA();                //发送DMA命令前，先使能I2C DMA功能

    DMA_I2C_SetDMAAmount(cnt - 1);      //使能I2C DMA发送/接收总字节数
    DMA_I2C_SetTxAmount(cnt - 1);       //设置I2C DMA发送总字节数
    DMA_I2C_ClearTxFlag();              //清除I2C 发送DMA中断标志
    DMA_I2C_TriggerTx();                //触发I2C DMA发送
}

////////////////////////////////////////
// I2C主机模式DMA方式读取数据
// 入口参数: cnt (DMA读取数据数量)
// 函数返回: 无
////////////////////////////////////////
void I2C_DMA_MasterTriggerRead(uint16_t cnt)
{
    DMA_I2C_DisableDMA();               //写DMA触发命令前须先关闭比DMA
    I2C_MasterSetACK();                 //(重要！！！需要先设置好主机读取数据后的ACK信号)
    I2C_RecvDataSendACK();              //I2C DMA接收数据必须使用11H命令
    DMA_I2C_EnableDMA();                //发送DMA命令前，先使能I2C DMA功能

    DMA_I2C_SetDMAAmount(cnt - 1);      //使能I2C DMA发送/接收总字节数
    DMA_I2C_SetRxAmount(cnt - 1);       //设置I2C DMA接收总字节数
    DMA_I2C_ClearRxFlag();              //清除I2C 接收DMA中断标志
    DMA_I2C_TriggerRx();                //触发I2C DMA接收
}

////////////////////////////////////////
// I2C使用DMA方式读AT24C02函数
// 入口参数: addr (EEPROM目标地址)
//           dat  (数据缓冲区)
//           size (数据大小)
// 函数返回: 无
////////////////////////////////////////
#define AT24C02_SLAW            0xa0    //写设备地址
#define AT24C02_SLAR            0xa1    //读设备地址
#define AT24C02_PAGE_SIZE       8       //页编程大小

void I2C_DMA_ReadAT24C02(uint8_t addr, uint8_t *dat, uint8_t size)
{
    while (I2C_CheckMasterBusy());      //等待I2C模块退出忙状态
    I2C_ClearMasterFlag();              //清除完成标志

    DMA_I2C_DisableDMA();               //需要先用寄存器模式发送起始信号和地址信息
    I2C_MasterStartSendByte(AT24C02_SLAW); //发送起始信号+设备写信号
    I2C_MasterSendByte(addr);           //发送地址信息
    I2C_MasterStartSendByte(AT24C02_SLAR); //发送起始信号+设备读信号

    I2C_DMA_MasterTriggerRead(size);    //触发DMA

    while (I2C_CheckMasterBusy());      //等待I2C模块退出忙状态

    memcpy(dat, pu8RxBuffer, size);     //将DMA接收缓冲区的数据复制到数据缓冲区
}

////////////////////////////////////////
// I2C使用DMA方式写AT24C02函数
// 入口参数: addr (EEPROM目标地址)
//           dat  (数据缓冲区)
//           size (数据大小)
// 函数返回: 无
////////////////////////////////////////
void I2C_DMA_WriteAT24C02(uint8_t addr, uint8_t *dat, uint8_t size)
{
    uint8_t cnt;

    while (size)                        //判断数据是否发送完成
    {
        while (I2C_CheckMasterBusy());  //等待I2C模块退出忙状态
        I2C_ClearMasterFlag();          //清除完成标志

        cnt = AT24C02_PAGE_SIZE - (addr & (AT24C02_PAGE_SIZE - 1)); //防止跨页编程
        if (cnt > size) cnt = size;     //获取正确的数据大小

        pu8TxBuffer[0] = AT24C02_SLAW;  //设备写信号数据
        pu8TxBuffer[1] = addr;          //地址信息数据
        memcpy(&pu8TxBuffer[2], dat, cnt); //将待写数据复制到DMA发送缓冲区
        size -= cnt;
        dat += cnt;
        addr += cnt;
        cnt += 2;                       //DMA总数=总数据量+2

        I2C_DMA_MasterTriggerSend(cnt); //触发DMA

        delay_ms(5);                    //等待AT24C02编程完成
    }
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
static BOOL ADS1110_SendAddress(uint8_t addr)
{
    if (I2C_MasterStartSendByte(addr))
    {
        I2C_MasterStop();
        return FALSE;
    }

    return TRUE;
}

BOOL ADS1110_WriteConfig(uint8_t config)
{
    while (I2C_CheckMasterBusy());
    I2C_ClearMasterFlag();

    if (!ADS1110_SendAddress(ADS1110_SLAW))
        return FALSE;

    if (I2C_MasterSendByte(config))
    {
        I2C_MasterStop();
        return FALSE;
    }

    I2C_MasterStop();
    return TRUE;
}

BOOL ADS1110_Init(void)
{
    return ADS1110_WriteConfig(ADS1110_CONFIG_DEFAULT);
}

BOOL ADS1110_StartSingle(uint8_t config)
{
    config &= (ADS1110_CFG_DR_MASK | ADS1110_CFG_PGA_MASK);
    config |= ADS1110_CFG_ST_DRDY | ADS1110_CFG_SINGLE;

    return ADS1110_WriteConfig(config);
}

BOOL ADS1110_ReadRaw(int16_t *raw, uint8_t *config)
{
    uint8_t msb;
    uint8_t lsb;
    uint8_t cfg;
    uint16_t word;

    if (raw == NULL)
        return FALSE;

    while (I2C_CheckMasterBusy());
    I2C_ClearMasterFlag();

    if (!ADS1110_SendAddress(ADS1110_SLAR))
        return FALSE;

    msb = I2C_MasterReadByte(FALSE);
    lsb = I2C_MasterReadByte(FALSE);
    cfg = I2C_MasterReadByte(TRUE);
    I2C_MasterStop();

    word = ((uint16_t)msb << 8) | lsb;
    *raw = (int16_t)word;

    if (config != NULL)
        *config = cfg;

    return TRUE;
}

BOOL ADS1110_ReadRaw16(int16_t *raw)
{
    return ADS1110_ReadRaw(raw, NULL);
}

int32_t ADS1110_RawToMicroVolt(int16_t raw, uint8_t config)
{
    int32_t value;
    int32_t divider;

    divider = 1;

    switch (config & ADS1110_CFG_PGA_MASK)
    {
    case ADS1110_CFG_PGA_2:
        divider = 2;
        break;

    case ADS1110_CFG_PGA_4:
        divider = 4;
        break;

    case ADS1110_CFG_PGA_8:
        divider = 8;
        break;

    default:
        divider = 1;
        break;
    }

    switch (config & ADS1110_CFG_DR_MASK)
    {
    case ADS1110_CFG_DR_240SPS_12BIT:
        value = (int32_t)raw * 1000L;
        break;

    case ADS1110_CFG_DR_60SPS_14BIT:
        value = (int32_t)raw * 250L;
        break;

    case ADS1110_CFG_DR_30SPS_15BIT:
        value = (int32_t)raw * 125L;
        break;

    default:
        value = (int32_t)raw * 625L;
        divider *= 10;
        break;
    }

    if (value >= 0)
        value = (value + (divider / 2)) / divider;
    else
        value = (value - (divider / 2)) / divider;

    return value;
}

static uint8_t SSD1315_GetSLAW(void)
{
    return (uint8_t)((g_ssd1315_i2c_addr << 1) | 0);
}

BOOL SSD1315_Probe(uint8_t addr)
{
    BOOL nack;

    DMA_I2C_DisableDMA();
    while (I2C_CheckMasterBusy());
    I2C_ClearMasterFlag();

    nack = I2C_MasterStartSendByte((uint8_t)((addr << 1) | 0));
    I2C_MasterStop();

    return !nack;
}

void SSD1315_SetAddress(uint8_t addr)
{
    g_ssd1315_i2c_addr = addr;
}

static BOOL SSD1315_WriteCommand(uint8_t cmd)
{
    DMA_I2C_DisableDMA();
    while (I2C_CheckMasterBusy());
    I2C_ClearMasterFlag();

    if (I2C_MasterStartSendByte(SSD1315_GetSLAW()))
    {
        I2C_MasterStop();
        return FALSE;
    }

    if (I2C_MasterSendByte(SSD1315_CMD_CONTROL))
    {
        I2C_MasterStop();
        return FALSE;
    }

    if (I2C_MasterSendByte(cmd))
    {
        I2C_MasterStop();
        return FALSE;
    }

    I2C_MasterStop();
    return TRUE;
}

static BOOL SSD1315_WriteCommand2(uint8_t cmd, uint8_t dat)
{
    DMA_I2C_DisableDMA();
    while (I2C_CheckMasterBusy());
    I2C_ClearMasterFlag();

    if (I2C_MasterStartSendByte(SSD1315_GetSLAW()))
    {
        I2C_MasterStop();
        return FALSE;
    }

    if (I2C_MasterSendByte(SSD1315_CMD_CONTROL))
    {
        I2C_MasterStop();
        return FALSE;
    }

    if (I2C_MasterSendByte(cmd) || I2C_MasterSendByte(dat))
    {
        I2C_MasterStop();
        return FALSE;
    }

    I2C_MasterStop();
    return TRUE;
}

static BOOL SSD1315_WriteDataPage(uint16_t offset)
{
    uint8_t i;

#if SSD1315_USE_I2C_DMA
    DMA_I2C_DisableDMA();
    while (I2C_CheckMasterBusy());
    I2C_ClearMasterFlag();

    pu8I2CDMATxBuffer[0] = SSD1315_GetSLAW();
    pu8I2CDMATxBuffer[1] = SSD1315_DATA_CONTROL;
    for (i = 0; i < SSD1315_WIDTH; i++)
    {
        pu8I2CDMATxBuffer[i + 2] = g_ssd1315_buffer[offset + i];
    }

    DMA_I2C_SetTxAddress(pu8I2CDMATxBuffer);
    I2C_StartSendDataRecvACK();
    DMA_I2C_EnableDMA();
    DMA_I2C_SetDMAAmount(SSD1315_WIDTH + 1);
    DMA_I2C_SetTxAmount(SSD1315_WIDTH + 1);
    DMA_I2C_ClearTxFlag();
    DMA_I2C_EnableTx();
    DMA_I2C_TriggerTx();

    while (!DMA_I2C_CheckTxFlag());
    DMA_I2C_ClearTxFlag();
    DMA_I2C_DisableDMA();

    if (DMA_I2C_CheckACK())
    {
        return FALSE;
    }

    return TRUE;
#else
    DMA_I2C_DisableDMA();
    while (I2C_CheckMasterBusy());
    I2C_ClearMasterFlag();

    if (I2C_MasterStartSendByte(SSD1315_GetSLAW()))
    {
        I2C_MasterStop();
        return FALSE;
    }

    if (I2C_MasterSendByte(SSD1315_DATA_CONTROL))
    {
        I2C_MasterStop();
        return FALSE;
    }

    for (i = 0; i < SSD1315_WIDTH; i++)
    {
        if (I2C_MasterSendByte(g_ssd1315_buffer[offset + i]))
        {
            I2C_MasterStop();
            return FALSE;
        }
    }

    I2C_MasterStop();
    return TRUE;
#endif
}

BOOL SSD1315_Init(void)
{
    uint8_t i;
    BOOL ok;

    delay_ms(50);

    ok = TRUE;
    if (!SSD1315_WriteCommand(0xAE)) ok = FALSE;
    if (!SSD1315_WriteCommand2(0xD5, 0x80)) ok = FALSE;
    if (!SSD1315_WriteCommand2(0xA8, 0x3F)) ok = FALSE;
    if (!SSD1315_WriteCommand2(0xD3, 0x00)) ok = FALSE;
    if (!SSD1315_WriteCommand(0x40)) ok = FALSE;
    if (!SSD1315_WriteCommand2(0x8D, 0x14)) ok = FALSE;
    if (!SSD1315_WriteCommand2(0x20, 0x02)) ok = FALSE;
    if (!SSD1315_WriteCommand(0xA1)) ok = FALSE;
    if (!SSD1315_WriteCommand(0xC8)) ok = FALSE;
    if (!SSD1315_WriteCommand2(0xDA, 0x12)) ok = FALSE;
    if (!SSD1315_WriteCommand2(0x81, 0xCF)) ok = FALSE;
    if (!SSD1315_WriteCommand2(0xD9, 0xF1)) ok = FALSE;
    if (!SSD1315_WriteCommand2(0xDB, 0x40)) ok = FALSE;
    if (!SSD1315_WriteCommand(0xA4)) ok = FALSE;
    if (!SSD1315_WriteCommand(0xA6)) ok = FALSE;

    SSD1315_ClearBuffer();
    for (i = 0; i < SSD1315_WIDTH; i++)
    {
        g_ssd1315_wave_adc[i] = 2048;
    }
    g_ssd1315_wave_head = 0;
    if (!SSD1315_Flush()) ok = FALSE;
    if (!SSD1315_WriteCommand(0xAF)) ok = FALSE;

    return ok;
}

void SSD1315_ClearBuffer(void)
{
    uint16_t i;

    for (i = 0; i < SSD1315_BUFFER_SIZE; i++)
    {
        g_ssd1315_buffer[i] = 0x00;
    }
}

BOOL SSD1315_Flush(void)
{
    uint8_t page;
    uint16_t offset;

    for (page = 0; page < SSD1315_PAGE_COUNT; page++)
    {
        offset = (uint16_t)page * SSD1315_WIDTH;

        if (!SSD1315_WriteCommand((uint8_t)(0xB0 | page)))
            return FALSE;
        if (!SSD1315_WriteCommand(0x00))
            return FALSE;
        if (!SSD1315_WriteCommand(0x10))
            return FALSE;
        if (!SSD1315_WriteDataPage(offset))
            return FALSE;
    }

    return TRUE;
}

static void SSD1315_DrawPoint(uint8_t x, uint8_t y)
{
    uint16_t index;

    if ((x >= SSD1315_WIDTH) || (y >= SSD1315_HEIGHT))
    {
        return;
    }

    index = ((uint16_t)(y >> 3) * SSD1315_WIDTH) + x;
    g_ssd1315_buffer[index] |= (uint8_t)(1u << (y & 0x07));
}

static void SSD1315_DrawVerticalLine(uint8_t x, uint8_t y0, uint8_t y1)
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
        SSD1315_DrawPoint(x, y);
        if (y == 255)
        {
            break;
        }
    }
}

static void SSD1315_RenderWave(void)
{
    uint8_t x;
    uint8_t last_y;
    uint8_t y;
    uint16_t src;
    uint16_t lower;
    uint16_t upper;
    uint16_t sample;
    uint16_t mean;
    uint16_t span;
    uint32_t sum;
    uint32_t dev_sum;

    SSD1315_ClearBuffer();

    lower = 0;
    upper = 4095;
    if (g_ssd1315_scope_mode == SSD1315_SCOPE_MODE_ZOOM)
    {
        sum = 0;
        for (x = 0; x < SSD1315_WIDTH; x++)
        {
            sum += g_ssd1315_wave_adc[x];
        }
        mean = (uint16_t)(sum / SSD1315_WIDTH);

        dev_sum = 0;
        for (x = 0; x < SSD1315_WIDTH; x++)
        {
            sample = g_ssd1315_wave_adc[x];
            dev_sum += (sample > mean) ? (sample - mean) : (mean - sample);
        }

        span = (uint16_t)((dev_sum / SSD1315_WIDTH) * 8);
        if (span < SSD1315_SCOPE_ZOOM_MIN_SPAN)
        {
            span = SSD1315_SCOPE_ZOOM_MIN_SPAN;
        }

        if (mean > (span / 2))
        {
            lower = (uint16_t)(mean - (span / 2));
        }
        else
        {
            lower = 0;
        }

        upper = (uint16_t)(lower + span);
        if (upper > 4095)
        {
            upper = 4095;
            lower = (upper > span) ? (uint16_t)(upper - span) : 0;
        }
    }

    src = g_ssd1315_wave_head;
    sample = g_ssd1315_wave_adc[(uint8_t)src];
    last_y = (g_ssd1315_scope_mode == SSD1315_SCOPE_MODE_ZOOM) ? SSD1315_MapZoom(sample, lower, upper) : SSD1315_MapGlobal(sample);
    for (x = 0; x < SSD1315_WIDTH; x++)
    {
        src = (uint16_t)g_ssd1315_wave_head + x;
        if (src >= SSD1315_WIDTH)
        {
            src -= SSD1315_WIDTH;
        }

        sample = g_ssd1315_wave_adc[(uint8_t)src];
        y = (g_ssd1315_scope_mode == SSD1315_SCOPE_MODE_ZOOM) ? SSD1315_MapZoom(sample, lower, upper) : SSD1315_MapGlobal(sample);
        SSD1315_DrawVerticalLine(x, last_y, y);
        last_y = y;
    }

    SSD1315_DrawScopeOverlay(lower, upper);
}

static uint8_t SSD1315_MapGlobal(uint16_t adc_value)
{
    if (adc_value > 4095)
    {
        adc_value = 4095;
    }

    return (uint8_t)(SSD1315_HEIGHT - 1 - (adc_value >> 6));
}

static uint8_t SSD1315_MapZoom(uint16_t adc_value, uint16_t lower, uint16_t upper)
{
    uint32_t scaled;

    if (upper <= lower)
    {
        return SSD1315_HEIGHT / 2;
    }

    if (adc_value <= lower)
    {
        return SSD1315_HEIGHT - 1;
    }

    if (adc_value >= upper)
    {
        return 0;
    }

    scaled = ((uint32_t)(adc_value - lower) * (SSD1315_HEIGHT - 1)) / (upper - lower);
    return (uint8_t)(SSD1315_HEIGHT - 1 - scaled);
}

static void SSD1315_DrawScopeOverlay(uint16_t lower, uint16_t upper)
{
    char text[9];
    char *p;
    uint32_t lower_uA;
    uint32_t upper_uA;
    uint32_t div_uA;

    lower_uA = SSD1315_AdcToCurrentMicroAmp(lower);
    upper_uA = SSD1315_AdcToCurrentMicroAmp(upper);
    div_uA = (upper_uA > lower_uA) ? ((upper_uA - lower_uA) / 8) : 0;

    SSD1315_FormatCurrent(text, upper_uA);
    SSD1315_DrawTextXor(0, 0, text);

    SSD1315_FormatCurrent(text, lower_uA);
    SSD1315_DrawTextXor(0, SSD1315_HEIGHT - 5, text);

    SSD1315_FormatTimebase(text);
    SSD1315_DrawTextXor((uint8_t)(SSD1315_WIDTH - SSD1315_TextWidth(text)), 0, text);

    SSD1315_FormatCurrent(text, div_uA);
    p = text;
    while (*p)
    {
        p++;
    }
    *p++ = '/';
    *p++ = 'd';
    *p = '\0';
    SSD1315_DrawTextXor((uint8_t)(SSD1315_WIDTH - SSD1315_TextWidth(text)), SSD1315_HEIGHT - 5, text);
}

static void SSD1315_DrawTextXor(uint8_t x, uint8_t y, char *text)
{
    uint8_t col;
    uint8_t row;
    uint8_t bits;
    char c;

    while (*text)
    {
        c = *text++;
        for (row = 0; row < 5; row++)
        {
            bits = SSD1315_GetFontRow(c, row);
            for (col = 0; col < 3; col++)
            {
                if (bits & (uint8_t)(0x04 >> col))
                {
                    SSD1315_InvertPixel((uint8_t)(x + col), (uint8_t)(y + row));
                }
            }
        }
        x += 4;
    }
}

static void SSD1315_InvertPixel(uint8_t x, uint8_t y)
{
    uint16_t index;

    if ((x >= SSD1315_WIDTH) || (y >= SSD1315_HEIGHT))
    {
        return;
    }

    index = ((uint16_t)(y >> 3) * SSD1315_WIDTH) + x;
    g_ssd1315_buffer[index] ^= (uint8_t)(1u << (y & 0x07));
}

static uint8_t SSD1315_GetFontRow(char c, uint8_t row)
{
    uint8_t rows[5];

    switch (c)
    {
    case '0': rows[0]=7; rows[1]=5; rows[2]=5; rows[3]=5; rows[4]=7; break;
    case '1': rows[0]=2; rows[1]=6; rows[2]=2; rows[3]=2; rows[4]=7; break;
    case '2': rows[0]=7; rows[1]=1; rows[2]=7; rows[3]=4; rows[4]=7; break;
    case '3': rows[0]=7; rows[1]=1; rows[2]=7; rows[3]=1; rows[4]=7; break;
    case '4': rows[0]=5; rows[1]=5; rows[2]=7; rows[3]=1; rows[4]=1; break;
    case '5': rows[0]=7; rows[1]=4; rows[2]=7; rows[3]=1; rows[4]=7; break;
    case '6': rows[0]=7; rows[1]=4; rows[2]=7; rows[3]=5; rows[4]=7; break;
    case '7': rows[0]=7; rows[1]=1; rows[2]=1; rows[3]=2; rows[4]=2; break;
    case '8': rows[0]=7; rows[1]=5; rows[2]=7; rows[3]=5; rows[4]=7; break;
    case '9': rows[0]=7; rows[1]=5; rows[2]=7; rows[3]=1; rows[4]=7; break;
    case '.': rows[0]=0; rows[1]=0; rows[2]=0; rows[3]=0; rows[4]=2; break;
    case '/': rows[0]=1; rows[1]=1; rows[2]=2; rows[3]=4; rows[4]=4; break;
    case 'm': rows[0]=0; rows[1]=7; rows[2]=7; rows[3]=5; rows[4]=5; break;
    case 'u': rows[0]=0; rows[1]=5; rows[2]=5; rows[3]=5; rows[4]=7; break;
    case 's': rows[0]=7; rows[1]=4; rows[2]=7; rows[3]=1; rows[4]=7; break;
    case 'd': rows[0]=1; rows[1]=1; rows[2]=7; rows[3]=5; rows[4]=7; break;
    default:  rows[0]=0; rows[1]=0; rows[2]=0; rows[3]=0; rows[4]=0; break;
    }

    return rows[row];
}

static uint8_t SSD1315_TextWidth(char *text)
{
    uint8_t len;

    len = 0;
    while (*text++)
    {
        len++;
    }

    return (uint8_t)(len ? ((len * 4) - 1) : 0);
}

static uint32_t SSD1315_AdcToCurrentMicroAmp(uint16_t adc_value)
{
    uint32_t input_uV;
    uint32_t denom;
    uint16_t vcc_mV;

    if (adc_value > 4095)
    {
        adc_value = 4095;
    }

    if ((CURRENT_SENSE_RES_MOHM == 0) || (CURRENT_AMP_GAIN == 0))
    {
        return 0;
    }

    vcc_mV = g_adc_vcc_mV ? g_adc_vcc_mV : 5000;
    input_uV = ((uint32_t)adc_value * vcc_mV * 1000UL) / ADC_FULL_SCALE;
    denom = (uint32_t)CURRENT_SENSE_RES_MOHM * CURRENT_AMP_GAIN;

    return ((input_uV / denom) * 1000UL) + (((input_uV % denom) * 1000UL) / denom);
}

static void SSD1315_FormatCurrent(char *buf, uint32_t current_uA)
{
    uint16_t value;

    if (current_uA < 10000UL)
    {
        value = (uint16_t)current_uA;
        buf = SSD1315_AppendU16(buf, value);
        *buf++ = 'u';
    }
    else if (current_uA < 100000UL)
    {
        value = (uint16_t)((current_uA + 50UL) / 100UL);
        buf = SSD1315_AppendU16(buf, (uint16_t)(value / 10));
        *buf++ = '.';
        *buf++ = (char)('0' + (value % 10));
        *buf++ = 'm';
    }
    else
    {
        value = (uint16_t)((current_uA + 500UL) / 1000UL);
        if (value > 9999)
        {
            value = 9999;
        }
        buf = SSD1315_AppendU16(buf, value);
        *buf++ = 'm';
    }

    *buf = '\0';
}

static void SSD1315_FormatTimebase(char *buf)
{
    uint16_t time_ms;

    time_ms = SSD1315_WAVE_SAMPLE_INTERVAL_MS * 16;
    if (time_ms < 1000)
    {
        buf = SSD1315_AppendU16(buf, time_ms);
        *buf++ = 'm';
        *buf++ = 's';
    }
    else
    {
        buf = SSD1315_AppendU16(buf, (uint16_t)(time_ms / 1000));
        *buf++ = 's';
    }

    *buf = '\0';
}

static char *SSD1315_AppendU16(char *buf, uint16_t value)
{
    uint16_t div;
    uint8_t started;

    div = 10000;
    started = 0;
    while (div > 0)
    {
        if ((value >= div) || started || (div == 1))
        {
            *buf++ = (char)('0' + (value / div));
            value %= div;
            started = 1;
        }
        div /= 10;
    }

    return buf;
}

void SSD1315_AddSample(uint16_t adc_value)
{
    if (g_ssd1315_test_active)
    {
        return;
    }

    if (adc_value > 4095)
    {
        adc_value = 4095;
    }

    g_ssd1315_wave_adc[g_ssd1315_wave_head] = adc_value;

    g_ssd1315_wave_head++;
    if (g_ssd1315_wave_head >= SSD1315_WIDTH)
    {
        g_ssd1315_wave_head = 0;
    }
}

void SSD1315_WaveTask(void)
{
    if (g_ssd1315_test_active)
    {
        return;
    }

    SSD1315_RenderWave();
    SSD1315_Flush();
}

void SSD1315_ToggleScopeMode(void)
{
    g_ssd1315_scope_mode = (g_ssd1315_scope_mode == SSD1315_SCOPE_MODE_GLOBAL) ? SSD1315_SCOPE_MODE_ZOOM : SSD1315_SCOPE_MODE_GLOBAL;
}

BOOL SSD1315_ShowTestPattern(uint8_t pattern)
{
    SSD1315_BuildTestPattern(pattern);
    return SSD1315_Flush();
}

void SSD1315_ResumeWave(void)
{
    g_ssd1315_test_active = 0;
}

static void SSD1315_BuildTestPattern(uint8_t pattern)
{
    uint16_t i;
    uint8_t x;
    uint8_t y;

    SSD1315_ClearBuffer();
    g_ssd1315_test_active = 1;

    switch (pattern)
    {
    case OLED_TEST_FULL:
        for (i = 0; i < SSD1315_BUFFER_SIZE; i++)
        {
            g_ssd1315_buffer[i] = 0xFF;
        }
        break;

    case OLED_TEST_ODD_ROWS:
        for (i = 0; i < SSD1315_BUFFER_SIZE; i++)
        {
            g_ssd1315_buffer[i] = 0xAA;
        }
        break;

    case OLED_TEST_EVEN_ROWS:
        for (i = 0; i < SSD1315_BUFFER_SIZE; i++)
        {
            g_ssd1315_buffer[i] = 0x55;
        }
        break;

    case OLED_TEST_ODD_COLS:
        for (x = 0; x < SSD1315_WIDTH; x++)
        {
            if (x & 0x01)
            {
                SSD1315_DrawVerticalLine(x, 0, SSD1315_HEIGHT - 1);
            }
        }
        break;

    case OLED_TEST_EVEN_COLS:
        for (x = 0; x < SSD1315_WIDTH; x++)
        {
            if (!(x & 0x01))
            {
                SSD1315_DrawVerticalLine(x, 0, SSD1315_HEIGHT - 1);
            }
        }
        break;

    case OLED_TEST_SINE:
        for (x = 0; x < SSD1315_WIDTH; x++)
        {
            y = g_ssd1315_sine32[x & 0x1F];
            SSD1315_DrawPoint(x, y);
        }
        break;

    case OLED_TEST_SQUARE:
        for (x = 0; x < SSD1315_WIDTH; x++)
        {
            y = (x & 0x20) ? 48 : 16;
            SSD1315_DrawPoint(x, y);
        }
        break;

    default:
        break;
    }
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


