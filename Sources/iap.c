//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: iap.c
// 文件描述: 
// 文件版本: V1.0
// 修改记录:
//   1. (2026-06-06) 创建文件
////////////////////////////////////////
//<<AICUBE_USER_HEADER_REMARK_END>>


#include "config.h"


//<<AICUBE_USER_INCLUDE_BEGIN>>
// 在此添加用户头文件包含  
//<<AICUBE_USER_INCLUDE_END>>


//<<AICUBE_USER_GLOBAL_DEFINE_BEGIN>>
// 在此添加用户全局变量定义、用户宏定义以及函数声明  
#define PID_STORE_MAGIC0        'P'
#define PID_STORE_MAGIC1        'I'
#define PID_STORE_MAGIC2        'D'
#define PID_STORE_MAGIC3        '1'
#define PID_STORE_VERSION       1
#define PID_STORE_SIZE          18

volatile int32_t g_pid_kp = PID_DEFAULT_KP;
volatile int32_t g_pid_ki = PID_DEFAULT_KI;
volatile int32_t g_pid_kd = PID_DEFAULT_KD;
volatile int32_t g_pid_error = 0;
volatile int32_t g_pid_output = 0;
volatile uint16_t g_dac_code = 0;

static int32_t g_pid_integral = 0;
static int32_t g_pid_last_error = 0;

static void PID_WriteLong(uint32_t addr, int32_t value);
static int32_t PID_ReadLong(uint32_t addr);
static uint8_t PID_ReadStoreByte(uint8_t offset);
static uint8_t PID_CalcChecksum(void);
static uint16_t PID_OutputToDacCode(int32_t output);
//<<AICUBE_USER_GLOBAL_DEFINE_END>>



////////////////////////////////////////
// IAP初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void IAP_Init(void)
{
    IAP_Enable();                       //使能EEPROM操作
    IAP_Idle();                         //设置EEPROM为空闲模式

    //<<AICUBE_USER_IAP_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    PID_Init();
    //<<AICUBE_USER_IAP_INITIAL_END>>
}
////////////////////////////////////////
// EEPROM扇区擦除函数
// 入口参数: dwAddress (EEPROM目标扇区地址)
// 函数返回: 无
////////////////////////////////////////
void IAP_EraseSector(uint32_t dwAddress)
{
    IAP_SetAddress(dwAddress);          //设置EEPROM目标地址
    IAP_TriggerErase();                 //触发EEPROM扇区擦除
    IAP_Idle();                         //恢复EEPROM空闲模式
}

////////////////////////////////////////
// EEPROM字节编程函数
// 入口参数: dwAddress (EEPROM目标字节地址)
//           bData (待写入的字节数据)
// 函数返回: 无
////////////////////////////////////////
void IAP_ProgramByte(uint32_t dwAddress, uint8_t bData)
{
    IAP_SetAddress(dwAddress);          //设置EEPROM目标地址
    IAP_SetData(bData);                 //设置EEPROM数据
    IAP_TriggerProgram();               //触发EEPROM字节编程
    IAP_Idle();                         //恢复EEPROM空闲模式
}

////////////////////////////////////////
// EEPROM字节读取函数
// 入口参数: dwAddress (EEPROM目标字节地址)
// 函数返回: (读取的字节数据)
////////////////////////////////////////
uint8_t IAP_ReadByte(uint32_t dwAddress)
{
    uint8_t dat;

    IAP_SetAddress(dwAddress);          //设置EEPROM目标地址
    IAP_TriggerRead();                  //触发EEPROM字节读取
    dat = IAP_ReadData();               //保存EEPROM数据
    IAP_Idle();                         //恢复EEPROM空闲模式

    return dat;                         //返回读取的数据
}



//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
static void PID_WriteLong(uint32_t addr, int32_t value)
{
    uint32_t data_value;

    data_value = (uint32_t)value;
    IAP_ProgramByte(addr, BYTE0(data_value));
    IAP_ProgramByte(addr + 1, BYTE1(data_value));
    IAP_ProgramByte(addr + 2, BYTE2(data_value));
    IAP_ProgramByte(addr + 3, BYTE3(data_value));
}

static int32_t PID_ReadLong(uint32_t addr)
{
    uint32_t data_value;

    data_value = (uint32_t)IAP_ReadByte(addr);
    data_value |= ((uint32_t)IAP_ReadByte(addr + 1) << 8);
    data_value |= ((uint32_t)IAP_ReadByte(addr + 2) << 16);
    data_value |= ((uint32_t)IAP_ReadByte(addr + 3) << 24);

    return (int32_t)data_value;
}

static uint8_t PID_ReadStoreByte(uint8_t offset)
{
    return IAP_ReadByte(PID_IAP_ADDRESS + offset);
}

static uint8_t PID_CalcChecksum(void)
{
    uint8_t i;
    uint8_t sum;

    sum = 0;
    for (i = 0; i < (PID_STORE_SIZE - 1); i++)
    {
        sum += PID_ReadStoreByte(i);
    }

    return sum;
}

