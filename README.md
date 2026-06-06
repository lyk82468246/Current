

# 2026 复旦大学电子设计竞赛 (A题：精密电流源设计) - 主控固件

## 🏆 项目简介
本项目为 2026 年复旦大学电子设计竞赛 A 题（精密电流源）的数字控制端固件。
系统采用“纯硬件运放闭环线性稳流”作为模拟核心架构，数字控制端创造性地采用了 **STC AI8051U (32位扩展8051)** 搭配 **普中51实验板** 进行魔改化开发。利用 AI8051U 强大的运算能力（42MHz主频）和超大内存（34KB SRAM），完美实现了题设要求的“四位数码管设定”、“电流曲线描绘”以及多点高精度数字校准。

## 🛠 硬件架构与选型
*   **MCU 主控**：STC AI8051U-34K64-PDIP40（**注意：需将普中板原有的 STC89C52RC 拔下，原位替换插入此芯片**）
*   **开发底板**：普中 51-单核-A2/A3/A4 实验板（利用板载 CH340、独立按键、数码管模块）
*   **显示模块**：
    *   板载 8 位动态数码管（左 4 位显示设定电流，右 4 位显示实际电流）
    *   外挂 0.96 寸 SSD1306 SPI OLED 屏幕（128x64，用于显示 ADC 实时波形）
*   **模拟前段 (外挂模块)**：
    *   **DAC 给定**：DAC8311IDCKR（14位单通道 SPI DAC），负责输出参考电压给运放；驱动已编译通过，硬件待到货验证。
    *   **ADC 采集**：当前波形显示和右侧数码管实际电流暂使用 AI8051U 片内 12 位 ADC；外挂精密 ADC 已确定采用 ADS1110A0IDBVR（16位单通道 I2C ADC），驱动已编译通过，硬件待到货验证。
    *   **核心功率级**：OPA2333精密运放 + IRF540N 功率MOS管（工作于放大区）组成的压控电流源。

## 🔌 硬件引脚映射 (Pin Mapping)
请严格按照下表在普中板的排针处接线，切勿冲突：

| 外设模块 | 管脚名称 | MCU 引脚 | 备注说明 |
| :--- | :--- | :--- | :--- |
| **程序下载/调试** | RXD / TXD | `P3.0` / `P3.1` | 复用普中板载 CH340，直接插 USB 下载 |
| **板载数码管** | 段码 (DPa~DPh) | `P0.0` ~ `P0.7` | 经板载 74HC245 驱动 |
| **板载数码管** | 位码 (A,B,C) | `P2.2`, `P2.3`, `P2.4`| 经板载 74HC138 译码驱动 8 位 |
| **板载按键** | K3 / K4 | `P3.2` / `P3.3` | K3 为电流增加 (+)，K4 为减少 (-) |
| **外挂 OLED (SPI)** | SCL / SDA / RST / D/C | `P2.7` / `P2.5` / `P2.0` / `P2.1` | 丝印 SCL/SDA 实际作为 SPI SCLK/MOSI；无 CS 引脚 |
| **外挂 ADC (I2C)** | SCL / SDA | `P1.5` / `P1.4` | ADS1110A0IDBVR 精密测量接口；驱动已编译通过，硬件待到货验证 |
| **外挂 DAC8311 (软件 SPI)** | SYNC / SCLK / DIN | `P1.1` / `P1.2` / `P1.3` | 驱动已编译通过，硬件待到货验证 |

## 💻 软件开发环境
*   **IDE**：Keil uVision 5
*   **编译器**：**Keil C251**（⚠️ 警告：千万不要用老款的 C51 编译器，无法编译 AI8051U 的 32 位扩展指令集）
*   **底层代码生成器**：STC-ISP v6.96 内部集成的 **AiCube**（类似 STM32CubeMX，用于图形化生成时钟、GPIO、定时器初始化代码）
*   **下载工具**：STC-ISP（烧录时需冷启动）

