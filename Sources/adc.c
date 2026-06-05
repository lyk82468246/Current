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
//<<AICUBE_USER_GLOBAL_DEFINE_END>>



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
    while (!ADC_CheckFlag());           //等待ADC转换完成
    ADC_ClearFlag();                    //清除ADC转换完成中断标志
    res = ADC_ReadResult();             //读取ADC转换结果

    return res;                         //返回ADC结果
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
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
    uint32_t current_mA;

    if ((CURRENT_SENSE_RES_MOHM == 0) || (CURRENT_AMP_GAIN == 0))
    {
        return 0;
    }

    input_uV = ((uint32_t)adc_value * vcc_mV * 1000UL) / ADC_FULL_SCALE;
    current_mA = input_uV / (CURRENT_AMP_GAIN * CURRENT_SENSE_RES_MOHM);

    if (current_mA > 9999UL)
    {
        current_mA = 9999UL;
    }

    return (uint16_t)current_mA;
}

void ADC_UpdateActualCurrentTask(void)
{
    uint16_t vcc_mV;
    uint16_t adc_value;
    uint16_t current_mA;

    vcc_mV = ADC_ReadVccMilliVolt();
    if (vcc_mV == 0)
    {
        return;
    }

    adc_value = ADC_Convert(ADC_CURRENT_CHANNEL);
    current_mA = ADC_ConvertCurrentMilliAmp(adc_value, vcc_mV);

    g_adc_vcc_mV = vcc_mV;
    g_actual_current_adc = adc_value;
    SEG_UpdateMemory(SEG_GROUP_ACTUAL_CURRENT, current_mA);
}

//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


