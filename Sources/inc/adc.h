//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: adc.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __ADC_H__
#define __ADC_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
#define ADC_WAVE_CHANNEL                0
#define ADC_CURRENT_CHANNEL             0
#define ADC_VREF_CHANNEL                15
#define ADC_INTERNAL_REF_MV             1190UL
#define ADC_FULL_SCALE                  4096UL
#define CURRENT_SENSE_RES_MOHM          100UL
#define CURRENT_AMP_GAIN                20UL

//<<AICUBE_USER_DEFINE_END>>


#define ADC_STPCYC              0       //ADC通道选择建立时间
#define ADC_HLDCYC              1       //ADC通道选择保持时间
#define ADC_SMPCYC              9       //ADC通道采样时间


void ADC_Init(void);
uint16_t ADC_Convert(uint8_t ch);



//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
extern volatile uint16_t g_adc_vcc_mV;
extern volatile uint16_t g_actual_current_adc;

uint16_t ADC_ReadVccMilliVolt(void);
uint16_t ADC_ConvertCurrentMilliAmp(uint16_t adc_value, uint16_t vcc_mV);
void ADC_UpdateActualCurrentTask(void);
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
