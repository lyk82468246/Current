//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: adc.c
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
volatile uint16_t g_adc_vcc_mV = 0;
volatile uint16_t g_actual_current_adc = 0;
static volatile uint8_t g_adc_wave_sampling = 0;
static volatile uint8_t g_adc_user_convert_busy = 0;

static uint16_t ADC_ConvertInputMicroVoltToMilliAmp(uint32_t input_uV);
#if CURRENT_SAMPLE_SOURCE == CURRENT_SAMPLE_SOURCE_ADS1110
static void ADC_UpdateVccIfIdle(void);
#endif
#if CURRENT_SAMPLE_SOURCE == CURRENT_SAMPLE_SOURCE_INTERNAL
static BOOL ADC_UpdateActualCurrentInternal(void);
#endif
#if CURRENT_SAMPLE_SOURCE == CURRENT_SAMPLE_SOURCE_ADS1110
static BOOL ADC_UpdateActualCurrentADS1110(void);
#endif
//<<AICUBE_USER_GLOBAL_DEFINE_END>>


BOOL fADCConverted;                     //ADC转换完成标志

////////////////////////////////////////
// ADC初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void ADC_Init(void)
{
    ADC_SetClockDivider(0);             //设置ADC时钟
    ADC_ResultRightAlign();             //设置ADC结果右对齐(12位结果)
    ADC_SetRepeat16Times();             //ADC自动重复转换16次并取平均值

    ADC_SetCSSetupCycles(ADC_STPCYC);   //设置ADC通道选择建立时间
    ADC_SetCSHoldCycles(ADC_HLDCYC);    //设置ADC通道选择保持时间
    ADC_SetSampleDutyCycles(ADC_SMPCYC); //设置ADC通道采样时间

    ADC_DisableETR();                   //禁止ADC外部触发功能

    ADC_SetIntPriority(0);              //设置中断为最低优先级
    ADC_EnableInt();                    //使能ADC中断
    fADCConverted = 0;                  //初始化转换结束标志

    ADC_ActiveChannel(0);               //选择ADC通道
    ADC_Enable();                       //使能ADC功能

    //<<AICUBE_USER_ADC_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_ADC_INITIAL_END>>
}

////////////////////////////////////////
// 获取ADC转换结果函数
// 入口参数: ch (ADC通道选择)
// 函数返回: ADC转换结果
////////////////////////////////////////
uint16_t ADC_Convert(uint8_t ch)
{
    uint16_t res;                       //定义保存ADC结果的变量

    ADC_ActiveChannel(ch);              //选择ADC通道
    ADC_Start();                        //开始ADC转换
    while (!fADCConverted);             //等待ADC转换完成
    fADCConverted = 0;                  //清除ADC转换结束标志
    res = ADC_ReadResult();             //读取ADC转换结果

    return res;                         //返回ADC结果
}