static uint16_t PID_OutputToDacCode(int32_t output)
{
    if (output <= 0)
    {
        return 0;
    }

    if (output > DAC8311_MAX_CODE)
    {
        return DAC8311_MAX_CODE;
    }

    return (uint16_t)output;
}

void PID_ResetState(void)
{
    g_pid_integral = 0;
    g_pid_last_error = 0;
    g_pid_error = 0;
    g_pid_output = 0;
    g_dac_code = 0;
}

void PID_LoadDefault(void)
{
    g_pid_kp = PID_DEFAULT_KP;
    g_pid_ki = PID_DEFAULT_KI;
    g_pid_kd = PID_DEFAULT_KD;
    PID_ResetState();
}

BOOL PID_LoadFromIAP(void)
{
    uint8_t checksum;

    if ((PID_ReadStoreByte(0) != PID_STORE_MAGIC0) ||
        (PID_ReadStoreByte(1) != PID_STORE_MAGIC1) ||
        (PID_ReadStoreByte(2) != PID_STORE_MAGIC2) ||
        (PID_ReadStoreByte(3) != PID_STORE_MAGIC3) ||
        (PID_ReadStoreByte(4) != PID_STORE_VERSION))
    {
        return FALSE;
    }

    checksum = PID_CalcChecksum();
    if (checksum != PID_ReadStoreByte(PID_STORE_SIZE - 1))
    {
        return FALSE;
    }

    g_pid_kp = PID_ReadLong(PID_IAP_ADDRESS + 5);
    g_pid_ki = PID_ReadLong(PID_IAP_ADDRESS + 9);
    g_pid_kd = PID_ReadLong(PID_IAP_ADDRESS + 13);
    PID_ResetState();

    return TRUE;
}

BOOL PID_SaveToIAP(void)
{
    uint8_t i;
    uint8_t checksum;

    DisableGlobalInt();

    IAP_ClearErrorFlag();
    IAP_EraseSector(PID_IAP_ADDRESS);
    IAP_ProgramByte(PID_IAP_ADDRESS + 0, PID_STORE_MAGIC0);
    IAP_ProgramByte(PID_IAP_ADDRESS + 1, PID_STORE_MAGIC1);
    IAP_ProgramByte(PID_IAP_ADDRESS + 2, PID_STORE_MAGIC2);
    IAP_ProgramByte(PID_IAP_ADDRESS + 3, PID_STORE_MAGIC3);
    IAP_ProgramByte(PID_IAP_ADDRESS + 4, PID_STORE_VERSION);
    PID_WriteLong(PID_IAP_ADDRESS + 5, g_pid_kp);
    PID_WriteLong(PID_IAP_ADDRESS + 9, g_pid_ki);
    PID_WriteLong(PID_IAP_ADDRESS + 13, g_pid_kd);

    checksum = 0;
    for (i = 0; i < (PID_STORE_SIZE - 1); i++)
    {
        checksum += IAP_ReadByte(PID_IAP_ADDRESS + i);
    }
    IAP_ProgramByte(PID_IAP_ADDRESS + (PID_STORE_SIZE - 1), checksum);

    EnableGlobalInt();

    return !IAP_CheckErrorFlag();
}

void PID_Init(void)
{
    if (!PID_LoadFromIAP())
    {
        PID_LoadDefault();
    }
}

BOOL PID_SetParam(char param, int32_t value)
{
    if ((param == 'P') || (param == 'p'))
    {
        g_pid_kp = value;
        PID_ResetState();
        return TRUE;
    }

    if ((param == 'I') || (param == 'i'))
    {
        g_pid_ki = value;
        PID_ResetState();
        return TRUE;
    }

    if ((param == 'D') || (param == 'd'))
    {
        g_pid_kd = value;
        PID_ResetState();
        return TRUE;
    }

    return FALSE;
}

void PID_ControlTask(void)
{
    int32_t error;
    int32_t derivative;
    int32_t output;

    error = (int32_t)g_current_set_mA - (int32_t)g_actual_current_mA;
    g_pid_integral += error;
    if (g_pid_integral > PID_INTEGRAL_LIMIT)
    {
        g_pid_integral = PID_INTEGRAL_LIMIT;
    }
    else if (g_pid_integral < -PID_INTEGRAL_LIMIT)
    {
        g_pid_integral = -PID_INTEGRAL_LIMIT;
    }

    derivative = error - g_pid_last_error;
    g_pid_last_error = error;

    output = ((g_pid_kp * error) +
              (g_pid_ki * g_pid_integral) +
              (g_pid_kd * derivative)) / PID_SCALE;

    g_pid_error = error;
    g_pid_output = output;
    g_dac_code = PID_OutputToDacCode(output);
    DAC8311_WriteCode(g_dac_code);
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