## 📂 工程目录结构
```text
📦 Current
 ┣ 📂 Sources
 ┃ ┣ 📜 main.c             # 主函数、系统初始化与当前显示测试入口
 ┃ ┣ 📜 port.c             # GPIO 初始化、数码管显存更新与动态扫描
 ┃ ┣ 📜 timer.c            # Timer0 1ms 数码管扫描、Timer1 20ms 按键防抖
 ┃ ┣ 📜 exti.c             # 外部中断按键事件与电流设定调节
 ┃ ┣ 📜 adc.c              # AI8051U 片内 ADC 初始化与转换函数
 ┃ ┣ 📜 spi.c              # SPI SSD1306 OLED 初始化、波形刷新与 DAC8311 软件 SPI 驱动
 ┃ ┣ 📜 i2c.c              # AI8051U 硬件 I2C 底层读写函数与 ADS1110 外挂 ADC 驱动
 ┃ ┣ 📜 uart.c             # UART1 串口命令解析、状态回传与调参接口
 ┃ ┣ 📜 iap.c              # IAP 参数存储、PID 参数与控制任务
 ┃ ┗ 📂 inc
 ┃   ┣ 📜 config.h         # 全局配置、引脚别名与公共头文件
 ┃   ┣ 📜 port.h           # 端口与数码管接口声明
 ┃   ┣ 📜 timer.h
 ┃   ┣ 📜 uart.h
 ┃   ┣ 📜 iap.h
 ┃   ┣ 📜 exti.h
 ┃   ┣ 📜 adc.h
 ┃   ┣ 📜 spi.h
 ┃   ┗ 📜 i2c.h
 ┣ 📜 Current.uvproj       # Keil uVision 工程
 ┣ 📜 Current.uvopt        # Keil 工程选项
 ┣ 📜 Current.aic          # AiCube 配置文件
 ┗ 📜 README.md            # 本说明文件
```

## 🚀 核心算法说明
1. **多任务伪操作系统 (裸机状态机)**：
   依靠 Timer0 的 1ms 中断维护数码管动态扫描。当前工程已经在 Timer0 中断中调用 `SEG_ScanNext()`，每次只扫描 1 位数码管；Timer1 作为一次性 20ms 按键防抖定时器，仅在外部中断触发后启动。
2. **多点线性校准 (Calibration)**：
   规划用于补偿硬件电阻温漂、运放失调电压和 DAC/ADC 链路误差。后续可在独立 `calibration.c/.h` 中放置 `[设定电流] -> [DAC控制字]` 映射表和线性插值算法。
3. **曲线绘制 (Curve Draw)**：
   当前使用 SSD1306 SPI OLED 显示 ADC 实时波形。Timer3 到期后只置位刷新标志，主循环调用 `OLED_WaveTask()` 读取 `ADC_Convert(ADC_WAVE_CHANNEL)`，将 12 位 ADC 结果映射到 0~63 行，并用 128 点环形缓存重建整屏波形。
4. **ADC15 供电补偿**：
   当前实际电流显示使用片内 ADC0 采样检流放大后的电压，同时读取 ADC15 内部参考通道反推当前 ADC 参考电压/VCC。该机制用于补偿 3.3V/5V 供电差异和 DC-DC 慢漂，但 ADC15 的内部约 1.19V 参考本身不是精密基准，最终需要用实测 VCC 校准 `ADC_INTERNAL_REF_MV`。
5. **串口调参与 PID 慢速修正**：
   UART1 通过板载 CH340 连接上位机，Timer2 作为 115200 波特率发生器并已切换到 1T 以降低波特率误差。串口命令在中断中按行接收，主循环解析；PID 参数使用定点数并可通过 IAP 保存。当前串口通信和参数解析已上机验证基本正常，DAC/ADC 外设尚未到货，PID 真实闭环表现待验证。

## 🔢 数码管显示接口
当前已经实现板载 8 位动态数码管驱动，相关代码位于 `Sources/port.c` / `Sources/inc/port.h`。

*   `SEG_UpdateMemory(idx, current_mA)`：更新数码管显存，不会立即自动扫描。
    *   `idx = SEG_GROUP_SET_CURRENT`：更新左 4 位，显示设定电流。
    *   `idx = SEG_GROUP_ACTUAL_CURRENT`：更新右 4 位，显示实际电流。
    *   `current_mA` 使用 mA 整数单位，例如 `1234` 显示为 `1.234A`，范围超过 `9999mA` 时按 `9.999A` 限幅。
*   `SEG_ScanNext()`：动态扫描下一位数码管。当前在 Timer0 的 1ms 中断中调用。
*   显存 `g_seg_display_buf[8]` 按逻辑从左到右保存；由于实测普中板物理位选顺序相反，扫描时已在代码中做反向位选映射，小数点跟随对应的千分位显示。

