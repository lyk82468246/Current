//<<AICUBE_USER_HEADER_REMARK_BEGIN>>
////////////////////////////////////////
// 在此添加用户文件头说明信息  
// 文件名称: uart.c
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
static volatile uint8_t g_uart1_rx_overflow = 0;
static char g_uart1_line[UART1_RXSIZE];

static char UART1_ToUpper(char ch);
static char *UART1_SkipSpace(char *str);
static BOOL UART1_IsCommandEnd(char ch);
static BOOL UART1_ParseValue(char *str, int32_t *value);
static BOOL UART1_MatchCommand(char *str, char *cmd);
static void UART1_ProcessCommand(char *line);
//<<AICUBE_USER_GLOBAL_DEFINE_END>>


BOOL fPrintfBusy;
BOOL fUART1ReadyRead;
uint8_t u8UART1RxCount;
uint8_t xdata pu8UART1RxBuffer[UART1_RXSIZE];

////////////////////////////////////////
// 串口1初始化函数
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void UART1_Init(void)
{
    UART1_SwitchP3031();                //设置串口数据端口: RxD (P3.0), TxD (P3.1)

    UART1_Timer2BRT();                  //选择定时器2作为串口1波特率发生器

    UART1_EnableRx();                   //使能串口1接收数据
    UART1_Mode1();                      //设置串口1为模式1 (8位数据可变波特率)
    UART1_SetIntPriority(0);            //设置中断为最低优先级
    UART1_EnableInt();                  //使能串口1中断

    fPrintfBusy = 0;                    //清除发送数据忙标志
    fUART1ReadyRead = 0;                //清除接收数据完成标志
    u8UART1RxCount = 0;                 //初始化接收字节数量

    //<<AICUBE_USER_UART1_INITIAL_BEGIN>>
    // 在此添加用户初始化代码  
    //<<AICUBE_USER_UART1_INITIAL_END>>
}

////////////////////////////////////////
// 重写printf字符发送重定向函数
// 入口参数: dat (printf函数待打印的字符)
// 函数返回: 需要返回入口参数的数据
////////////////////////////////////////
char putchar (char dat)                 //将串口1和printf函数绑定
{
    while (fPrintfBusy);                //等待之前的数据发送完成
    UART1_SendData(dat);                //发送当前字节
    fPrintfBusy = 1;                    //设置发送忙标志

    return dat;
}

////////////////////////////////////////
// 串口1发送数据函数
// 入口参数: dat (待发送的字节数据)
// 函数返回: 无
////////////////////////////////////////
void UART1_SendByte(uint8_t dat)
{
    putchar((char)dat);
}

////////////////////////////////////////
// 串口1发送多字节数据函数
// 入口参数: dat  (发送数据缓冲区)
//           size (数据大小)
// 函数返回: 无
////////////////////////////////////////
void UART1_SendBuffer(uint8_t *dat, uint8_t size)
{
    while (size--)                      //判断数据是否结束
        UART1_SendByte(*dat++);         //发送当前字节
}

////////////////////////////////////////
// 串口1发送字符串函数
// 入口参数: str  (字符串首地址)
// 函数返回: 无
////////////////////////////////////////
void UART1_SendString(char *str)
{
    printf(str);                        //直接使用printf函数打印字符串
}


////////////////////////////////////////
// 串口1中断服务程序
// 入口参数: 无
// 函数返回: 无
////////////////////////////////////////
void UART1_ISR(void) interrupt UART1_VECTOR
{
    //<<AICUBE_USER_UART1_ISR_CODE1_BEGIN>>
    // 在此添加中断函数用户代码  
    if (UART1_CheckTxFlag())            //判断串口发送中断
    {
        UART1_ClearTxFlag();            //清除串口发送中断标志

        fPrintfBusy = 0;                //清除printf发送忙标志
    }

    if (UART1_CheckRxFlag())            //判断串口接收中断
    {
        uint8_t dat;

        UART1_ClearRxFlag();            //清除串口接收中断标志

        dat = UART1_ReadData();
        if (!fUART1ReadyRead)
        {
            if ((dat == '\r') || (dat == '\n'))
            {
                if (u8UART1RxCount)
                {
                    pu8UART1RxBuffer[u8UART1RxCount] = 0;
                    fUART1ReadyRead = 1;
                }
                return;
            }

            if (u8UART1RxCount < (UART1_RXSIZE - 1))
            {
                pu8UART1RxBuffer[u8UART1RxCount++] = dat;
            }
            else
            {
                pu8UART1RxBuffer[UART1_RXSIZE - 1] = 0;
                g_uart1_rx_overflow = 1;
                fUART1ReadyRead = 1;
            }
        }
    }
    //<<AICUBE_USER_UART1_ISR_CODE1_END>>
}


//<<AICUBE_USER_FUNCTION_IMPLEMENT_BEGIN>>
// 在此添加用户函数实现代码  
static char UART1_ToUpper(char ch)
{
    if ((ch >= 'a') && (ch <= 'z'))
    {
        ch -= 32;
    }

    return ch;
}

static char *UART1_SkipSpace(char *str)
{
    while ((*str == ' ') || (*str == '\t') || (*str == '='))
    {
        str++;
    }

    return str;
}

