# 2026 复旦大学电子设计竞赛 A 题：精密电流源主控固件

本仓库是基于 AiCube 创建的 AI8051U Keil C251 工程，用于精密电流源数字控制端。当前数字系统负责设定电流输入、数码管显示、OLED 波形显示、串口 PID 调参、DAC 输出控制框架，以及片内/外挂 ADC 采集框架。

## 当前状态

- 主控：STC AI8051U-34K64，普中 51 实验板改造使用。
- 开发环境：Keil uVision 5 + Keil C251 + AiCube。
- 当前可用功能：
  - 8 位板载数码管动态扫描。
  - K3/K4 外部中断按键调节设定电流，并使用 Timer1 做 20ms 防抖。
  - 片内 ADC0 采样波形，SSD1315 I2C OLED 显示 128x64 实时波形。
  - 片内 ADC0 + ADC15 估算实际电流并显示到右侧 4 位数码管。
  - UART1 通过 CH340 做状态回传和 PID 参数调节。
  - PID 框架与 IAP 参数保存框架已加入。
- 待硬件验证：
  - DAC8311IDCKR 外挂 DAC 输出。
  - ADS1110A0IDBVR 外挂精密 ADC。
  - PID 对真实模拟闭环的调节效果。

## 重要约定

AiCube 自动生成代码不要手工修改。自定义代码必须放在对应的用户区：

```c
//<<AICUBE_USER_XXX_BEGIN>>
// user code
//<<AICUBE_USER_XXX_END>>
```

外设模式、分频、DMA、中断使能、采样平均次数等配置应优先在 AiCube 中设置；工程内手写代码只做业务逻辑、运行期任务调度和外设驱动传输过程。

## 引脚映射

| 功能 | 信号 | MCU 引脚 | 说明 |
| --- | --- | --- | --- |
| UART1 | RXD / TXD | P3.0 / P3.1 | 板载 CH340，115200 8N1 |
| 数码管段选 | SA~SH | P0.0~P0.7 | 板载段码，经 74HC245 |
| 数码管位选 | DA / DB / DC | P2.2 / P2.3 / P2.4 | 板载 3-8 译码器 |
| 按键 | ADD / SUB | P3.2 / P3.3 | K3 增加，K4 减少 |
| SSD1315 OLED | SCL / SDA | P1.5 / P1.4 | 当前使用 I2C 128x64 OLED |
| 旧 SPI OLED | RST / D_C | P2.0 / P2.1 | SPI OLED 方案已暂时禁用 |
| DAC8311 | SYNC / SCLK / DIN | P1.1 / AiCube SPI / AiCube SPI | 默认软件 SPI，可编译切换硬件 SPI，硬件待验证 |
| 片内 ADC | ADC0 | AiCube 配置 | 电流采样/波形输入 |
| 片内 ADC | ADC15 | 内部通道 | 用于估算 VCC/Vref |
| ADS1110A0 | SCL / SDA | P1.5 / P1.4 | 与 OLED 共用 I2C，总线设备待验证 |

## 任务划分

### 数码管

代码位置：`Sources/port.c`、`Sources/inc/port.h`

- `SEG_UpdateMemory(idx, current_mA)` 更新显存，不立即扫描。
- `SEG_GROUP_SET_CURRENT` 显示左侧 4 位设定电流。
- `SEG_GROUP_ACTUAL_CURRENT` 显示右侧 4 位实际电流。
- 电流内部按 mA 整数处理，例如 `1234` 显示为 `1.234A`。
- `SEG_ScanNext()` 在 Timer0 1ms 中断内调用，每次扫描 1 位。

### 按键

代码位置：`Sources/exti.c`、`Sources/timer.c`

- INT0/K3/ADD 增加设定电流。
- INT1/K4/SUB 减少设定电流。
- `g_current_mode = 0`：基础部分，允许 `0mA` 或 `100~1000mA`，步进 `50mA`。
- `g_current_mode = 1`：提高部分，允许 `0mA` 或 `100~2000mA`，步进 `100mA`。
- 外部中断只记录按键事件并启动 Timer1；Timer1 到 20ms 后确认电平并执行一次调节。

### ADC 与实际电流

代码位置：`Sources/adc.c`、`Sources/inc/adc.h`

- `ADC_CURRENT_CHANNEL` 当前为 ADC0，用于采样检流放大后的电压。
- `ADC_VREF_CHANNEL` 当前为 ADC15，用内部参考反推 VCC/Vref。
- `CURRENT_SENSE_RES_MOHM` 当前假设 `100mOhm`。
- `CURRENT_AMP_GAIN` 当前假设 `20`，若无 INA/检流放大器可改为 `1`。
- `ADC_UpdateActualCurrentTask()` 在主循环中根据 Timer3 标志调用，更新右侧 4 位数码管。
- ADC15 可补偿 3.3V/5V 或 DC-DC 慢变化带来的 ADC 参考变化，但不能替代最终台表校准。

