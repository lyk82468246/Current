//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: i2c.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __I2C_H__
#define __I2C_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
#define ADS1110_I2C_ADDR                0x48
#define ADS1110_SLAW                    ((ADS1110_I2C_ADDR << 1) | 0)
#define ADS1110_SLAR                    ((ADS1110_I2C_ADDR << 1) | 1)

#define ADS1110_CFG_ST_DRDY             0x80
#define ADS1110_CFG_SINGLE              0x10
#define ADS1110_CFG_CONTINUOUS          0x00

#define ADS1110_CFG_DR_240SPS_12BIT     0x00
#define ADS1110_CFG_DR_60SPS_14BIT      0x04
#define ADS1110_CFG_DR_30SPS_15BIT      0x08
#define ADS1110_CFG_DR_15SPS_16BIT      0x0c
#define ADS1110_CFG_DR_MASK             0x0c

#define ADS1110_CFG_PGA_1               0x00
#define ADS1110_CFG_PGA_2               0x01
#define ADS1110_CFG_PGA_4               0x02
#define ADS1110_CFG_PGA_8               0x03
#define ADS1110_CFG_PGA_MASK            0x03

#define ADS1110_DEFAULT_PGA             ADS1110_CFG_PGA_2
#define ADS1110_CONFIG_DEFAULT          (ADS1110_CFG_CONTINUOUS | ADS1110_CFG_DR_15SPS_16BIT | ADS1110_DEFAULT_PGA)
#define ADS1110_CONFIG_SINGLE_16BIT     (ADS1110_CFG_ST_DRDY | ADS1110_CFG_SINGLE | ADS1110_CFG_DR_15SPS_16BIT | ADS1110_DEFAULT_PGA)
#define ADS1110_IsDataReady(config)     (((config) & ADS1110_CFG_ST_DRDY) == 0)

#define SSD1315_I2C_ADDR                0x3c
#define SSD1315_SLAW                    ((SSD1315_I2C_ADDR << 1) | 0)
#define SSD1315_WIDTH                   128
#define SSD1315_HEIGHT                  64
#define SSD1315_PAGE_COUNT              8
#define SSD1315_BUFFER_SIZE             (SSD1315_WIDTH * SSD1315_PAGE_COUNT)
#define SSD1315_WAVE_SAMPLE_INTERVAL_MS 5
#define SSD1315_EXT_SAMPLE_INTERVAL_MS  67
#define SSD1315_USE_I2C_DMA             1
#define SSD1315_SCOPE_MODE_GLOBAL       0
#define SSD1315_SCOPE_MODE_ZOOM         1
#define SSD1315_SCOPE_ZOOM_MIN_SPAN     64
#define SSD1315_SCOPE_ZOOM_MIN_DMA      10

#define I2C_DMARXSIZE                   1024
//<<AICUBE_USER_DEFINE_END>>


#define I2C_DMATXSIZE           1024    //I2C DMA发送缓冲区大小
#define I2C_DMAITV              0       //I2C DMA发送/接收字节间隔时间（系统时钟）
#define I2C_SLA             0           //I2C 从机地址
#define I2C_SLAR                ((I2C_SLA << 1) | 1) //I2C 从机读地址
#define I2C_SLAW                ((I2C_SLA << 1)) //I2C 从机写地址


void I2C_Init(void);
void I2C_MasterWait(void);
void I2C_MasterStart(void);
void I2C_MasterStop(void);
BOOL I2C_MasterStartSendCode(uint8_t dat);
BOOL I2C_MasterSendByte(uint8_t dat);
uint8_t I2C_MasterReadByte(BOOL ack);
void I2C_DMA_MasterTriggerSend(uint16_t cnt);
void I2C_DMA_MasterTriggerRead(uint16_t cnt);
void I2C_DMA_ReadAT24C02(uint8_t addr, uint8_t *dat, uint8_t size);
void I2C_DMA_WriteAT24C02(uint8_t addr, uint8_t *dat, uint8_t size);

extern uint8_t xdata pu8I2CDMATxBuffer[I2C_DMATXSIZE];


//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
BOOL I2C_MasterStartSendByte(uint8_t dat);

BOOL ADS1110_Init(void);
BOOL ADS1110_WriteConfig(uint8_t config);
BOOL ADS1110_StartSingle(uint8_t config);
BOOL ADS1110_ReadRaw(int16_t *raw, uint8_t *config);
BOOL ADS1110_ReadRaw16(int16_t *raw);
int32_t ADS1110_RawToMicroVolt(int16_t raw, uint8_t config);

BOOL SSD1315_Init(void);
BOOL SSD1315_Probe(uint8_t addr);
void SSD1315_SetAddress(uint8_t addr);
void SSD1315_ClearBuffer(void);
BOOL SSD1315_Flush(void);
void SSD1315_AddSample(uint16_t adc_value);
void SSD1315_AddCurrentSample(uint16_t current_dmA);
void SSD1315_WaveTask(void);
void SSD1315_ToggleScopeMode(void);
BOOL SSD1315_ShowTestPattern(uint8_t pattern);
void SSD1315_ResumeWave(void);
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
