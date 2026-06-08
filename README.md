# 2026 大学电子设计竞赛 A 题：精密电流源主控固件

本仓库是基于 AiCube 创建的 AI8051U Keil C251 工程，用于精密电流源数字控制端。当前数字系统负责设定电流输入、数码管显示、OLED 趋势/调试波形显示、UART PID 调参、DAC 输出控制框架，以及片内/外挂 ADC 采样框架。

## 当前状态

- 主控：STC AI8051U-34K64，普通 51 实验板改造使用。
- 开发环境：Keil uVision 5 + Keil C251 + AiCube。
- 当前可用功能：
  - 8 位板载数码管动态扫描。
  - K3/K4 外部中断按键调节设定电流，并使用 Timer1 做 20ms 防抖和长按识别。
  - 片内 ADC0 保留为 OLED 全局模式快速调试波形输入。
  - INA250A2PWR 电流采样芯片输出接入 ADS1110A0，已作为实际电流和 PID 数据源接入代码，默认 16bit / 15SPS / PGA=2。
  - SSD1315 I2C OLED 默认进入聚焦模式，用 ADS1110 电流样本显示低速精密趋势。
  - UART1 通过 CH340 做状态回传和 PID 参数调节。
  - PID 框架与 IAP 参数保存框架已加入。
- 待硬件验证：
  - DAC8311IDCKR 外挂 DAC 输出。
  - INA250A2PWR + ADS1110A0IDBVR 电流采样链路实物采样。
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
| DAC8311 | SYNC / SCLK / DIN | P1.1 / AiCube SPI 或 P1.2 / AiCube SPI 或 P1.3 | 默认软件 SPI，可编译切换硬件 SPI，硬件待验证 |
| 片内 ADC | ADC0 | AiCube 配置 | OLED 全局模式快速调试波形输入 |
| 片内 ADC | ADC15 | 内部通道 | 旧备用方案中用于估算 VCC/Vref |
| INA250A2 + ADS1110A0 | SCL / SDA | P1.5 / P1.4 | 与 OLED 共用 I2C，作为实际电流/PID 数据源，待实物验证 |

## 任务划分

### 数码管

代码位置：`Sources/port.c`、`Sources/inc/port.h`

- `SEG_UpdateMemory(idx, current_mA)` 更新显存，不立即扫描。
- `SEG_GROUP_SET_CURRENT` 显示左侧 4 位设定电流；第 4 位复用为模式标识，基础模式显示 `b`，提高模式显示 `P`。
- `SEG_GROUP_ACTUAL_CURRENT` 显示右侧 4 位实际电流。
- 电流内部按 mA 整数处理，例如 `1234` 显示为 `1.234A`。
- `SEG_ScanNext()` 在 Timer0 1ms 中断内调用，每次扫描 1 位。

### 按键

代码位置：`Sources/exti.c`、`Sources/timer.c`

- INT0/K3/ADD 单击增加设定电流，长按约 1 秒切换基础/提高模式。
- INT1/K4/SUB 单击减少设定电流，长按约 1 秒切换 OLED 示波器显示模式。
- `g_current_mode = 0`：基础部分，允许 `0~1000mA`，步进 `50mA`。
- `g_current_mode = 1`：提高部分，允许 `0~2000mA`，步进 `100mA`。
- 外部中断只记录按键事件并启动 Timer1；Timer1 到 20ms 后确认电平并执行一次调节。
- ADD 长按切换规则：基础切到提高时保留当前设定值，并按 100mA 步进向下取整；提高切回基础时，若超过 1000mA 则钳位到 1000mA，否则保留当前设定值。
- SUB 长按切换 OLED 示波器模式：全局模式保留为片内 ADC0 快速调试波形；聚焦模式使用 ADS1110 电流样本，根据最近 128 点数据范围自动缩放显示范围。

### ADC 与实际电流

代码位置：`Sources/adc.c`、`Sources/inc/adc.h`、`Sources/i2c.c`、`Sources/inc/i2c.h`