### OLED 波形

代码位置：`Sources/i2c.c`、`Sources/inc/i2c.h`

- 当前屏幕：SSD1315 I2C OLED，128x64。
- ADC0 波形采样由 Timer0 用户区按 `SSD1315_WAVE_SAMPLE_INTERVAL_MS` 节拍启动。
- ADC 中断完成后将 ADC0 结果写入 OLED 波形环形显存。
- OLED 刷新与 ADC 采样解耦：Timer0 只置 `g_oled_update_pending`，主循环调用 `SSD1315_WaveTask()` 渲染并刷新整屏。
- 当前 I2C/DMA 配置由 AiCube 负责；驱动只在传输时使用 I2C DMA 搬移 OLED 显存。
- 旧 SSD1306 SPI OLED 曾出现缺列/行异常，当前已切换到 SSD1315 I2C 方案。

### UART 与 PID

代码位置：`Sources/uart.c`、`Sources/iap.c`

- UART1 使用板载 CH340，`115200 8N1`。
- Timer2 作为 UART 波特率发生器，当前需使用 1T 配置以降低 40MHz 下的 115200 波特率误差。
- 支持命令：
  - `P value` / `I value` / `D value`
  - `KP value` / `KI value` / `KD value`
  - `SAVE`
  - `LOAD`
  - `DEFAULT`
  - `STAT` 或 `?`
- 状态回传格式：

```text
STAT,set=...,act=...,err=...,kp=...,ki=...,kd=...,out=...,dac=...,adc=...,vcc=...
```

- PID 参数使用定点数，`PID_SCALE=1000`，例如 `P 1200` 表示 `Kp=1.200`。
- PID 控制任务当前在实际电流刷新后由主循环调用，计算结果写入 DAC8311 输出码。

### DAC8311

代码位置：`Sources/spi.c`、`Sources/inc/spi.h`

- 型号：DAC8311IDCKR，14 位单通道 DAC。
- 默认使用 P1.1/P1.2/P1.3 软件 SPI 驱动。
- `DAC8311_USE_HARDWARE_SPI` 默认为 `0`；改为 `1` 时，`SYNC` 仍使用原 GPIO，SCLK/DIN 改用 AiCube 当前配置的硬件 SPI 引脚和 SPI 模式。
- SPI 引脚映射、SPI mode、分频等属于 AiCube 管辖内容，应在 AiCube 中修改，代码只在用户区切换 DAC8311 的发送路径。
- 已实现 `DAC8311_WriteCode()`、`DAC8311_WriteRaw()`、`DAC8311_PowerDown()` 等函数。
- DAC 硬件尚未到货，输出电压、时序和真实 PID 联动仍待验证。

### ADS1110A0

代码位置：`Sources/i2c.c`、`Sources/inc/i2c.h`

- 型号：ADS1110A0IDBVR，I2C 16 位 Delta-Sigma ADC。
- 7 位地址：`0x48`。
- 已实现配置写入、单次转换启动、原始值读取和 uV 定点换算。
- 硬件尚未到货，I2C 通信、输入极性、噪声和量程仍待验证。

## 目录结构

```text
Current
├─ Sources
│  ├─ main.c          主循环、任务调度、系统初始化入口
│  ├─ port.c          数码管显存与动态扫描
│  ├─ timer.c         Timer0 扫描/采样节拍、Timer1 防抖、Timer3 慢任务
│  ├─ exti.c          ADD/SUB 外部中断按键
│  ├─ adc.c           片内 ADC、电流换算、ADC0 波形采样入口
│  ├─ i2c.c           SSD1315 OLED 与 ADS1110 驱动
│  ├─ spi.c           DAC8311 与旧 SPI OLED 保留代码
│  ├─ uart.c          串口命令解析和状态回传
│  ├─ iap.c           PID 参数与 IAP 存储
│  └─ inc             头文件
├─ Current.aic        AiCube 配置
├─ Current.uvproj     Keil 工程
└─ README.md
```

## 编译与下载

1. 使用 Keil uVision 5 打开 `Current.uvproj`。
2. 确认使用 Keil C251 编译器。
3. Rebuild，确认 0 Errors。
4. 使用 STC-ISP 下载，芯片型号选择 AI8051U-34K64。

## 进度

- 已完成：AiCube 工程配置、数码管扫描、按键调节、防抖、SSD1315 I2C OLED 波形显示、片内 ADC 实际电流显示、UART 调参、PID/IAP 框架。
- 待验证：DAC8311 实物输出、ADS1110A0 实物采样、真实闭环 PID 效果。
- 后续重点：ADC/DAC 标定、采样电阻和放大器实测参数校准、PID 联调、模拟板 PCB 与最终闭环测试。

更新日期：2026-06-06