当前设定电流按 mA 整数处理；实际测量链路目前也输出 mA 用于 4 位数码管显示。若后续要把实测值参与 DAC 误差控制，建议升级为 uA 定点内部计算，再在显示层四舍五入成 mA。

## 🔘 按键与防抖
当前已实现 K3/K4 外部中断按键调节，相关代码位于 `Sources/exti.c` / `Sources/inc/exti.h`。

*   `INT0 / K3 / ADD`：增加设定电流。
*   `INT1 / K4 / SUB`：减少设定电流。
*   `g_current_mode = 0`：基础部分，允许 `0mA` 或 `100~1000mA`，步进 `50mA`。
*   `g_current_mode = 1`：提高部分，允许 `0mA` 或 `100~2000mA`，步进 `100mA`。
*   外部中断只记录按键待确认标志并启动 Timer1；Timer1 到 20ms 后读取 `ADD/SUB` 当前电平，若仍为低电平才执行一次调节。
*   所有手写逻辑都放在 AiCube 的 `//<<AICUBE_USER_...>>` 用户区内，避免重新生成配置时被覆盖。

## 📈 OLED 波形显示
当前已实现 128x64 SSD1306 SPI OLED 波形显示，相关代码位于 `Sources/spi.c` / `Sources/inc/spi.h`。

*   `OLED_Init()`：在 `SPI_Init()` 的用户初始化区调用，完成 SSD1306 初始化。
*   `OLED_WaveTask()`：读取 ADC 通道 `ADC_WAVE_CHANNEL`，更新 128 点环形波形缓存，并整屏刷新 1024 字节 OLED 显存。
*   Timer3 当前由 AiCube 配置为 100ms 周期；Timer3 中断只设置 `g_oled_update_pending`，真正的 OLED 整屏刷新放在主循环执行，避免阻塞 1ms 数码管动态扫描。
*   ADC 当前为 12 位右对齐，`0~4095` 通过 `adc >> 6` 映射到 OLED 的 `0~63` 行。
*   注意采样混叠：100ms 采样率只有 10Hz，测试高频正弦时会出现相位翻转或伪波形。例如 25Hz 正弦每次采样跨过 2.5 个周期，显示上会呈现奇偶列近似反相。

## 📏 ADC 实际电流显示
当前右侧 4 位数码管显示由 `ADC_UpdateActualCurrentTask()` 更新，相关代码位于 `Sources/adc.c` / `Sources/inc/adc.h`。

*   `ADC_CURRENT_CHANNEL`：实际电流采样通道，当前为 ADC0。
*   `ADC_VREF_CHANNEL`：内部参考测量通道，当前为 ADC15。
*   `ADC_INTERNAL_REF_MV`：ADC15 内部参考标称值，当前为 `1190mV`；最终应在实物上用万用表测 MCU VCC 后反推校准。
*   `CURRENT_SENSE_RES_MOHM`：串联采样电阻，单位 mΩ，当前假设 `100mΩ`。
*   `CURRENT_AMP_GAIN`：INA/检流放大器增益，当前假设 `20`；若无放大器可改为 `1`。
*   计算流程：`ADC15 -> VCC_mV`，`ADC0 -> 输入电压 uV`，再按采样电阻和放大倍数换算成 mA 并更新右 4 位显示。
*   若 MCU VCC 来自 DC-DC，ADC15 可补偿慢变化的 VCC 偏移，但不能消除高频纹波、ADC 非线性、INA 失调和采样电阻误差；最终精度仍依赖零点/增益/多点校准。

## 📐 ADS1110 外挂精密 ADC
当前已加入 ADS1110A0IDBVR 的 I2C 驱动，相关代码位于 `Sources/i2c.c` / `Sources/inc/i2c.h`。

*   ADS1110A0 为 16 位单通道 Delta-Sigma ADC，I2C 7 位地址为 `0x48`，使用内部 2.048V 参考源。
*   `ADS1110_Init()`：写入默认配置，当前为连续转换、16bit、15SPS、PGA=1。
*   `ADS1110_StartSingle(config)`：启动单次转换，可指定数据率与 PGA。
*   `ADS1110_ReadRaw(raw, config)`：读取转换结果高字节、低字节和配置字节，结果为有符号 16 位二补码。
*   `ADS1110_RawToMicroVolt(raw, config)`：按当前数据率和 PGA 将原始码换算为输入电压，单位为 uV，使用整数定点计算。
*   当前仅完成编译验证，ADS1110A0 硬件尚未到货，I2C 通信、输入极性、量程和噪声表现仍待实物验证。

