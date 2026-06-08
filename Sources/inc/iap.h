//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: iap.h
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#ifndef __IAP_H__
#define __IAP_H__


//<<AICUBE_USER_DEFINE_BEGIN>>
// 在此添加用户宏定义  
#define PID_SCALE                       1000L
#define PID_DEFAULT_KP                  0L
#define PID_DEFAULT_KI                  0L
#define PID_DEFAULT_KD                  0L
#define PID_INTEGRAL_LIMIT              100000L
#define PID_IAP_ADDRESS                 IAP_OFFSET

#define DAC8311_REF_MV                  2000UL
#define FF_BASIC_POINTS                 21
#define FF_PRO_POINTS                   21
//<<AICUBE_USER_DEFINE_END>>


#define IAP_OFFSET              0xfc00  //定义EEPROM起始地址


void IAP_Init(void);
void IAP_EraseSector(uint32_t dwAddress);
void IAP_ProgramByte(uint32_t dwAddress, uint8_t bData);
uint8_t IAP_ReadByte(uint32_t dwAddress);



//<<AICUBE_USER_EXTERNAL_DECLARE_BEGIN>>
// 在此添加用户外部函数和外部变量声明  
extern volatile int32_t g_pid_kp;
extern volatile int32_t g_pid_ki;
extern volatile int32_t g_pid_kd;
extern volatile int32_t g_pid_error;
extern volatile int32_t g_pid_output;
extern volatile uint16_t g_dac_code;
extern volatile uint16_t g_dac_feedforward_code;
extern volatile uint8_t g_pid_enabled;

void PID_Init(void);
void PID_LoadDefault(void);
BOOL PID_LoadFromIAP(void);
BOOL PID_SaveToIAP(void);
BOOL PID_SetParam(char param, int32_t value);
void PID_SetEnabled(uint8_t enabled);
void PID_ResetState(void);
void PID_ControlTask(void);
void PID_SetpointTask(uint8_t mode, uint16_t current_mA);
uint16_t FF_GetCurrentCode(void);
BOOL FF_SetCurrentCode(uint16_t dac_code);
BOOL FF_AdjustCurrentCode(int32_t delta);
//<<AICUBE_USER_EXTERNAL_DECLARE_END>>


#endif