- 当前默认 `CURRENT_SAMPLE_SOURCE = CURRENT_SAMPLE_SOURCE_INA250_ADS1110`。
- 实际电流和 PID 数据源来自 INA250A2PWR + ADS1110A0；片内 ADC0 仅保留为 OLED 全局模式快速调试输入。
- INA250A2 输出比例按 `INA250A2_UV_PER_MA = 500` 换算，即 `500uV/mA`；`INA250_ZERO_OFFSET_UV` 预留零点校准，当前默认 0。
- ADS1110 配置为连续转换、15SPS、16bit、PGA=2；读取结果为 16 位有符号二补码，负值在电流换算时钳为 0。
- 旧备用方案仍保留：片内 ADC 或 ADS1110 可按 `100mOhm` 采样电阻、OPA189 `10.1` 倍放大进行换算。
- `ADC_UpdateActualCurrentTask()` 在主循环中根据 Timer3 标志调用；只有 ADS1110 有新数据时才更新右侧数码管并触发 PID。
- ADS1110 与 OLED 共用 I2C。OLED 使用 I2C DMA 刷屏；ADS1110 读写前会关闭 DMA 并等待总线空闲，下一次 OLED 刷新会重新配置并启用 DMA。
- 新 INA250+ADS1110 默认路径不再依赖 ADC15/VCC 校正。
- Timer3 已改为匹配 ADS1110 慢速采样任务；如果后续改变 Timer3 周期，需要同步调整 `SSD1315_EXT_SAMPLE_INTERVAL_MS`，以保证 OLED 聚焦模式时基标注正确。

### OLED 波形

代码位置：`Sources/i2c.c`、`Sources/inc/i2c.h`

- 当前屏幕：SSD1315 I2C OLED，128x64。
- 开机默认进入聚焦模式；聚焦模式使用 ADS1110 解算后的电流样本，单位为 `0.1mA`。
- 全局模式保留片内 ADC0 快速调试波形：ADC0 采样由 Timer0 用户区按 `SSD1315_WAVE_SAMPLE_INTERVAL_MS` 节拍启动，ADC 中断完成后写入 OLED 波形环形显存。
- OLED 刷新与 ADC/ADS1110 采样解耦：Timer0 只置 `g_oled_update_pending`，主循环调用 `SSD1315_WaveTask()` 渲染并刷新整屏；聚焦模式只有在 ADS1110 有新样本后才刷新。
- 当前 I2C/DMA 配置由 AiCube 负责；驱动只在传输时使用 I2C DMA 搬移 OLED 显存。
- OLED 示波器支持全局/聚焦两种模式。全局模式显示片内 ADC0 全量程调试波形；聚焦模式保存最近 128 个 ADS1110 电流样本，刷新时按窗口最小/最大值和边距自动设置纵轴范围，适合观察稳定直流附近的小纹波。
- OLED 四角使用 3x5 小字反色叠加当前显示信息：左上为屏幕上限电流，左下为屏幕下限电流，右上为时基，右下为每格电流。
- 旧 SSD1306 SPI OLED 曾出现缺列/行异常，当前已切换到 SSD1315 I2C 方案。

### UART 与 PID

代码位置：`Sources/uart.c`、`Sources/iap.c`

- UART1 使用板载 CH340，`115200 8N1`。
- Timer2 作为 UART 波特率发生器，当前需使用 1T 配置以降低 40MHz 下的 115200 波特率误差。
- 支持命令：
  - `P value` / `I value` / `D value`
  - `KP value` / `KI value` / `KD value`
  - `PID ON` / `PID OFF` / `PID 1` / `PID 0`
  - `FF` / `FF value` / `FFABS value`
  - `FFREL delta`
  - `SAVE`
  - `LOAD`
  - `DEFAULT`
  - `STAT` 或 `?`
- 状态回传格式：

```text
STAT,set=...,act=...,err=...,kp=...,ki=...,kd=...,out=...,ff=...,dac=...,piden=...,adc=...,vcc=...
```