## 🧭 UART 调参与 PID 控制
当前已加入 UART1 串口调参与 PID 控制框架，相关代码位于 `Sources/uart.c` / `Sources/inc/uart.h` 和 `Sources/iap.c` / `Sources/inc/iap.h`。

*   UART1 使用 `P3.0/P3.1` 连接板载 CH340，串口格式为 `115200 8N1`；Timer2 作为波特率发生器，需配置为 1T，否则 40MHz 下 115200 误差过大容易乱码。
*   串口接收缓冲区当前为 64 字节；中断只负责按行接收，主循环调用 `UART1_CommandTask()` 解析命令。
*   支持命令：`P value` / `I value` / `D value`，也兼容 `KP value` / `KI value` / `KD value`。PID 参数使用定点数，`PID_SCALE=1000`，例如 `P 1200` 表示 `Kp=1.200`。
*   `SAVE`：将当前 PID 参数写入 IAP；`LOAD`：从 IAP 加载已保存参数；`DEFAULT`：恢复固件内置默认参数；`?` 或 `STAT`：立即回传当前状态。
*   Timer3 每 1 秒置位一次串口遥测标志，主循环回传一行 `STAT,set=...,act=...,err=...,kp=...,ki=...,kd=...,out=...,dac=...,adc=...,vcc=...`。
*   `PID_ControlTask()` 在实际电流刷新后由主循环调用，根据设定电流和实际电流误差计算输出，并调用 `DAC8311_WriteCode()` 更新 DAC 码。
*   当前仅验证了串口通信、命令解析和状态回传；由于 DAC8311 与 ADS1110A0 尚未到货，PID 对真实电流源的闭环调节效果仍待实物验证。

## 🎚 DAC8311 输出
当前已加入 DAC8311IDCKR 的 3 线软件 SPI 驱动，相关代码位于 `Sources/spi.c` / `Sources/inc/spi.h`。

*   引脚别名在 `Sources/inc/config.h` 中定义：`DAC8311_SYNC=P1.1`、`DAC8311_SCLK=P1.2`、`DAC8311_DIN=P1.3`。
*   `DAC8311_WriteCode(dac_code)`：写入 14 位 DAC 码，范围 `0~0x3FFF`。
*   `DAC8311_WriteRaw(frame)`：写入完整 16 位帧，高 2 位为掉电控制，低 14 位为 DAC 数据。
*   `DAC8311_PowerDown(power_mode)`：进入 DAC8311 掉电模式。
*   当前驱动仅编译验证通过，DAC8311 硬件尚未到货，输出电压和时序仍待实物验证。
*   OLED 因无 CS 引脚继续独占硬件 SPI；DAC8311 使用独立 GPIO 软件 SPI，避免 OLED 在硬件 SPI 总线上误接收 DAC 数据。

## ⚙️ 编译与下载指南
1. 确保电脑已安装 Keil C251，并在 STC-ISP 中将 STC 芯片包导入到 Keil 中。
2. 双击工程文件打开，点击 `Rebuild` 或按 `F7` 进行编译，确保 0 Errors。
3. 打开 STC-ISP 软件：
   * 芯片型号选择 `AI8051U-34K64`
   * 波特率设置推荐 `115200`
   * 点击“下载/编程”后，按下普中开发板上的白色电源开关（关闭再开启，完成冷启动即可自动下载）。

---
**👨‍💻 参赛团队**：[在这里填入你的学校/团队名]
**📅 更新日期**：2026 年 6 月 6 日
**⚠️ 进度标记**：`[已完成] LTspice全链路仿真` -> `[已完成] AiCube底层配置、数码管动态扫描、按键防抖调节、OLED 波形显示与 UART 调参框架` -> `[待验证] DAC8311 软件 SPI 输出` -> `[待验证] ADS1110A0 I2C 精密 ADC` -> `[待验证] PID 真实闭环调节` -> `[进行中] ADC/DAC 标定与闭环联调` -> `[待进行] 模拟板PCB打样与闭环联调`