static BOOL UART1_IsCommandEnd(char ch)
{
    return (ch == 0) || (ch == ' ') || (ch == '\t') || (ch == '=');
}

static BOOL UART1_ParseValue(char *str, int32_t *value)
{
    int32_t result;
    uint8_t negative;

    str = UART1_SkipSpace(str);
    result = 0;
    negative = 0;

    if (*str == '-')
    {
        negative = 1;
        str++;
    }
    else if (*str == '+')
    {
        str++;
    }

    if ((*str < '0') || (*str > '9'))
    {
        return FALSE;
    }

    while ((*str >= '0') && (*str <= '9'))
    {
        result = (result * 10) + (*str - '0');
        str++;
    }

    str = UART1_SkipSpace(str);
    if (*str != 0)
    {
        return FALSE;
    }

    if (negative)
    {
        result = -result;
    }

    *value = result;
    return TRUE;
}

static BOOL UART1_MatchCommand(char *str, char *cmd)
{
    while (*cmd)
    {
        if (UART1_ToUpper(*str) != *cmd)
        {
            return FALSE;
        }
        str++;
        cmd++;
    }

    return UART1_IsCommandEnd(*str);
}

void UART1_SendStatus(void)
{
    printf("STAT,set=%u,act=%u,err=%ld,kp=%ld,ki=%ld,kd=%ld,out=%ld,dac=%u,adc=%u,vcc=%u\r\n",
           g_current_set_mA,
           g_actual_current_mA,
           (long)g_pid_error,
           (long)g_pid_kp,
           (long)g_pid_ki,
           (long)g_pid_kd,
           (long)g_pid_output,
           g_dac_code,
           g_actual_current_adc,
           g_adc_vcc_mV);
}

static void UART1_ProcessCommand(char *line)
{
    int32_t value;
    char *arg;

    line = UART1_SkipSpace(line);
    if (*line == 0)
    {
        return;
    }

    if ((*line == '?') && (*(line + 1) == 0))
    {
        UART1_SendStatus();
        return;
    }

    if (UART1_MatchCommand(line, "STAT"))
    {
        UART1_SendStatus();
        return;
    }

    if (UART1_MatchCommand(line, "SAVE"))
    {
        if (PID_SaveToIAP())
        {
            printf("OK,SAVE\r\n");
        }
        else
        {
            printf("ERR,SAVE\r\n");
        }
        return;
    }

    if (UART1_MatchCommand(line, "DEFAULT"))
    {
        PID_LoadDefault();
        printf("OK,DEFAULT\r\n");
        return;
    }

    if (UART1_MatchCommand(line, "LOAD"))
    {
        if (PID_LoadFromIAP())
        {
            printf("OK,LOAD\r\n");
        }
        else
        {
            printf("ERR,LOAD\r\n");
        }
        return;
    }

    if ((UART1_ToUpper(line[0]) == 'K') &&
        ((UART1_ToUpper(line[1]) == 'P') ||
         (UART1_ToUpper(line[1]) == 'I') ||
         (UART1_ToUpper(line[1]) == 'D')) &&
        UART1_IsCommandEnd(line[2]))
    {
        arg = UART1_SkipSpace(line + 2);
        if (UART1_ParseValue(arg, &value) && PID_SetParam(UART1_ToUpper(line[1]), value))
        {
            printf("OK,K%c=%ld\r\n", UART1_ToUpper(line[1]), (long)value);
        }
        else
        {
            printf("ERR,VALUE\r\n");
        }
        return;
    }

    if (((UART1_ToUpper(line[0]) == 'P') ||
         (UART1_ToUpper(line[0]) == 'I') ||
         (UART1_ToUpper(line[0]) == 'D')) &&
        UART1_IsCommandEnd(line[1]))
    {
        arg = UART1_SkipSpace(line + 1);
        if (UART1_ParseValue(arg, &value) && PID_SetParam(UART1_ToUpper(line[0]), value))
        {
            printf("OK,%c=%ld\r\n", UART1_ToUpper(line[0]), (long)value);
        }
        else
        {
            printf("ERR,VALUE\r\n");
        }
        return;
    }

    printf("ERR,CMD\r\n");
}

void UART1_CommandTask(void)
{
    uint8_t i;
    uint8_t overflow;

    if (!fUART1ReadyRead)
    {
        return;
    }

    DisableGlobalInt();
    overflow = g_uart1_rx_overflow;
    for (i = 0; i < UART1_RXSIZE; i++)
    {
        g_uart1_line[i] = (char)pu8UART1RxBuffer[i];
        if (pu8UART1RxBuffer[i] == 0)
        {
            break;
        }
    }
    g_uart1_line[UART1_RXSIZE - 1] = 0;
    u8UART1RxCount = 0;
    fUART1ReadyRead = 0;
    g_uart1_rx_overflow = 0;
    EnableGlobalInt();

    if (overflow)
    {
        printf("ERR,OVERFLOW\r\n");
        return;
    }

    UART1_ProcessCommand(g_uart1_line);
}
//<<AICUBE_USER_FUNCTION_IMPLEMENT_END>>