- PID 参数使用定点数，`PID_SCALE=1000`，例如 `P 1200` 表示 `Kp=1.200`。
- PID 反馈默认开启，可通过串口临时关闭；关闭后 DAC 只输出当前设定点对应的前馈码。
- PID 控制任务当前在 INA250+ADS1110 实际电流刷新后由主循环调用，最终 DAC 输出为“前馈查表码 + PID 反馈修正量”。

### DAC 前馈

代码位置：`Sources/iap.c`、`Sources/inc/iap.h`

- 前馈始终存在，设定电流或基础/提高模式变化后，主循环会查表并立即更新 DAC。
- 基础模式前馈表包含 `0~1000mA`，50mA 步进，共 21 点。
- 提高模式前馈表包含 `0~2000mA`，100mA 步进，共 21 点。
- 两张表互相独立；同一个电流在基础和提高模式下可以保存不同前馈 DAC 码。
- 默认表按当前 INA250A2 理想输出初始化：`500uV/mA`、`DAC8311_REF_MV = 5000mV`。旧备用采样方案下仍可按 `100mOhm` 和 `10.1` 倍放大初始化。
- 当前 IAP 存储版本为 `PID4`；旧 `PID3` 前馈表会被忽略并重新生成默认表。
- `FF` 查询当前模式和当前设定电流对应的前馈码。
- `FF value` 或 `FFABS value` 设置当前点的绝对前馈 DAC 码。
- `FFREL delta` 对当前点前馈码做相对增减。
- `SAVE` / `LOAD` / `DEFAULT` 会同时管理 PID 参数、PID 开关状态和两张前馈表。

### DAC8311

代码位置：`Sources/spi.c`、`Sources/inc/spi.h`

- 型号：DAC8311IDCKR，14 位单通道 DAC。
- 默认使用 P1.1/P1.2/P1.3 软件 SPI 驱动。
- `DAC8311_USE_HARDWARE_SPI` 默认 `0`；改为 `1` 时，`SYNC` 仍使用原 GPIO，SCLK/DIN 改用 AiCube 当前配置的硬件 SPI 引脚和 SPI 模式。
- SPI 引脚映射、SPI mode、分频等属于 AiCube 管辖内容，应在 AiCube 中修改；代码只在用户区切换 DAC8311 的发送路径。
- 已实现 `DAC8311_WriteCode()`、`DAC8311_WriteRaw()`、`DAC8311_PowerDown()` 等函数。
- DAC 硬件尚未到货，输出电压、时序和真实 PID 联动仍待验证。

### ADS1110A0

代码位置：`Sources/i2c.c`、`Sources/inc/i2c.h`

- 型号：ADS1110A0IDBVR，I2C 16 位 Delta-Sigma ADC。
- 7 位地址：`0x48`。
- 已实现配置写入、单次转换启动、原始值读取和 uV 定点换算。
- 当前通过 INA250A2 输出接入 ADS1110，作为实际电流和 PID 数据源；I2C 通信、输入极性、噪声、PGA=2 下的量程裕量和真实精度仍待验证。

## 目录结构

```text
Current
├─ Sources
│  ├─ main.c          主循环、任务调度、系统初始化入口
│  ├─ port.c          数码管显存与动态扫描
│  ├─ timer.c         Timer0 扫描/波形采样节拍、Timer1 防抖、Timer3 慢任务
│  ├─ exti.c          ADD/SUB 外部中断按键
│  ├─ adc.c           片内 ADC、实际电流换算、OLED 波形采样入口
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

- 已完成：AiCube 工程配置、数码管扫描、按键调节、防抖、长按模式切换、SSD1315 I2C OLED 聚焦/全局显示、INA250+ADS1110 实际电流/PID 数据源代码接入、UART 调参、PID/IAP 框架。
- 待验证：DAC8311 实物输出、INA250A2+ADS1110A0 实物采样、真实闭环 PID 效果。
- 后续重点：ADC/DAC 标定、INA250 零点校准、PGA=2 量程裕量验证、PID 联调、模拟板 PCB 与最终闭环测试。

更新日期：2026-06-07
