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
#define PID_STORE_MAGIC3        '3'
#define PID_STORE_VERSION       3
#define PID_STORE_KP_OFFSET     5
#define PID_STORE_KI_OFFSET     9
#define PID_STORE_KD_OFFSET     13
#define PID_STORE_ENABLE_OFFSET 17
#define PID_STORE_BASIC_OFFSET  18
#define PID_STORE_PRO_OFFSET    (PID_STORE_BASIC_OFFSET + (FF_BASIC_POINTS * 2))
#define PID_STORE_CHECK_OFFSET  (PID_STORE_PRO_OFFSET + (FF_PRO_POINTS * 2))
#define PID_STORE_SIZE          (PID_STORE_CHECK_OFFSET + 1)

volatile int32_t g_pid_kp = PID_DEFAULT_KP;
volatile int32_t g_pid_ki = PID_DEFAULT_KI;
volatile int32_t g_pid_kd = PID_DEFAULT_KD;
volatile int32_t g_pid_error = 0;
volatile int32_t g_pid_output = 0;
volatile uint16_t g_dac_code = 0;
volatile uint16_t g_dac_feedforward_code = 0;
volatile uint8_t g_pid_enabled = 1;

static int32_t g_pid_integral = 0;
static int32_t g_pid_last_error = 0;
static uint16_t xdata g_ff_basic_table[FF_BASIC_POINTS];
static uint16_t xdata g_ff_pro_table[FF_PRO_POINTS];
static uint8_t g_setpoint_valid = 0;
static uint8_t g_last_setpoint_mode = 0;
static uint16_t g_last_setpoint_mA = 0;

static void PID_WriteLong(uint32_t addr, int32_t value);
static int32_t PID_ReadLong(uint32_t addr);
static void PID_WriteWord(uint32_t addr, uint16_t value);
static uint16_t PID_ReadWord(uint32_t addr);
static uint8_t PID_ReadStoreByte(uint8_t offset);
static uint8_t PID_CalcChecksum(void);
static void FF_InitIdealTables(void);
static uint16_t FF_CurrentToIdealCode(uint16_t current_mA);
static uint8_t FF_GetBasicIndex(uint16_t current_mA);
static uint8_t FF_GetProIndex(uint16_t current_mA);
static uint16_t FF_GetCode(uint8_t mode, uint16_t current_mA);
static BOOL FF_SetCode(uint8_t mode, uint16_t current_mA, uint16_t dac_code);
static void PID_InvalidateSetpoint(void);
static uint16_t PID_WriteDacWithFeedback(int32_t feedback);
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

static void PID_WriteWord(uint32_t addr, uint16_t value)
{
    IAP_ProgramByte(addr, LOBYTE(value));
    IAP_ProgramByte(addr + 1, HIBYTE(value));
}