////////////////////////////////////////
// ADC中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void ADC_ISR(void) interrupt ADC_VECTOR
{
    //<<AICUBE_USER_ADC_ISR_CODE1_BEGIN>>
    if (g_adc_wave_sampling)
    {
        ADC_ClearFlag();
        SSD1315_AddSample(ADC_ReadResult());
        g_adc_wave_sampling = 0;
        return;
    }

    // 在此添加中断函数用户代码  
    ADC_ClearFlag();                    //清除ADC转换完成中断标志
    fADCConverted = 1;                  //设置转换结束标志
    //<<AICUBE_USER_ADC_ISR_CODE1_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
void ADC_StartWaveSample(void)
{
    if (g_adc_wave_sampling || g_adc_user_convert_busy)
    {
        return;
    }

    fADCConverted = 0;
    g_adc_wave_sampling = 1;
    ADC_ActiveChannel(ADC_WAVE_CHANNEL);
    ADC_Start();
}

// 在此添加用户函数实现代码  
uint16_t ADC_ReadVccMilliVolt(void)
{
    uint16_t vref_adc;
    uint32_t vcc_mV;

    vref_adc = ADC_Convert(ADC_VREF_CHANNEL);
    if (vref_adc == 0)
    {
        return 0;
    }

    vcc_mV = (ADC_INTERNAL_REF_MV * ADC_FULL_SCALE) / vref_adc;
    if (vcc_mV > 65535UL)
    {
        vcc_mV = 65535UL;
    }

    return (uint16_t)vcc_mV;
}

uint16_t ADC_ConvertCurrentMilliAmp(uint16_t adc_value, uint16_t vcc_mV)
{
    uint32_t input_uV;

    if ((CURRENT_SENSE_RES_MOHM == 0) || (CURRENT_AMP_GAIN_NUM == 0))
    {
        return 0;
    }

    input_uV = ((uint32_t)adc_value * vcc_mV * 1000UL) / ADC_FULL_SCALE;
    return ADC_ConvertInputMicroVoltToMilliAmp(input_uV);
}

static uint16_t ADC_ConvertInputMicroVoltToMilliAmp(uint32_t input_uV)
{
    uint32_t denom;
    uint32_t current_mA;

    if ((CURRENT_SENSE_RES_MOHM == 0) || (CURRENT_AMP_GAIN_NUM == 0))
    {
        return 0;
    }

    denom = CURRENT_SENSE_RES_MOHM * CURRENT_AMP_GAIN_NUM;
    current_mA = ((input_uV * CURRENT_AMP_GAIN_DEN) + (denom / 2)) / denom;

    if (current_mA > 9999UL)
    {
        current_mA = 9999UL;
    }

    return (uint16_t)current_mA;
}

#if CURRENT_SAMPLE_SOURCE == CURRENT_SAMPLE_SOURCE_ADS1110
static void ADC_UpdateVccIfIdle(void)
{
    uint16_t vcc_mV;

    if (g_adc_wave_sampling || g_adc_user_convert_busy)
    {
        return;
    }

    g_adc_user_convert_busy = 1;
    vcc_mV = ADC_ReadVccMilliVolt();
    if (vcc_mV)
    {
        g_adc_vcc_mV = vcc_mV;
    }
    g_adc_user_convert_busy = 0;
}
#endif

#if CURRENT_SAMPLE_SOURCE == CURRENT_SAMPLE_SOURCE_INTERNAL
static BOOL ADC_UpdateActualCurrentInternal(void)
{
    uint16_t vcc_mV;
    uint16_t adc_value;
    uint16_t current_mA;

    if (g_adc_wave_sampling || g_adc_user_convert_busy)
    {
        return FALSE;
    }
    g_adc_user_convert_busy = 1;

    vcc_mV = ADC_ReadVccMilliVolt();
    if (vcc_mV == 0)
    {
        g_adc_user_convert_busy = 0;
        return FALSE;
    }

    adc_value = ADC_Convert(ADC_CURRENT_CHANNEL);
    current_mA = ADC_ConvertCurrentMilliAmp(adc_value, vcc_mV);

    g_adc_vcc_mV = vcc_mV;
    g_actual_current_adc = adc_value;
    SEG_UpdateMemory(SEG_GROUP_ACTUAL_CURRENT, current_mA);
    g_adc_user_convert_busy = 0;

    return TRUE;
}
#endif

#if CURRENT_SAMPLE_SOURCE == CURRENT_SAMPLE_SOURCE_ADS1110
static BOOL ADC_UpdateActualCurrentADS1110(void)
{
    int16_t raw;
    uint8_t config;
    int32_t input_uV;
    uint16_t current_mA;

    ADC_UpdateVccIfIdle();

    if (!ADS1110_ReadRaw(&raw, &config))
    {
        return FALSE;
    }

    if (!ADS1110_IsDataReady(config))
    {
        return FALSE;
    }

    if (raw <= 0)
    {
        input_uV = 0;
        g_actual_current_adc = 0;
    }
    else
    {
        input_uV = ADS1110_RawToMicroVolt(raw, config);
        if (input_uV < 0)
        {
            input_uV = 0;
        }
        g_actual_current_adc = (uint16_t)raw;
    }

    current_mA = ADC_ConvertInputMicroVoltToMilliAmp((uint32_t)input_uV);
    SEG_UpdateMemory(SEG_GROUP_ACTUAL_CURRENT, current_mA);

    return TRUE;
}
#endif

BOOL ADC_UpdateActualCurrentTask(void)
{
#if CURRENT_SAMPLE_SOURCE == CURRENT_SAMPLE_SOURCE_ADS1110
    return ADC_UpdateActualCurrentADS1110();
#else
    return ADC_UpdateActualCurrentInternal();
#endif
}

//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