static uint16_t PID_ReadWord(uint32_t addr)
{
    uint16_t value;

    value = (uint16_t)IAP_ReadByte(addr);
    value |= ((uint16_t)IAP_ReadByte(addr + 1) << 8);

    return value;
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

static uint16_t FF_CurrentToIdealCode(uint16_t current_mA)
{
    uint32_t input_uV;
    uint32_t ref_unit;
    uint32_t input_unit;
    uint32_t dac_value;

    input_uV = ((uint32_t)current_mA * CURRENT_SENSE_RES_MOHM * CURRENT_AMP_GAIN_NUM) / CURRENT_AMP_GAIN_DEN;
    ref_unit = DAC8311_REF_MV * 10UL;
    input_unit = input_uV / 100UL;
    if (ref_unit == 0)
    {
        return 0;
    }

    dac_value = ((input_unit * (DAC8311_MAX_CODE + 1UL)) + (ref_unit / 2)) / ref_unit;
    if (dac_value > DAC8311_MAX_CODE)
    {
        dac_value = DAC8311_MAX_CODE;
    }

    return (uint16_t)dac_value;
}

static void FF_InitIdealTables(void)
{
    uint8_t i;

    g_ff_basic_table[0] = FF_CurrentToIdealCode(0);
    for (i = 1; i < FF_BASIC_POINTS; i++)
    {
        g_ff_basic_table[i] = FF_CurrentToIdealCode((uint16_t)(i * 50));
    }

    g_ff_pro_table[0] = FF_CurrentToIdealCode(0);
    for (i = 1; i < FF_PRO_POINTS; i++)
    {
        g_ff_pro_table[i] = FF_CurrentToIdealCode((uint16_t)(100 + ((i - 1) * 100)));
    }
}

static uint8_t FF_GetBasicIndex(uint16_t current_mA)
{
    if (current_mA < 50)
    {
        return 0;
    }

    if (current_mA > 1000)
    {
        current_mA = 1000;
    }

    return (uint8_t)(((current_mA - 50) / 50) + 1);
}

static uint8_t FF_GetProIndex(uint16_t current_mA)
{
    if (current_mA < 100)
    {
        return 0;
    }

    if (current_mA > 2000)
    {
        current_mA = 2000;
    }

    return (uint8_t)(((current_mA - 100) / 100) + 1);
}

static uint16_t FF_GetCode(uint8_t mode, uint16_t current_mA)
{
    if (mode == 0)
    {
        return g_ff_basic_table[FF_GetBasicIndex(current_mA)];
    }

    return g_ff_pro_table[FF_GetProIndex(current_mA)];
}

static BOOL FF_SetCode(uint8_t mode, uint16_t current_mA, uint16_t dac_code)
{
    if (dac_code > DAC8311_MAX_CODE)
    {
        return FALSE;
    }

    if (mode == 0)
    {
        g_ff_basic_table[FF_GetBasicIndex(current_mA)] = dac_code;
    }
    else
    {
        g_ff_pro_table[FF_GetProIndex(current_mA)] = dac_code;
    }

    PID_InvalidateSetpoint();
    return TRUE;
}

static void PID_InvalidateSetpoint(void)
{
    g_setpoint_valid = 0;
}

static uint16_t PID_WriteDacWithFeedback(int32_t feedback)
{
    int32_t dac_value;

    dac_value = (int32_t)g_dac_feedforward_code + feedback;
    if (dac_value < 0)
    {
        dac_value = 0;
    }
    else if (dac_value > DAC8311_MAX_CODE)
    {
        dac_value = DAC8311_MAX_CODE;
    }

    g_dac_code = (uint16_t)dac_value;
    DAC8311_WriteCode(g_dac_code);

    return g_dac_code;
}

void PID_ResetState(void)
{
    g_pid_integral = 0;
    g_pid_last_error = 0;
    g_pid_error = 0;
    g_pid_output = 0;
}

void PID_LoadDefault(void)
{
    g_pid_kp = PID_DEFAULT_KP;
    g_pid_ki = PID_DEFAULT_KI;
    g_pid_kd = PID_DEFAULT_KD;
    g_pid_enabled = 1;
    FF_InitIdealTables();
    PID_InvalidateSetpoint();
    PID_ResetState();
}

BOOL PID_LoadFromIAP(void)
{
    uint8_t checksum;
    uint8_t i;

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

    g_pid_kp = PID_ReadLong(PID_IAP_ADDRESS + PID_STORE_KP_OFFSET);
    g_pid_ki = PID_ReadLong(PID_IAP_ADDRESS + PID_STORE_KI_OFFSET);
    g_pid_kd = PID_ReadLong(PID_IAP_ADDRESS + PID_STORE_KD_OFFSET);
    g_pid_enabled = PID_ReadStoreByte(PID_STORE_ENABLE_OFFSET) ? 1 : 0;

    for (i = 0; i < FF_BASIC_POINTS; i++)
    {
        g_ff_basic_table[i] = PID_ReadWord(PID_IAP_ADDRESS + PID_STORE_BASIC_OFFSET + ((uint16_t)i * 2));
        if (g_ff_basic_table[i] > DAC8311_MAX_CODE)
        {
            return FALSE;
        }
    }

    for (i = 0; i < FF_PRO_POINTS; i++)
    {
        g_ff_pro_table[i] = PID_ReadWord(PID_IAP_ADDRESS + PID_STORE_PRO_OFFSET + ((uint16_t)i * 2));
        if (g_ff_pro_table[i] > DAC8311_MAX_CODE)
        {
            return FALSE;
        }
    }

    PID_InvalidateSetpoint();
    PID_ResetState();

    return TRUE;
}

BOOL PID_SaveToIAP(void)
{
    uint8_t i;
    uint8_t checksum;
    uint32_t addr;

    DisableGlobalInt();

    IAP_ClearErrorFlag();
    IAP_EraseSector(PID_IAP_ADDRESS);
    IAP_ProgramByte(PID_IAP_ADDRESS + 0, PID_STORE_MAGIC0);
    IAP_ProgramByte(PID_IAP_ADDRESS + 1, PID_STORE_MAGIC1);
    IAP_ProgramByte(PID_IAP_ADDRESS + 2, PID_STORE_MAGIC2);
    IAP_ProgramByte(PID_IAP_ADDRESS + 3, PID_STORE_MAGIC3);
    IAP_ProgramByte(PID_IAP_ADDRESS + 4, PID_STORE_VERSION);
    PID_WriteLong(PID_IAP_ADDRESS + PID_STORE_KP_OFFSET, g_pid_kp);
    PID_WriteLong(PID_IAP_ADDRESS + PID_STORE_KI_OFFSET, g_pid_ki);
    PID_WriteLong(PID_IAP_ADDRESS + PID_STORE_KD_OFFSET, g_pid_kd);
    IAP_ProgramByte(PID_IAP_ADDRESS + PID_STORE_ENABLE_OFFSET, g_pid_enabled ? 1 : 0);

    addr = PID_IAP_ADDRESS + PID_STORE_BASIC_OFFSET;
    for (i = 0; i < FF_BASIC_POINTS; i++)
    {
        PID_WriteWord(addr, g_ff_basic_table[i]);
        addr += 2;
    }

    addr = PID_IAP_ADDRESS + PID_STORE_PRO_OFFSET;
    for (i = 0; i < FF_PRO_POINTS; i++)
    {
        PID_WriteWord(addr, g_ff_pro_table[i]);
        addr += 2;
    }

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
        PID_WriteDacWithFeedback(0);
        return TRUE;
    }

    if ((param == 'I') || (param == 'i'))
    {
        g_pid_ki = value;
        PID_ResetState();
        PID_WriteDacWithFeedback(0);
        return TRUE;
    }

    if ((param == 'D') || (param == 'd'))
    {
        g_pid_kd = value;
        PID_ResetState();
        PID_WriteDacWithFeedback(0);
        return TRUE;
    }

    return FALSE;
}

void PID_SetEnabled(uint8_t enabled)
{
    g_pid_enabled = enabled ? 1 : 0;
    PID_ResetState();
    PID_WriteDacWithFeedback(0);
}

void PID_SetpointTask(uint8_t mode, uint16_t current_mA)
{
    if (!g_setpoint_valid ||
        (g_last_setpoint_mode != mode) ||
        (g_last_setpoint_mA != current_mA))
    {
        g_last_setpoint_mode = mode;
        g_last_setpoint_mA = current_mA;
        g_setpoint_valid = 1;

        g_dac_feedforward_code = FF_GetCode(mode, current_mA);
        PID_ResetState();
        PID_WriteDacWithFeedback(0);
    }
}

uint16_t FF_GetCurrentCode(void)
{
    return FF_GetCode(g_current_mode, g_current_set_mA);
}

BOOL FF_SetCurrentCode(uint16_t dac_code)
{
    return FF_SetCode(g_current_mode, g_current_set_mA, dac_code);
}

BOOL FF_AdjustCurrentCode(int32_t delta)
{
    int32_t dac_value;

    dac_value = (int32_t)FF_GetCurrentCode() + delta;
    if (dac_value < 0)
    {
        dac_value = 0;
    }
    else if (dac_value > DAC8311_MAX_CODE)
    {
        dac_value = DAC8311_MAX_CODE;
    }

    return FF_SetCurrentCode((uint16_t)dac_value);
}

void PID_ControlTask(void)
{
    int32_t error;
    int32_t derivative;
    int32_t output;

    error = (int32_t)g_current_set_mA - (int32_t)g_actual_current_mA;
    g_pid_error = error;

    if (!g_pid_enabled)
    {
        g_pid_output = 0;
        PID_WriteDacWithFeedback(0);
        return;
    }

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

    g_pid_output = output;
    PID_WriteDacWithFeedback(output);
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


